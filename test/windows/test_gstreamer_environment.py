"""Regression checks for Windows GStreamer discovery and minimum version."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
HELPER = PROJECT_ROOT / "scripts" / "setup_gstreamer.bat"
VERSION_CHECK = PROJECT_ROOT / "scripts" / "check_gstreamer_version.ps1"
ROOT_VARIABLES = (
    "GSTREAMER_1_0_ROOT_MSVC_X86_64",
    "GSTREAMER_1_0_ROOT_X86_64",
)


@unittest.skipUnless(sys.platform == "win32", "requires Windows CMD")
class GStreamerEnvironmentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        temporary = tempfile.TemporaryDirectory(prefix="dx-gst-inspector-")
        cls.addClassCleanup(temporary.cleanup)
        root = Path(temporary.name)
        cls.inspector = root / "gst-inspect-1.0.exe"
        script = root / "compile-inspector.ps1"
        script.write_text(
            """$code = @'
using System;
using System.IO;
using System.Threading;

public static class Inspector {
    public static int Main(string[] args) {
        if (args.Length != 1 || args[0] != "--version") return 99;
        string root = AppDomain.CurrentDomain.BaseDirectory;
        string delay = Path.Combine(root, "delay.txt");
        if (File.Exists(delay)) Thread.Sleep(int.Parse(File.ReadAllText(delay)));
        Console.Write(File.ReadAllText(Path.Combine(root, "version.txt")));
        string error = Path.Combine(root, "stderr.txt");
        if (File.Exists(error)) Console.Error.Write(File.ReadAllText(error));
        return int.Parse(File.ReadAllText(Path.Combine(root, "exit-code.txt")));
    }
}
'@
Add-Type -TypeDefinition $code -OutputType ConsoleApplication -OutputAssembly """
            + f"'{str(cls.inspector).replace(chr(39), chr(39) * 2)}'\n"
        )
        result = subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
             "-File", str(script)],
            capture_output=True, text=True, timeout=30,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stdout + result.stderr)

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="dx-gst-env-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.modern = self.make_sdk("Modern SDK", development=True)
        self.legacy = self.make_sdk("Legacy SDK", development=True)

    def make_sdk(self, name, *, development):
        root = self.root / name
        (root / "bin").mkdir(parents=True)
        for command in ("gst-launch-1.0.exe", "gst-inspect-1.0.exe"):
            (root / "bin" / command).touch()
        shutil.copyfile(self.inspector, root / "bin" / "gst-inspect-1.0.exe")
        (root / "bin" / "version.txt").write_text(
            "gst-inspect-1.0 version 1.28.4\nGStreamer 1.28.4\nUnknown package origin\n"
        )
        (root / "bin" / "exit-code.txt").write_text("0")
        if development:
            (root / "lib" / "pkgconfig").mkdir(parents=True)
            (root / "lib" / "pkgconfig" / "gstreamer-1.0.pc").touch()
        return root

    def run_version_check(self, sdk=None):
        return subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
             "-File", str(VERSION_CHECK), "-GStreamerRoot", str(sdk or self.legacy)],
            capture_output=True, text=True, timeout=20,
        )

    def test_minimum_version_boundaries(self):
        cases = (
            ("1.9.99", False),
            ("1.16.2", False),
            ("1.16.3", True),
            ("1.16.10", True),
            ("1.26.1", True),
            ("1.28.4", True),
            ("1.100.0", True),
            ("2.0.0", True),
        )
        for version, supported in cases:
            with self.subTest(version=version):
                (self.legacy / "bin" / "version.txt").write_text(
                    f"gst-inspect-1.0 version {version}\nGStreamer {version}\n"
                )
                result = self.run_version_check()
                output = result.stdout + result.stderr
                self.assertEqual(result.returncode, 0 if supported else 1, output)
                self.assertIn(str(self.legacy), output)
                self.assertIn(version, output)
                self.assertIn("1.16.3", output)
                if supported:
                    self.assertIn("[OK]", output)
                else:
                    self.assertIn("[ERROR]", output)
                    self.assertIn("https://gstreamer.freedesktop.org/download/", output)

    def test_version_check_uses_selected_sdk_not_path(self):
        (self.legacy / "bin" / "version.txt").write_text("GStreamer 1.16.2\n")
        env = os.environ.copy()
        env["PATH"] = str(self.modern / "bin") + ";" + env["PATH"]
        result = subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
             "-File", str(VERSION_CHECK), "-GStreamerRoot", str(self.legacy)],
            env=env, capture_output=True, text=True, timeout=20,
        )
        output = result.stdout + result.stderr
        self.assertEqual(result.returncode, 1, output)
        self.assertIn("1.16.2", output)

    def test_invalid_version_output_reports_error(self):
        for output in ("", "not a version\n", "GStreamer 1.28\n",
                       "GStreamer 1.28.4.5\n", "GStreamer 1.28.4\nGStreamer 1.16.3\n"):
            with self.subTest(output=output):
                (self.legacy / "bin" / "version.txt").write_text(output)
                result = self.run_version_check()
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertIn("[ERROR]", result.stdout)
                self.assertIn("1.16.3", result.stdout)

    def test_version_query_failure_reports_error(self):
        (self.legacy / "bin" / "exit-code.txt").write_text("2")
        (self.legacy / "bin" / "stderr.txt").write_text("probe failed\n")
        result = self.run_version_check()
        output = result.stdout + result.stderr
        self.assertEqual(result.returncode, 1, output)
        self.assertIn("[ERROR]", output)
        self.assertIn("probe failed", output)

    def test_missing_inspector_reports_error(self):
        (self.legacy / "bin" / "gst-inspect-1.0.exe").unlink()
        result = self.run_version_check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[ERROR]", result.stdout)
        self.assertIn("https://gstreamer.freedesktop.org/download/", result.stdout)

    def test_invalid_inspector_reports_error(self):
        (self.legacy / "bin" / "gst-inspect-1.0.exe").write_bytes(b"invalid executable")
        result = self.run_version_check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[ERROR]", result.stdout)
        self.assertIn("https://gstreamer.freedesktop.org/download/", result.stdout)

    def test_version_query_timeout_reports_error(self):
        (self.legacy / "bin" / "delay.txt").write_text("30000")
        result = self.run_version_check()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[ERROR]", result.stdout)
        self.assertIn("timed out", result.stdout)

    def test_minimum_version_agrees_with_meson(self):
        source = VERSION_CHECK.read_text(encoding="utf-8")
        minimum = re.search(r"\$minimumVersion\s*=\s*\[version\]'([^']+)'", source)
        self.assertIsNotNone(minimum)
        meson = (PROJECT_ROOT / "gst-dxstream-plugin" / "meson.build").read_text(
            encoding="utf-8"
        )
        dependency = re.search(
            r"'gstreamer-1\.0',\s*version\s*:\s*'>=([^']+)'", meson
        )
        self.assertIsNotNone(dependency)
        self.assertEqual(minimum.group(1), dependency.group(1))

    def test_installer_rejects_old_version_before_installation(self):
        source = (PROJECT_ROOT / "install.bat").read_text(encoding="utf-8")
        start = source.index("REM ---- GStreamer MSVC ----")
        end = source.index("echo [Phase 1] All prerequisites OK.")
        probe = self.root / "old-version-install-probe.bat"
        probe.write_text(
            '@echo off\nsetlocal EnableDelayedExpansion\nset "FAIL=0"\n'
            f'set "PROJECT_ROOT={PROJECT_ROOT}"\n'
            + source[start:end] + '\necho UNEXPECTED_SUCCESS\nexit /b 0\n'
        )
        (self.legacy / "bin" / "version.txt").write_text("GStreamer 1.16.2\n")
        env = os.environ.copy()
        env.pop(ROOT_VARIABLES[0], None)
        env[ROOT_VARIABLES[1]] = str(self.legacy)
        result = subprocess.run(
            [os.environ["ComSpec"], "/d", "/c", str(probe)],
            env=env, capture_output=True, text=True, timeout=20,
        )
        output = result.stdout + result.stderr
        self.assertEqual(result.returncode, 1, output)
        self.assertIn("1.16.2", output)
        self.assertIn("1.16.3", output)
        self.assertIn("https://gstreamer.freedesktop.org/download/", output)
        self.assertNotIn("UNEXPECTED_SUCCESS", output)

    def run_setup(self, modern, legacy, mode="runtime"):
        env = os.environ.copy()
        for name in ROOT_VARIABLES:
            env.pop(name, None)
        if modern is not None:
            env[ROOT_VARIABLES[0]] = str(modern)
        if legacy is not None:
            env[ROOT_VARIABLES[1]] = str(legacy)
        env["PATH"] = os.environ["SystemRoot"] + r"\System32"
        env["PKG_CONFIG_PATH"] = r"C:\existing-pkgconfig"
        script = self.root / "probe.bat"
        script.write_text(
            f'@echo off\ncall "{HELPER}" {mode}\n'
            'if errorlevel 1 exit /b %errorlevel%\n'
            'echo ROOT=%GST_ROOT%\n'
            'echo CANONICAL=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\n'
            'echo SEARCH=%PATH%\n'
            'echo PACKAGES=%PKG_CONFIG_PATH%\n'
            'exit /b 0\n'
        )
        return subprocess.run(
            [os.environ["ComSpec"], "/d", "/c", str(script)],
            env=env, capture_output=True, text=True, timeout=15,
        )

    def test_root_selection(self):
        cases = (
            ("modern", self.modern, None, self.modern),
            ("legacy", None, self.legacy, self.legacy),
            ("priority", self.modern, self.legacy, self.modern),
            ("stale-modern", self.root / "absent", self.legacy, self.legacy),
            ("trailing-slash", None, str(self.legacy) + "\\", self.legacy),
        )
        for mode in ("runtime", "development"):
            for name, modern, legacy, expected in cases:
                with self.subTest(mode=mode, case=name):
                    result = self.run_setup(modern, legacy, mode)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(f"ROOT={expected}\n", result.stdout)
                    self.assertIn(f"CANONICAL={expected}\n", result.stdout)
                    self.assertIn(f"SEARCH={expected}\\bin;", result.stdout)
                    if mode == "development":
                        self.assertIn(
                            f"PACKAGES={expected}\\lib\\pkgconfig;C:\\existing-pkgconfig",
                            result.stdout,
                        )
                    else:
                        self.assertIn("PACKAGES=C:\\existing-pkgconfig", result.stdout)

    def test_runtime_only_root_falls_back_for_development(self):
        runtime = self.make_sdk("Runtime Only", development=False)
        result = self.run_setup(runtime, self.legacy, "development")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(f"ROOT={self.legacy}\n", result.stdout)

    def test_invalid_mode_reports_error(self):
        result = self.run_setup(self.modern, self.legacy, "invalid")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("[ERROR]", result.stdout + result.stderr)

    def test_missing_pkg_config_stops_before_installation(self):
        source = (PROJECT_ROOT / "install.bat").read_text(encoding="utf-8")
        start = source.index("REM ---- GStreamer MSVC ----")
        end_marker = "echo [Phase 1] All prerequisites OK."
        end = source.index(end_marker) + len(end_marker)
        probe = self.root / "install-probe.bat"
        probe.write_text(
            '@echo off\nsetlocal EnableDelayedExpansion\nset "FAIL=0"\n'
            f'set "PROJECT_ROOT={PROJECT_ROOT}"\n'
            + source[start:end] + '\necho UNEXPECTED_SUCCESS\nexit /b 0\n'
        )
        env = os.environ.copy()
        env.pop(ROOT_VARIABLES[0], None)
        env[ROOT_VARIABLES[1]] = str(self.legacy)
        system32 = Path(os.environ["SystemRoot"]) / "System32"
        env["PATH"] = str(system32) + ";" + str(system32 / "WindowsPowerShell" / "v1.0")
        result = subprocess.run(
            [os.environ["ComSpec"], "/d", "/c", str(probe)],
            env=env, capture_output=True, text=True, timeout=15,
        )
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[OK] GStreamer", result.stdout)
        self.assertIn("[MISSING] pkg-config", result.stdout)
        self.assertIn("https://github.com/pkgconf/pkgconf/releases", result.stdout)
        self.assertNotIn("UNEXPECTED_SUCCESS", result.stdout)

    def test_powershell_runner_root_selection(self):
        source = (PROJECT_ROOT / "test" / "windows" / "run_test.ps1").read_text(
            encoding="utf-8"
        )
        block = source.split("# ---- GStreamer root ----", 1)[1].split("$installDir", 1)[0]
        cases = (
            (self.modern, None, self.modern),
            (None, self.legacy, self.legacy),
            (self.modern, self.legacy, self.modern),
            (self.root / "absent", self.legacy, self.legacy),
            (None, str(self.legacy) + "\\", self.legacy),
        )
        for modern, legacy, expected in cases:
            with self.subTest(modern=modern, legacy=legacy):
                env = os.environ.copy()
                for name in ROOT_VARIABLES:
                    env.pop(name, None)
                if modern is not None:
                    env[ROOT_VARIABLES[0]] = str(modern)
                if legacy is not None:
                    env[ROOT_VARIABLES[1]] = str(legacy)
                result = subprocess.run(
                    ["powershell", "-NoProfile", "-Command",
                     "$ErrorActionPreference='Stop';" + block + ';Write-Output "ROOT=$gstDir"'],
                    env=env, capture_output=True, text=True, timeout=15,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"ROOT={expected}\n", result.stdout)

    def test_all_batch_entrypoints_use_shared_discovery(self):
        scripts = [
            PROJECT_ROOT / "install.bat",
            PROJECT_ROOT / "build.bat",
            PROJECT_ROOT / "run_demo.bat",
            PROJECT_ROOT / "dx_stream" / "apps" / "build.bat",
            PROJECT_ROOT / "dx_stream" / "custom_library" / "build.bat",
            PROJECT_ROOT / "test" / "run_pydxs.bat",
        ]
        scripts.extend((PROJECT_ROOT / "dx_stream" / "pipelines" / "windows").glob("*.bat"))
        for script in scripts:
            with self.subTest(script=script.name):
                self.assertTrue(
                    "setup_gstreamer.bat" in script.read_text(encoding="utf-8"),
                    str(script),
                )

    def test_pipeline_headers_configure_legacy_root(self):
        env = os.environ.copy()
        env.pop(ROOT_VARIABLES[0], None)
        env[ROOT_VARIABLES[1]] = str(self.legacy)
        env["DXSTREAM_ROOT"] = str(PROJECT_ROOT)
        for pipeline in (PROJECT_ROOT / "dx_stream" / "pipelines" / "windows").glob("*.bat"):
            with self.subTest(pipeline=pipeline.name):
                source = pipeline.read_text(encoding="utf-8")
                header = source.split(
                    'if not exist "%DXSTREAM_ROOT%\\install\\bin\\gstdxstream.dll"', 1
                )[0]
                probe = self.root / "pipeline-probe.bat"
                probe.write_text(header + '\necho ROOT=%GST_ROOT%\nexit /b 0\n')
                result = subprocess.run(
                    [os.environ["ComSpec"], "/d", "/c", str(probe)],
                    cwd=PROJECT_ROOT, env=env, capture_output=True, text=True, timeout=15,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"ROOT={self.legacy}\n", result.stdout)


if __name__ == "__main__":
    unittest.main()
