# web_server.py
import os, sys
from flask import Flask, Response, send_from_directory

BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
HTML_DIR = os.path.join(BASE, "html")

# Bypass external/__init__.py side effects by importing wheezy directly
sys.path.insert(0, os.path.join(BASE, "python", "external", "wheezy"))

from wheezy.template.engine import Engine
from wheezy.template.ext.core import CoreExtension
from wheezy.template.loader import FileLoader

app = Flask(__name__)

engine = Engine(
    loader=FileLoader([HTML_DIR]),
    extensions=[CoreExtension("@@")]
)

TEMPLATE_VARS = dict(
    PLATFORM="ESP32-S2",   # whatever you want to preview as
    VERSION="dev (local)",
    hasSubGHz=False,
    is8285=False,
)

MIME = {
    ".html": "text/html",
    ".css":  "text/css",
    ".js":   "application/javascript",
    ".svg":  "image/svg+xml",
}

def render_asset(name, **ctx):
    t = engine.get_template(name)
    return t.render(ctx)

@app.get("/")
def index():
    html = render_asset("index.html", **TEMPLATE_VARS)
    return Response(html, mimetype="text/html")

@app.get("/<path:path>")
def assets(path):
    ext = os.path.splitext(path)[1].lower()
    if ext in (".html", ".css", ".js"):  # render all templated assets
        body = render_asset(path, **TEMPLATE_VARS)
        return Response(body, mimetype=MIME.get(ext, "text/plain"))
    return send_from_directory(HTML_DIR, path)

if __name__ == "__main__":
    app.run(debug=True)
