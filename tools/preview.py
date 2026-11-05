"""Local-only companion preview; avoid reverse-DNS stalls on Windows."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from socketserver import TCPServer


class LocalServer(ThreadingHTTPServer):
    def server_bind(self):
        TCPServer.server_bind(self)
        self.server_name = "localhost"
        self.server_port = self.server_address[1]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8766)
    args = parser.parse_args()
    handler = partial(SimpleHTTPRequestHandler, directory=str(Path(__file__).resolve().parents[1] / "web"))
    with LocalServer(("127.0.0.1", args.port), handler) as server:
        print(f"Ready: http://127.0.0.1:{args.port}/pets.html", flush=True)
        server.serve_forever()
