"""Network-isolated Windows setup tests using real curl and tar."""

from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import io
import json
import logging
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import threading
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
LOGGER = logging.getLogger(__name__)


@unittest.skipUnless(sys.platform == "win32", "requires Windows CMD, curl and tar")
class SetupScriptTests(unittest.TestCase):
    def setUp(self):
        for tool in ("curl.exe", "tar.exe", "powershell.exe"):
            if not shutil.which(tool):
                self.fail(f"Required Windows test tool not found: {tool}")
        temporary = tempfile.TemporaryDirectory(prefix="dx setup ")
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        self.checkout = root / "checkout"
        self.checkout.mkdir()
        self.web = root / "server"
        self.web.mkdir()
        self.requests = []
        requests = self.requests

        class Handler(SimpleHTTPRequestHandler):
            def do_GET(self):
                requests.append(self.path)
                super().do_GET()

            def log_message(self, message, *args):
                LOGGER.debug(message, *args)

        self.server = ThreadingHTTPServer(
            ("127.0.0.1", 0), partial(Handler, directory=str(self.web))
        )
        thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        thread.start()

        def stop_server():
            self.server.shutdown()
            self.server.server_close()
            thread.join(timeout=5)
            self.assertFalse(thread.is_alive(), "HTTP fixture did not stop")

        self.addCleanup(stop_server)
        base_url = f"http://127.0.0.1:{self.server.server_port}"
        self.names = ("yolov5-s_640x640.dxnn", "yolo26-n_640x640.dxnn")
        self.model_payloads = {
            name: f"download fixture for {name}\n".encode() for name in self.names
        }
        self.remote_models = self.web / "models" / "fixture"
        self.remote_models.mkdir(parents=True)
        for name, content in self.model_payloads.items():
            (self.remote_models / name).write_bytes(content)
        self.video_payload = b"video download fixture\n"
        self.archive = self.web / "sample_videos.tar.gz"
        with tarfile.open(self.archive, "w:gz") as archive:
            entry = tarfile.TarInfo("sample_videos/clip.mp4")
            entry.size = len(self.video_payload)
            archive.addfile(entry, io.BytesIO(self.video_payload))
        (self.checkout / "model_list.json").write_text(
            json.dumps({"version": "fixture", "models": self.names}),
            encoding="utf-8",
        )
        source = (PROJECT_ROOT / "setup.bat").read_text(encoding="utf-8")
        model_url = 'set "BASE_URL=https://sdk.deepx.ai/modelzoo/q-lite-dxnn"'
        video_url = 'set "VIDEO_URL=https://sdk.deepx.ai/res/video/sample_videos.tar.gz"'
        self.assertEqual(source.count(model_url), 1)
        self.assertEqual(source.count(video_url), 1)
        source = source.replace(model_url, f'set "BASE_URL={base_url}/models"')
        source = source.replace(video_url, f'set "VIDEO_URL={base_url}/sample_videos.tar.gz"')
        self.script = self.checkout / "setup.bat"
        self.script.write_text(source, encoding="utf-8")
        self.models = self.checkout / "dx_stream" / "samples" / "models"
        self.videos = self.checkout / "dx_stream" / "samples" / "videos"

    def run_setup(self, arguments="", *, entry=None):
        script = entry or self.script
        command = f'"{os.environ["ComSpec"]}" /d /s /c ""{script}" {arguments}"'
        env = os.environ.copy()
        for variable in ("HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY"):
            env.pop(variable, None)
        env["NO_PROXY"] = "127.0.0.1"
        return subprocess.run(
            command, cwd=self.checkout, env=env,
            capture_output=True, text=True, timeout=30,
        )

    def assert_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Setup complete.", result.stdout)
        self.assertNotIn("unexpected at this time", result.stdout + result.stderr)

    def test_download_all_models_and_extract_video(self):
        result = self.run_setup()
        self.assert_success(result)
        for name, content in self.model_payloads.items():
            self.assertEqual((self.models / name).read_bytes(), content)
        self.assertEqual((self.videos / "clip.mp4").read_bytes(), self.video_payload)
        self.assertEqual(len(self.requests), 3)

    def test_single_model_argument_forms(self):
        name = self.names[0]
        for arguments in (f"--model={name}", f"--model {name}", f'"--model={name}"'):
            with self.subTest(arguments=arguments):
                result = self.run_setup(arguments + " --force")
                self.assert_success(result)
                self.assertEqual((self.models / name).read_bytes(), self.model_payloads[name])
                self.assertFalse((self.models / self.names[1]).exists())
                self.assertFalse(self.videos.exists())
        self.assertEqual(self.requests, [f"/models/fixture/{name}"] * 3)

    def test_existing_downloads_are_skipped(self):
        self.assert_success(self.run_setup())
        requests = list(self.requests)
        result = self.run_setup()
        self.assert_success(result)
        self.assertIn("0 downloaded, 2 skipped, 0 failed", result.stdout)
        self.assertIn("[SKIP] Videos already exist", result.stdout)
        self.assertEqual(self.requests, requests)

    def test_force_redownloads_models_and_videos(self):
        self.assert_success(self.run_setup())
        name = self.names[0]
        updated = b"updated download fixture\n"
        (self.remote_models / name).write_bytes(updated)
        result = self.run_setup("--force")
        self.assert_success(result)
        self.assertEqual((self.models / name).read_bytes(), updated)
        self.assertEqual((self.videos / "clip.mp4").read_bytes(), self.video_payload)
        self.assertEqual(len(self.requests), 6)

    def test_model_download_failure_returns_nonzero_and_removes_file(self):
        name = self.names[0]
        (self.remote_models / name).unlink()
        result = self.run_setup(f"--model={name}")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(f"[FAIL] {name}", result.stdout)
        self.assertIn("WARNING: 1 model downloads failed.", result.stdout)
        self.assertFalse((self.models / name).exists())
        self.assertNotIn("unexpected at this time", result.stdout + result.stderr)

    def test_multiple_model_failures_preserve_failure_count(self):
        for name in self.names:
            (self.remote_models / name).unlink()
        result = self.run_setup()
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertIn("WARNING: 2 model downloads failed.", result.stdout)
        self.assertEqual((self.videos / "clip.mp4").read_bytes(), self.video_payload)

    def test_video_download_failure_returns_nonzero(self):
        self.archive.unlink()
        result = self.run_setup()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[FAIL] Video download failed.", result.stdout)
        self.assertFalse((self.checkout / "download" / "sample_videos.tar.gz").exists())

    def test_video_extraction_failure_returns_nonzero(self):
        self.archive.write_bytes(b"not a gzip archive")
        result = self.run_setup()
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[FAIL] Video extraction failed.", result.stdout)

    def test_missing_model_value_is_rejected_without_downloads(self):
        for arguments in ("--model", "--model=", '"--model="',
                          "--model --force", "--model= --help"):
            with self.subTest(arguments=arguments):
                result = self.run_setup(arguments)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertIn("[ERROR] --model requires a non-empty model name.", result.stdout)
                self.assertFalse(self.models.exists())
                self.assertEqual(self.requests, [])

    def test_unknown_option_is_rejected_without_downloads(self):
        result = self.run_setup("--unknown")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("[ERROR] Unknown option:", result.stdout)
        self.assertEqual(self.requests, [])

    def test_help_does_not_download(self):
        result = self.run_setup("--help")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("--model <name>", result.stdout)
        self.assertIn("--model=<name>", result.stdout)
        self.assertIn('"--model=<name>"', result.stdout)
        self.assertNotIn("^", result.stdout)
        self.assertEqual(self.requests, [])

    def test_calling_batch_resumes_and_receives_exit_code(self):
        caller = self.checkout / "caller.bat"
        caller.write_text(
            f'@echo off\ncall "{self.script}" --model={self.names[0]} --force\n'
            'set "RESULT=%ERRORLEVEL%"\necho CALL_RETURN=%RESULT%\n'
            'exit /b %RESULT%\n'
        )
        for expected in (0, 1):
            with self.subTest(exit_code=expected):
                if expected:
                    (self.remote_models / self.names[0]).unlink()
                result = self.run_setup(entry=caller)
                self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
                self.assertIn(f"CALL_RETURN={expected}", result.stdout)
                self.assertNotIn("unexpected at this time", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
