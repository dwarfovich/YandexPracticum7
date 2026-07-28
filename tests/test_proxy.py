import socket
import subprocess
import sys
import time
from pathlib import Path

PROXY_HOST = "127.0.0.1"
PROXY_PORT = 12345

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 9000

PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROXY_EXE = PROJECT_ROOT / "out" / "build" / "x64-Debug" / "AsyncHttpProxy.exe"
HTTP_SERVER = PROJECT_ROOT / "tests" / "http_server.py"


def wait_port(host, port, timeout=5):
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.1)

    raise RuntimeError(f"{host}:{port} didn't start")


def main():
    http_server = None
    proxy = None

    try:
        http_server = subprocess.Popen([
            sys.executable,
            "http_server.py"
        ])

        wait_port(SERVER_HOST, SERVER_PORT)
        print(f"HTTP server started on {SERVER_HOST}:{SERVER_PORT}")

        proxy = subprocess.Popen([
            str(PROXY_EXE),
            str(PROXY_PORT)
        ])

        time.sleep(0.2)

        #wait_port(PROXY_HOST, PROXY_PORT)
        print("HTTP server PID:", http_server.pid)
        print("HTTP server running:", http_server.poll() is None)
        print(f"Proxy started on {PROXY_HOST}:{PROXY_PORT}")
        print("HTTP server running before wget:", http_server.poll() is None)
        
        result = subprocess.run(
            [
                "wget2",
                "--tries=1",
                 "--header=Connection: close",
                "-O", "-",
                "--proxy=on",
                f"--execute=http_proxy=http://{PROXY_HOST}:{PROXY_PORT}",
                f"http://{SERVER_HOST}:{SERVER_PORT}/"
            ],
            capture_output=True,
            text=True
        )

        print()
        print("wget stdout:")
        print(result.stdout)

        print("wget stderr:")
        print(result.stderr)

        assert result.returncode == 0, (
            f"wget failed with exit code {result.returncode}"
        )

        assert "Hello from test server!" in result.stdout

        print("TEST PASSED")

    finally:
        if proxy is not None:
            proxy.terminate()
            try:
                proxy.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proxy.kill()
                proxy.wait()

        if http_server is not None:
            http_server.terminate()
            try:
                http_server.wait(timeout=5)
            except subprocess.TimeoutExpired:
                http_server.kill()
                http_server.wait()


if __name__ == "__main__":
    main()