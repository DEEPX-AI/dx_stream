"""Isolated user-environment and Windows uninstall regression checks."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
HELPER = PROJECT_ROOT / "scripts" / "manage_build_environment.ps1"
POWERSHELL = (
    Path(os.environ.get("SYSTEMROOT", r"C:\Windows"))
    / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
)


@unittest.skipUnless(sys.platform == "win32", "requires Windows PowerShell")
class BuildEnvironmentTests(unittest.TestCase):
    def run_case(self, values, operations, *, fail_name="", env=None):
        with tempfile.TemporaryDirectory(prefix="dx environment tests ") as directory:
            root = Path(directory)
            data = root / "case.json"
            data.write_text(json.dumps({
                "helper": str(HELPER), "values": values,
                "operations": operations, "fail_name": fail_name,
            }), encoding="utf-8")
            script = root / "case.ps1"
            script.write_text(
                """param([string]$CaseFile)
$ErrorActionPreference = 'Stop'
$case = Get-Content -LiteralPath $CaseFile -Raw | ConvertFrom-Json
. $case.helper -Mode Unregister -InstallPrefix 'C:\\fixture\\install'
$script:Values = @{}
foreach ($property in $case.values.PSObject.Properties) {
    $script:Values[$property.Name] = $property.Value
}
$script:Writes = New-Object 'System.Collections.Generic.List[object]'
$script:Notifications = 0
function Get-UserEnvironmentValue([string]$Name) { return $script:Values[$Name] }
function Set-UserEnvironmentValue([string]$Name, [AllowNull()]$Value) {
    if ($Name -eq $case.fail_name) { throw 'simulated registry write failure' }
    $script:Writes.Add(@{ name = $Name; value = $Value })
    if ($null -eq $Value) { $script:Values.Remove($Name) }
    else { $script:Values[$Name] = $Value }
}
function Send-EnvironmentChangeNotification { $script:Notifications += 1 }
$status = 0
$message = ''
try {
    foreach ($operation in $case.operations) {
        Update-BuildEnvironment -Mode $operation.mode -InstallPrefix $operation.prefix
    }
} catch {
    $status = 1
    $message = $_.Exception.Message
}
$result = @{ values = $script:Values; writes = $script:Writes.ToArray();
             notifications = $script:Notifications; error = $message }
