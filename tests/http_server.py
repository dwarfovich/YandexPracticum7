from http.server import BaseHTTPRequestHandler, HTTPServer

class TestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        body = b"Hello from test server!"

        self.send_response(200)

        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("X-Test-Header", "proxy-test")
        self.end_headers()

        print("Sending response", flush=True)
        self.wfile.write(body)
        self.wfile.flush()


    def log_message(self, format, *args):
        # отключаем логирование
        pass


if __name__ == "__main__":
    server = HTTPServer(("127.0.0.1", 9000), TestHandler)

    print("Test HTTP server started on port 9000")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass