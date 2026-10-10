#!/usr/bin/env python3
"""
RELIEF-Q Local Development Server
---------------------------------
Lightweight HTTP server with proper WebAssembly (application/wasm) MIME type support.
Runs locally at http://localhost:8000
"""

import http.server
import socketserver
import webbrowser
import os
import sys

PORT = 8000
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

class WasmHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        # Add Cross-Origin headers for optimal WebAssembly execution
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-cache, must-revalidate")
        super().end_headers()

    def guess_type(self, path):
        if path.endswith(".wasm"):
            return "application/wasm"
        if path.endswith(".js"):
            return "application/javascript"
        if path.endswith(".json"):
            return "application/json"
        if path.endswith(".css"):
            return "text/css"
        if path.endswith(".html"):
            return "text/html"
        return super().guess_type(path)

if __name__ == "__main__":
    # Ensure mime-types dictionary knows .wasm
    WasmHandler.extensions_map['.wasm'] = 'application/wasm'
    WasmHandler.extensions_map['.js'] = 'application/javascript'
    
    with socketserver.TCPServer(("", PORT), WasmHandler) as httpd:
        url = f"http://localhost:{PORT}"
        print("=" * 60)
        print("  RELIEF-Q Local WebAssembly Simulation Server")
        print(f"  Running at: {url}")
        print("  Press CTRL+C to stop the server.")
        print("=" * 60)
        try:
            webbrowser.open(url)
        except Exception:
            pass
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")
            sys.exit(0)
