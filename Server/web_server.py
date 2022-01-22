import os
from http.server import BaseHTTPRequestHandler
import urllib
import priority_queue


class WebServerContext:
    def __init__(self):
        self.commands = priority_queue.PriorityQueue()


Context = WebServerContext()


class AdaWebServer(BaseHTTPRequestHandler):
    def do_GET(self):
        global Context
        if self.path.endswith(".css") or self.path.endswith(".htm"):
            return self.serve_file(self.path)
        elif self.path == "/":
            return self.serve_file("main.htm")
        elif self.path == "/favicon.ico":
            self.send_response(400)
            return
        self.handle_command(self.path)

    def serve_file(self, filename):
        if filename.startswith("/"):
            filename = filename[1:]
        ext = os.path.splitext(filename)[1]
        script_path = os.path.dirname(__file__)
        self.send_response(200)
        if ext == ".css":
            self.send_header("Content-type", "text/css")
        elif ext == ".htm" or ext == ".html":
            self.send_header("Content-type", "text/html")
        else:
            raise Exception("Unsupported file type")
        self.end_headers()

        with open(os.path.join(script_path, filename), "rb") as f:
            while True:
                chunk = f.read(8000)
                if chunk:
                    self.wfile.write(chunk)
                else:
                    break

    def handle_command(self, command):
        Context.commands.enqueue(1, urllib.parse.unquote(command))
        self.send_response(200)
        self.send_header("Content-type", "text/text")
        self.wfile.write(bytes("ok", encoding='utf-8'))
