# hexlog_to_bbl.py
import re, sys

# Usage: python hexlog_to_bbl.py device-monitor.log out.bbl
src = sys.argv[1]
dst = sys.argv[2]

hex_token = re.compile(r'\b[0-9A-Fa-f]{2}\b')

with open(src, 'r', errors='ignore') as f, open(dst, 'wb') as o:
    for line in f:
        # extract "AA", "0F", "7c" etc. and write as bytes
        bs = bytes(int(t, 16) for t in hex_token.findall(line))
        if bs:
            o.write(bs)

print(f"Wrote {dst}")
