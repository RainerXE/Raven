#!/usr/bin/env python3
"""Compare two directories of PPM frames (reference = Vulkan, candidate = sokol).

For each frame: max channel difference, mean absolute error, and the share of pixels that differ by
more than TOL (default 3, the tolerance of tests/test_render.c). With --png DIR also writes a
side-by-side image (reference | candidate | amplified difference)."""
import sys, os
from PIL import Image, ImageChops

TOL = 3
args = sys.argv[1:]
png = None
if "--png" in args:
    i = args.index("--png"); png = args[i + 1]; del args[i:i + 2]
ref, cand = args[0], args[1]
if png: os.makedirs(png, exist_ok=True)
worst = 0
print(f"{'scene':<16}{'size':<10}{'max':>5}{'mean':>8}{'>%d' % TOL:>9}   verdict")
for name in sorted(f for f in os.listdir(ref) if f.endswith(".ppm")):
    a = Image.open(os.path.join(ref, name)).convert("RGB")
    cpath = os.path.join(cand, name)
    if not os.path.exists(cpath):
        print(f"{name[:-4]:<16}missing"); continue
    b = Image.open(cpath).convert("RGB")
    if a.size != b.size:
        print(f"{name[:-4]:<16}SIZE MISMATCH {a.size} vs {b.size}"); worst = max(worst, 255); continue
    d = ImageChops.difference(a, b)
    px = list(d.getdata())
    mx = max(max(p) for p in px)
    mean = sum(sum(p) for p in px) / (3 * len(px))
    over = sum(1 for p in px if max(p) > TOL) / len(px)
    worst = max(worst, mx)
    verdict = "identical" if mx == 0 else ("within tolerance" if mx <= TOL else ("minor" if over < 0.005 else "DIFFERS"))
    print(f"{name[:-4]:<16}{a.size[0]}x{a.size[1]:<5}{mx:>5}{mean:>8.3f}{over*100:>8.2f}%   {verdict}")
    if png:
        amp = d.point(lambda v: min(255, v * 16))
        sheet = Image.new("RGB", (a.size[0] * 3 + 8, a.size[1]), (255, 0, 255))
        sheet.paste(a, (0, 0)); sheet.paste(b, (a.size[0] + 4, 0)); sheet.paste(amp, (a.size[0] * 2 + 8, 0))
        sheet.save(os.path.join(png, name[:-4] + ".png"))
sys.exit(0)
