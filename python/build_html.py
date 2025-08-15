from SCons.Script import Import
Import("env")

import os, io, gzip

# templating
from external.wheezy.template.engine import Engine
from external.wheezy.template.ext.core import CoreExtension
from external.wheezy.template.loader import FileLoader

# minifiers (ok if missing)
try:
    from external.minify import html_minifier, rcssmin, rjsmin
except Exception:
    html_minifier = rcssmin = rjsmin = None

ROOT = env["PROJECT_DIR"]
HTML_DIR = os.path.join(ROOT, "html")
OUT_H = os.path.join(ROOT, "include", "WebContent.h")

CTX = {
    "PLATFORM": "ESP32-S2",
    "VERSION": "dev",
    "isTX": False,
    "hasSubGHz": False,
    "is8285": False,
}

engine = Engine(loader=FileLoader([HTML_DIR]), extensions=[CoreExtension("@@")])

def gz(data: bytes) -> bytes:
    buf = io.BytesIO()
    with gzip.GzipFile(fileobj=buf, mode="wb", compresslevel=9, mtime=0) as f:
        f.write(data)
    return buf.getvalue()

def render_asset(name: str) -> str:
    tpl = engine.get_template(name)
    s = tpl.render(CTX)
    if name.endswith(".html") and html_minifier:
        s = html_minifier.html_minify(s)
    elif name.endswith(".css") and rcssmin:
        s = rcssmin.cssmin(s)
    elif name.endswith(".js") and rjsmin:
        s = rjsmin.jsmin(s)
    return s

def emit_array(var: str, blob: bytes) -> str:
    hexes = ",".join(f"0x{b:02X}" for b in blob)
    return f"static const uint8_t {var}[] PROGMEM = {{\n{hexes}\n}};\n\n"

assets = [
    ("index.html", "INDEX_HTML"),
    ("elrs.css",   "ELRS_CSS"),
    ("mui.js",     "MUI_JS"),
    ("scan.js",    "SCAN_JS"),
]

os.makedirs(os.path.dirname(OUT_H), exist_ok=True)
with open(OUT_H, "w", encoding="utf-8") as out:
    out.write("// Auto-generated — DO NOT EDIT\n#pragma once\n#include <stdint.h>\n#include <pgmspace.h>\n\n")
    for fname, var in assets:
        path = os.path.join(HTML_DIR, fname)
        text = render_asset(fname) if os.path.exists(path) else ""
        out.write(emit_array(var, gz(text.encode("utf-8"))))

print(f"[build_html] wrote {OUT_H}")
