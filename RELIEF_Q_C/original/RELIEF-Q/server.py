#!/usr/bin/env python3
"""
RELIEF-Q local server
----------------------
Just a tiny static file server so the project can be opened at
http://localhost:8000 instead of a file:// path (this avoids any
browser restrictions on local file access and lets you open it the
same way a real web app would run).

Usage:
    python3 server.py
    (or just double-click start.bat on Windows / start.sh on Mac/Linux)

Then open http://localhost:8000 in your browser.
"""
import http.server
import socketserver
import webbrowser
import os

PORT = 8000
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        url = f"http://localhost:{PORT}"
        print(f"RELIEF-Q is running at {url}")
        print("Press CTRL+C to stop the server.")
        try:
            webbrowser.open(url)
        except Exception:
            pass
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")
