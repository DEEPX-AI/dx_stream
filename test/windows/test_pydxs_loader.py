"""Windows DLL loader and cold-process pydxs import regression checks."""

import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import types
import unittest
from unittest import mock


PROJECT_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = PROJECT_ROOT / "bindings" / "python" / "pydxs" / "src" / "pydxs"
VENV_PYTHON = PROJECT_ROOT / "venv-dx_stream" / "Scripts" / "python.exe"
PUBLIC_API = {
    "DXFrameMeta", "DXObjectMeta", "DXUserMeta", "DXUserMetaType",
    "FRAME", "OBJECT", "dx_acquire_obj_meta_from_pool",
    "dx_add_obj_meta_to_frame", "dx_create_frame_meta",
    "dx_ensure_writable_and_create_meta", "dx_get_frame_meta",
    "dx_remove_obj_meta_from_frame", "writable_buffer",
}


@unittest.skipUnless(sys.platform == "win32", "requires Windows DLL loading")
class PydxsImportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not VENV_PYTHON.is_file():
            raise unittest.SkipTest("run build.bat to install the real pydxs binding")

    def run_import(self, cwd, overrides=None):
        env = os.environ.copy()
        for key in (
            "DX_STREAM_ROOT", "GSTREAMER_1_0_ROOT_MSVC_X86_64",
            "GSTREAMER_1_0_ROOT_X86_64", "DEEPX_SDK_DIR", "DEEPX_VNPU_DIR",
            "PYTHONPATH", "PYTHONHOME",
        ):
            env.pop(key, None)
        env["PATH"] = os.path.join(env["SYSTEMROOT"], "System32")
        env["PYTHONIOENCODING"] = "utf-8"
        for key in (
            "GST_PLUGIN_PATH", "GST_PLUGIN_PATH_1_0",
            "GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0",
        ):
            env[key] = ""
        env.update(overrides or {})
        code = (
            "import gc,json,sys; import pydxs; gc.collect(); "
            "obj=pydxs.dx_acquire_obj_meta_from_pool(); "
            "obj.label=42; obj.track_id=100; "
            "sys.stdout.write(json.dumps({'public':"
            "[n for n in dir(pydxs) if not n.startswith('_')],"
            "'label':obj.label,'track_id':obj.track_id,"
            "'module':pydxs.__file__}))"
        )
        with tempfile.TemporaryDirectory(prefix="dx-pydxs-registry-") as directory:
            env["GST_REGISTRY"] = str(Path(directory) / "registry.bin")
            return subprocess.run(
                [str(VENV_PYTHON), "-c", code], cwd=cwd, env=env,
                capture_output=True, text=True, encoding="utf-8", timeout=30,
            )

    def assert_import(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        data = json.loads(result.stdout)
        self.assertEqual(set(data["public"]), PUBLIC_API)
        self.assertEqual((data["label"], data["track_id"]), (42, 100))
        self.assertTrue(data["module"].endswith("__init__.py"), data["module"])

    def test_plain_import_without_sdk_path_or_environment(self):
        self.assert_import(self.run_import(PROJECT_ROOT))

    def test_plain_import_from_external_working_directory(self):
        with tempfile.TemporaryDirectory(prefix="dx pydxs external ") as directory:
            self.assert_import(self.run_import(directory))

    def test_import_from_binding_build_directory(self):
        self.assert_import(self.run_import(PROJECT_ROOT / "bindings" / "python" / "pydxs"))

    def test_previous_bare_extension_is_not_reinstalled(self):
        site_packages = VENV_PYTHON.parents[1] / "Lib" / "site-packages"
        self.assertEqual(list(site_packages.glob("pydxs*.pyd")), [])

    def test_invalid_explicit_checkout_has_actionable_error(self):
        with tempfile.TemporaryDirectory(prefix="dx pydxs missing ") as directory:
            result = self.run_import(directory, {"DX_STREAM_ROOT": directory})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("DX_STREAM_ROOT", result.stderr)
        self.assertIn("gstdxstream.dll", result.stderr)

    def test_missing_transitive_dlls_has_native_loading_guidance(self):
        with tempfile.TemporaryDirectory(prefix="dx pydxs dependencies ") as directory:
            root = Path(directory)
            plugin = root / "install" / "lib" / "gstreamer-1.0"
            plugin.mkdir(parents=True)
            (root / "install" / "bin").mkdir()
            sdk = root / "SDK"
            (sdk / "bin").mkdir(parents=True)
            shutil.copyfile(
                PROJECT_ROOT / "install" / "lib" / "gstreamer-1.0" / "gstdxstream.dll",
                plugin / "gstdxstream.dll",
            )
            result = self.run_import(root, {
                "DX_STREAM_ROOT": str(root), "DEEPX_SDK_DIR": str(sdk),
            })
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("could not load the native binding", result.stderr)
        self.assertIn("DLL directories:", result.stderr)
        self.assertIn("build.bat", result.stderr)


@unittest.skipUnless(sys.platform == "win32", "requires Windows DLL loading")
class DllDirectoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location(
            "pydxs_windows_loader_tests", PACKAGE / "_windows.py",
        )
        cls.loader = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.loader)

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="dx pydxs paths ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.prefix = self.make_prefix("built checkout")
        self.gst = self.make_gstreamer("GStreamer SDK")
        self.sdk = self.root / "DEEPX SDK"
        (self.sdk / "bin").mkdir(parents=True)
        self.paths = {
            "install_prefix": str(self.prefix),
            "gstreamer_root": str(self.gst),
            "deepx_sdk_dir": str(self.sdk),
            "deepx_vnpu_dir": "",
        }
        self.environment = {}
        self.environment_patch = mock.patch.dict(os.environ, self.environment, clear=True)
        self.environment_patch.start()
        self.addCleanup(self.environment_patch.stop)
        for target, value in (
            ("__file__", str(self.root / "external" / "pydxs" / "_windows.py")),
            ("DEFAULT_GSTREAMER_ROOTS", ()),
        ):
            patch = mock.patch.object(self.loader, target, value)
            patch.start()
            self.addCleanup(patch.stop)
        patch = mock.patch.object(sys, "prefix", str(self.root / "external-venv"))
        patch.start()
        self.addCleanup(patch.stop)

    def make_prefix(self, name):
        prefix = self.root / name / "install"
        (prefix / "lib" / "gstreamer-1.0").mkdir(parents=True)
        (prefix / "lib" / "gstreamer-1.0" / "gstdxstream.dll").touch()
        (prefix / "bin").mkdir()
        return prefix

    def make_gstreamer(self, name):
        root = self.root / name
        (root / "bin").mkdir(parents=True)
        for executable in ("gst-inspect-1.0.exe", "gst-launch-1.0.exe"):
            (root / "bin" / executable).touch()
        return root

    def directories(self):
        return self.loader.dll_directories(self.paths)

    def test_recorded_paths_support_external_venv(self):
        self.assertEqual(self.directories(), [
            str(self.prefix / "lib" / "gstreamer-1.0"),
            str(self.prefix / "bin"), str(self.gst / "bin"), str(self.sdk / "bin"),
        ])

    def test_explicit_checkout_overrides_recorded_checkout(self):
        selected = self.make_prefix("selected checkout")
        os.environ["DX_STREAM_ROOT"] = str(selected.parent)
        directories = self.directories()
        self.assertEqual(directories[0], str(selected / "lib" / "gstreamer-1.0"))
        self.assertNotIn(str(self.prefix / "bin"), directories)

    def test_current_venv_checkout_overrides_recorded_checkout(self):
        selected = self.make_prefix("current checkout")
        with mock.patch.object(sys, "prefix", str(selected.parent / "venv-dx_stream")):
            self.assertEqual(
                self.directories()[0], str(selected / "lib" / "gstreamer-1.0"),
            )

    def test_invalid_explicit_checkout_does_not_load_another_checkout(self):
        os.environ["DX_STREAM_ROOT"] = str(self.root / "missing")
        with self.assertRaisesRegex(ImportError, "DX_STREAM_ROOT.*gstdxstream.dll"):
            self.directories()

    def test_relative_explicit_checkout_is_rejected(self):
        os.environ["DX_STREAM_ROOT"] = "relative-checkout"
        with self.assertRaisesRegex(ImportError, "absolute"):
            self.directories()

    def test_modern_gstreamer_overrides_legacy_and_recorded_sdk(self):
        modern = self.make_gstreamer("modern")
        legacy = self.make_gstreamer("legacy")
        os.environ["GSTREAMER_1_0_ROOT_MSVC_X86_64"] = '"' + str(modern) + '\\\\"'
        os.environ["GSTREAMER_1_0_ROOT_X86_64"] = str(legacy)
        directories = self.directories()
        self.assertIn(str(modern / "bin"), directories)
        self.assertNotIn(str(legacy / "bin"), directories)
        self.assertNotIn(str(self.gst / "bin"), directories)

    def test_invalid_modern_gstreamer_falls_back_to_legacy(self):
        legacy = self.make_gstreamer("legacy")
        os.environ["GSTREAMER_1_0_ROOT_MSVC_X86_64"] = str(self.root / "missing")
        os.environ["GSTREAMER_1_0_ROOT_X86_64"] = str(legacy)
        self.assertIn(str(legacy / "bin"), self.directories())

    def test_missing_gstreamer_has_install_guidance(self):
        self.paths["gstreamer_root"] = ""
        with self.assertRaisesRegex(ImportError, "GSTREAMER_1_0_ROOT_MSVC_X86_64"):
            self.directories()

    def test_explicit_sdk_overrides_recorded_sdk(self):
        selected = self.root / "selected DEEPX SDK"
        (selected / "bin").mkdir(parents=True)
        os.environ["DEEPX_SDK_DIR"] = str(selected)
        self.assertIn(str(selected / "bin"), self.directories())
        self.assertNotIn(str(self.sdk / "bin"), self.directories())

    def test_missing_explicit_sdk_is_not_silently_replaced(self):
        os.environ["DEEPX_SDK_DIR"] = str(self.root / "missing")
        with self.assertRaisesRegex(ImportError, "DEEPX_SDK_DIR"):
            self.directories()

    def test_optional_vnpu_and_duplicate_directories(self):
        self.paths["deepx_sdk_dir"] = str(self.gst)
        self.paths["deepx_vnpu_dir"] = str(self.sdk)
        directories = self.directories()
        self.assertEqual(directories.count(str(self.gst / "bin")), 1)
        self.assertIn(str(self.sdk / "bin"), directories)

    def test_unrelated_path_directories_are_not_registered(self):
        unrelated = self.make_prefix("unrelated checkout")
        os.environ["PATH"] = str(unrelated / "bin")
        self.assertNotIn(str(unrelated / "bin"), self.directories())

    def test_registration_handles_are_returned_and_kept_open(self):
        with mock.patch.object(self.loader.os, "add_dll_directory") as register:
            handles, directories = self.loader.register_dll_directories(self.paths)
        self.assertEqual(register.call_count, len(directories))
        self.assertEqual(handles, [register.return_value] * len(directories))
        register.return_value.close.assert_not_called()

    def test_registration_failure_closes_previously_opened_handles(self):
        handle = mock.Mock()
        with mock.patch.object(
            self.loader.os, "add_dll_directory",
            side_effect=[handle, OSError("registration denied")],
        ):
            with self.assertRaisesRegex(ImportError, "registration denied") as failure:
                self.loader.register_dll_directories(self.paths)
        self.assertIsInstance(failure.exception.__cause__, OSError)
        handle.close.assert_called_once_with()

    def test_legacy_python_preserves_path_based_loading(self):
        os.environ["PATH"] = "existing-path"
        with mock.patch.object(self.loader.os, "add_dll_directory", None), \
                mock.patch.object(sys, "version_info", (3, 7, 0)):
            handles, directories = self.loader.register_dll_directories(self.paths)
        self.assertEqual(handles, [])
        self.assertEqual(os.environ["PATH"], os.pathsep.join(directories + ["existing-path"]))

    def test_modern_python_without_registration_api_is_an_error(self):
        with mock.patch.object(self.loader.os, "add_dll_directory", None):
            with self.assertRaisesRegex(ImportError, "os.add_dll_directory"):
                self.loader.register_dll_directories(self.paths)


