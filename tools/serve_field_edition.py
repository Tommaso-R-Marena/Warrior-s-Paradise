#!/usr/bin/env python3
"""Serve Warrior's Paradise Field Edition on localhost or the local Wi-Fi.

No third-party packages. HTTP remote PWA install is intentionally unsupported:
use HTTPS Cloudflare Pages for offline home-screen installation.
"""
from __future__ import annotations

import argparse
import functools
import http.server
import socket
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "browser"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--lan", action="store_true", help="Listen on all interfaces (trusted LAN only)")
    args = parser.parse_args()

    if not 1 <= args.port <= 65535:
        parser.error("--port must be 1..65535")
    if not (ROOT / "index.html").is_file():
        parser.error("browser/index.html is missing")

    bind = "0.0.0.0" if args.lan else "127.0.0.1"
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(ROOT))
    with http.server.ThreadingHTTPServer((bind, args.port), handler) as server:
        print(f"Desktop: http://127.0.0.1:{args.port}/")
        if args.lan:
            print("LAN mode is unencrypted; share only with trusted devices on your local network.")
            try:
                local_ip = socket.gethostbyname(socket.gethostname())
                if not local_ip.startswith("127."):
                    print(f"Phone (same Wi-Fi): http://{local_ip}:{args.port}/")
            except OSError:
                pass
        print("Ctrl+C to stop the server.")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\nStopped.")


if __name__ == "__main__":
    main()
