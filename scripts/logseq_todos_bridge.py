#!/usr/bin/env python3
"""
Serve Logseq journal TODO/DONE items as JSON for the InfoDisplay ESP32.
Use when Logseq's getPageBlocksTree API returns null (known bug).

Finds journal files under LOGSEQ_GRAPH (default: ~/logseq) and parses
markdown for "- TODO ..." / "- DONE ..." (and "- [ ]" / "- [x]").

Usage:
  python3 logseq_todos_bridge.py [--graph PATH] [--port 8765] [--bind 0.0.0.0]
  # Then set LOGSEQ_TODOS_URL in config.h to http://<your-PC-IP>:8765/todos

Requires: Python 3.6+
"""

import argparse
import json
import os
import re
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
from typing import List, Optional


def find_journal_path(graph_root: str, date_str: str) -> Optional[str]:
    """Return path to journal .md file for date_str (yyyy_mm_dd or yyyy-mm-dd), or None."""
    journals = os.path.join(graph_root, "journals")
    if not os.path.isdir(journals):
        return None
    # Try underscore (2026_02_25) and dash (2026-02-25)
    name_under = date_str.replace("-", "_")
    name_dash = date_str.replace("_", "-")
    for name in (name_under, name_dash):
        path = os.path.join(journals, name + ".md")
        if os.path.isfile(path):
            return path
    return None


def parse_todos_from_md(path: str) -> List[dict]:
    """Parse a Logseq journal .md file for TODO/DONE items. Returns [{"title": str, "done": bool}, ...]."""
    out = []
    if not path or not os.path.isfile(path):
        return out
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError:
        return out
    # Logseq/Org: "- TODO title" or "- DONE title" (optional leading spaces)
    for m in re.finditer(r"^\s*-\s+(TODO|DONE)\s+(.+)$", text, re.MULTILINE):
        kind, title = m.group(1), m.group(2).strip()
        out.append({"title": title[:120], "done": kind == "DONE"})
    # Common: "- [ ] title" / "- [x] title"
    for m in re.finditer(r"^\s*-\s+\[([ xX])\]\s+(.+)$", text, re.MULTILINE):
        done = m.group(1).lower() == "x"
        title = m.group(2).strip()
        out.append({"title": title[:120], "done": done})
    return out


class TodosHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path != "/todos":
            self.send_response(404)
            self.end_headers()
            return
        qs = parse_qs(parsed.query)
        date_str = (qs.get("date") or [None])[0]
        if not date_str:
            date_str = datetime.now().strftime("%Y_%m_%d")
        graph = getattr(self.server, "graph_root", os.path.expanduser("~/logseq"))
        path = find_journal_path(graph, date_str)
        todos = parse_todos_from_md(path) if path else []
        body = json.dumps(todos).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        print("[bridge]", format % args)


def main():
    ap = argparse.ArgumentParser(description="Serve Logseq journal todos as JSON for ESP32 InfoDisplay.")
    ap.add_argument("--graph", default=os.environ.get("LOGSEQ_GRAPH", os.path.expanduser("~/logseq")),
                    help="Logseq graph root (default: ~/logseq or LOGSEQ_GRAPH)")
    ap.add_argument("--port", type=int, default=8765, help="Port (default: 8765)")
    ap.add_argument("--bind", default="0.0.0.0", help="Bind address (default: 0.0.0.0)")
    args = ap.parse_args()
    if not os.path.isdir(args.graph):
        print("Error: graph path is not a directory:", args.graph)
        return 1
    server = HTTPServer((args.bind, args.port), TodosHandler)
    server.graph_root = args.graph
    print("Serving todos at http://%s:%s/todos (graph: %s)" % (args.bind, args.port, args.graph))
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