@unittest.skipUnless(sys.platform == "win32", "requires Windows package")
class PackageLoaderTests(unittest.TestCase):
    def load_package(self, native=None, error=None):
        name = "pydxs_package_loader_test"
        spec = importlib.util.spec_from_file_location(
            name, PACKAGE / "__init__.py", submodule_search_locations=[str(PACKAGE)],
        )
        package = importlib.util.module_from_spec(spec)
        configuration = types.ModuleType(name + "._runtime_paths")
        configuration.RUNTIME_PATHS = {}
        windows = types.ModuleType(name + "._windows")
        self.handle = mock.Mock()
        windows.register_dll_directories = mock.Mock(return_value=(
            [self.handle], [r"C:\selected SDK\bin"],
        ))
        modules = {
            name: package, name + "._runtime_paths": configuration,
            name + "._windows": windows,
        }
        with mock.patch.dict(sys.modules, modules), mock.patch(
            "importlib.import_module", return_value=native, side_effect=error,
        ):
            spec.loader.exec_module(package)
        return package

    def test_exports_native_api_and_preserves_documentation(self):
        native = types.ModuleType("_pydxs", "Native API documentation")
        native.DXFrameMeta = object()
        native._private = object()
        package = self.load_package(native=native)
        self.assertIs(package.DXFrameMeta, native.DXFrameMeta)
        self.assertEqual(package.__all__, ["DXFrameMeta"])
        self.assertEqual(package.__doc__, native.__doc__)
        self.assertEqual(package._dll_handles, [self.handle])
        self.handle.close.assert_not_called()

    def test_native_load_failure_reports_directories_and_releases_handles(self):
        original = ImportError("missing dependent.dll")
        with self.assertRaisesRegex(ImportError, "missing dependent.dll") as failure:
            self.load_package(error=original)
        self.assertIs(failure.exception.__cause__, original)
        self.assertIn(r"C:\selected SDK\bin", str(failure.exception))
        self.assertIn("build.bat", str(failure.exception))
        self.handle.close.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