[Console]::WriteLine('RESULT_JSON:' + ($result | ConvertTo-Json -Depth 6 -Compress))
exit $status
""",
                encoding="utf-8",
            )
            result = subprocess.run(
                [str(POWERSHELL), "-NoProfile", "-ExecutionPolicy", "Bypass",
                 "-File", str(script), "-CaseFile", str(data)],
                capture_output=True, text=True, timeout=30,
                env=env or os.environ.copy(),
            )
        lines = [line for line in result.stdout.splitlines() if line.startswith("RESULT_JSON:")]
        self.assertEqual(len(lines), 1, result.stdout + result.stderr)
        return result, json.loads(lines[0][len("RESULT_JSON:"):])

    def operation(self, mode="Unregister", prefix=r"C:\DX owner's checkout\install"):
        return {"mode": mode, "prefix": prefix}

    def entries(self, prefix):
        return [
            prefix + r"\bin", prefix + r"\share\gstdxstream\lib",
            prefix + r"\share\gstdxstream\bin", prefix + r"\lib\gstreamer-1.0",
        ]

    def test_unregister_preserves_other_checkout_and_sdk_variables(self):
        prefix = self.operation()["prefix"]
        other = r"D:\other checkout\install"
        values = {
            "Path": ";".join(self.entries(prefix) + self.entries(other)),
            "GST_PLUGIN_PATH": prefix + r"\lib\gstreamer-1.0;" + other + r"\lib\gstreamer-1.0",
            "DEEPX_SDK_DIR": r"C:\shared SDK",
            "GSTREAMER_1_0_ROOT_MSVC_X86_64": r"C:\shared GStreamer",
        }
        result, data = self.run_case(values, [self.operation()])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"]["Path"], ";".join(self.entries(other)))
        self.assertEqual(data["values"]["GST_PLUGIN_PATH"], other + r"\lib\gstreamer-1.0")
        self.assertEqual(data["values"]["DEEPX_SDK_DIR"], values["DEEPX_SDK_DIR"])
        self.assertEqual(data["values"]["GSTREAMER_1_0_ROOT_MSVC_X86_64"],
                         values["GSTREAMER_1_0_ROOT_MSVC_X86_64"])
        self.assertEqual({write["name"] for write in data["writes"]},
                         {"Path", "GST_PLUGIN_PATH"})
        self.assertEqual(data["notifications"], 1)

    def test_only_owned_values_are_deleted_when_empty(self):
        prefix = self.operation()["prefix"]
        result, data = self.run_case({
            "Path": ";".join(self.entries(prefix)),
            "GST_PLUGIN_PATH": prefix + r"\lib\gstreamer-1.0",
        }, [self.operation()])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("Path", data["values"])
        self.assertNotIn("GST_PLUGIN_PATH", data["values"])
        self.assertTrue(all(write["value"] is None for write in data["writes"]))

    def test_path_matching_handles_case_quotes_slashes_and_duplicates(self):
        prefix = self.operation()["prefix"]
        owned = '"' + (prefix + r"\bin").upper().replace("\\", "/") + '/"'
        plugin = ' "' + prefix.upper() + '\\LIB\\GSTREAMER-1.0\\\\" '
        values = {"Path": owned + ";" + prefix + r"\bin;C:\keep;;C:\keep;",
                  "GST_PLUGIN_PATH": plugin + r";D:\keep"}
        result, data = self.run_case(values, [self.operation()])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"]["Path"], r"C:\keep;;C:\keep;")
        self.assertEqual(data["values"]["GST_PLUGIN_PATH"], r"D:\keep")

    def test_unrelated_values_are_not_rewritten(self):
        values = {"Path": r"C:\keep;;C:\keep;", "GST_PLUGIN_PATH": r"D:\plugin"}
        result, data = self.run_case(values, [self.operation()])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"], values)
        self.assertEqual(data["writes"], [])
        self.assertEqual(data["notifications"], 0)

    def test_absent_values_and_repeated_uninstall_are_noops(self):
        result, data = self.run_case({}, [self.operation(), self.operation()])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["writes"], [])
        self.assertEqual(data["notifications"], 0)

    def test_register_preserves_other_plugins_and_is_idempotent(self):
        prefix = self.operation()["prefix"]
        values = {"Path": r"%USERPROFILE%\bin;C:\keep", "GST_PLUGIN_PATH": r"D:\plugin"}
        result, data = self.run_case(values, [
            self.operation("Register"), self.operation("Register"),
        ])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"]["Path"],
                         ";".join(self.entries(prefix)) + ";" + values["Path"])
        self.assertEqual(data["values"]["GST_PLUGIN_PATH"],
                         prefix + r"\lib\gstreamer-1.0;D:\plugin")
        self.assertEqual(len(data["writes"]), 2)
        self.assertEqual(data["notifications"], 1)

    def test_register_and_unregister_two_checkouts_preserve_each_other(self):
        first = r"C:\first checkout\install"
        second = r"D:\second checkout\install"
        result, data = self.run_case({"Path": r"C:\keep"}, [
            self.operation("Register", first), self.operation("Register", second),
            self.operation("Unregister", first),
        ])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"]["Path"], ";".join(self.entries(second)) + r";C:\keep")
        self.assertEqual(data["values"]["GST_PLUGIN_PATH"], second + r"\lib\gstreamer-1.0")

    def test_expanded_matching_keeps_unrelated_variable_tokens(self):
        env = os.environ.copy()
        env["DX_ENV_FIXTURE_ROOT"] = r"C:\DX owner's checkout"
        values = {"Path": r"%DX_ENV_FIXTURE_ROOT%\install\bin;%USERPROFILE%\bin"}
        result, data = self.run_case(values, [self.operation()], env=env)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["values"]["Path"], r"%USERPROFILE%\bin")

    def test_registry_failure_is_reported(self):
        result, data = self.run_case({}, [self.operation("Register")], fail_name="Path")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("registry write failure", data["error"])
        self.assertEqual(data["notifications"], 1)

    def test_invalid_prefix_is_rejected_before_any_writes(self):
        for prefix in ("relative-path", "C:relative-path", r"\relative-path",
                       "C:\\", r"C:\path;separator\install"):
            with self.subTest(prefix=prefix):
                result, data = self.run_case({}, [self.operation(prefix=prefix)])
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(data["writes"], [])


@unittest.skipUnless(sys.platform == "win32", "requires Windows CMD")
class UninstallBatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        temporary = tempfile.TemporaryDirectory(prefix="dx-uninstall-tools-")
        cls.addClassCleanup(temporary.cleanup)
        cls.tools = Path(temporary.name)
        source = cls.tools / "compile.ps1"
        source.write_text(
            """$source = @'
using System;
using System.IO;
public static class FakeEnvironmentCommand {
    public static int Main(string[] args) {
        string name = Path.GetFileNameWithoutExtension(
            System.Reflection.Assembly.GetExecutingAssembly().Location);
        string log = Environment.GetEnvironmentVariable("DX_ENV_TEST_CALL_LOG");
        File.AppendAllText(log, name + "\\t" + String.Join("\\t", args) + "\\n");
        if (name.Equals("reg", StringComparison.OrdinalIgnoreCase)) return 1;
        if (name.Equals("setx", StringComparison.OrdinalIgnoreCase)) return 0;
        int file = Array.IndexOf(args, "-File");
        if (file >= 0 && !File.Exists(args[file + 1])) return 2;
        return Int32.Parse(Environment.GetEnvironmentVariable("DX_ENV_TEST_EXIT"));
    }
}
'@
Add-Type -TypeDefinition $source -OutputType ConsoleApplication -OutputAssembly """
            + "'" + str(cls.tools / "powershell.exe").replace("'", "''") + "'\n",
            encoding="utf-8",
        )
        result = subprocess.run(
            [str(POWERSHELL), "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(source)],
            capture_output=True, text=True, timeout=30,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stdout + result.stderr)
        shutil.copyfile(cls.tools / "powershell.exe", cls.tools / "reg.exe")
        shutil.copyfile(cls.tools / "powershell.exe", cls.tools / "setx.exe")

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="dx uninstall owner's checkout ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.script = self.root / "build.bat"
        shutil.copyfile(PROJECT_ROOT / "build.bat", self.script)
        if HELPER.exists():
            (self.root / "scripts").mkdir()
            shutil.copyfile(HELPER, self.root / "scripts" / HELPER.name)
        self.log = self.root / "commands.log"

    def run_uninstall(self, status=0):
        env = os.environ.copy()
        env["PATH"] = str(self.tools) + ";" + str(Path(env["SYSTEMROOT"]) / "System32")
        env["DX_ENV_TEST_EXIT"] = str(status)
        env["DX_ENV_TEST_CALL_LOG"] = str(self.log)
        command = f'"{os.environ["COMSPEC"]}" /d /s /c ""{self.script}" --uninstall"'
        return subprocess.run(
            command, cwd=self.root, env=env,
            capture_output=True, text=True, timeout=30,
        )

    def test_environment_failure_returns_nonzero_without_false_success(self):
        result = self.run_uninstall(status=7)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("[OK] Uninstall complete", result.stdout)
        result = self.run_uninstall()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def run_registration(self, status=0):
        source = (PROJECT_ROOT / "build.bat").read_text(encoding="utf-8")
        self.script.write_text(
        '@echo off\nsetlocal EnableDelayedExpansion\n'
        'set "PROJECT_ROOT=%~dp0"\nset "PREFIX=%~dp0install"\n'
        'call :setup_env\n'
        'if errorlevel 1 exit /b 1\nexit /b 0\n' + source,
        encoding="utf-8",
        )
        return self.run_uninstall(status=status)

    def test_registration_uses_shared_helper_with_quoted_paths(self):
        result = self.run_registration()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        arguments = self.log.read_text(encoding="utf-8").strip().split("\t")
        self.assertEqual(arguments[0], "powershell")
        self.assertEqual(Path(arguments[arguments.index("-File") + 1]),
                     self.root / "scripts" / HELPER.name)
        self.assertEqual(arguments[arguments.index("-Mode") + 1], "Register")
        self.assertEqual(Path(arguments[arguments.index("-InstallPrefix") + 1]),
                     self.root / "install")

    def test_registration_failure_is_not_reported_as_success(self):
        result = self.run_registration(status=7)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("Environment variables registered.", result.stdout)

    def test_locked_install_file_returns_nonzero(self):
        install = self.root / "install"
        install.mkdir()
        with (install / "locked.dll").open("wb") as locked:
            locked.write(b"uninstall lock fixture")
            locked.flush()
            result = self.run_uninstall()
            self.assertTrue(install.exists())
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn("[OK] Uninstall complete", result.stdout)
        result = self.run_uninstall()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse(install.exists())

    def test_removes_only_owned_artifacts_and_supports_repeat(self):
        artifacts = [
            "install", "gst-dxstream-plugin\\builddir", "venv-dx_stream",
            "dx_stream\\apps\\sample\\builddir",
            "dx_stream\\custom_library\\postprocess_library\\sample\\builddir",
            "dx_stream\\custom_library\\message_convert_library\\dx_msgconvl\\builddir",
            "bindings\\python\\pydxs\\build", "bindings\\python\\pydxs\\dist",
            "bindings\\python\\pydxs\\pydxs.egg-info",
            "bindings\\python\\pydxs\\src\\pydxs.egg-info",
        ]
        for relative in artifacts:
            directory = self.root / relative
            directory.mkdir(parents=True)
            (directory / "artifact.txt").write_text("owned build artifact", encoding="ascii")
        source = self.root / "dx_stream" / "apps" / "sample" / "source.cpp"
        source.write_text("int main() { return 0; }\n", encoding="ascii")
        for _ in range(2):
            result = self.run_uninstall()
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("[OK] Uninstall complete", result.stdout)
        for relative in artifacts:
            self.assertFalse((self.root / relative).exists(), relative)
        self.assertTrue(source.exists())
        self.assertIn("Unregister", self.log.read_text(encoding="utf-8"))

    def test_missing_environment_helper_is_a_failure(self):
        helper = self.root / "scripts" / HELPER.name
        if helper.exists():
            helper.unlink()
        result = self.run_uninstall()
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
