#!/usr/bin/env fontforge -lang=py
# Thicken a font's outlines, keeping its letter shapes and heights.
#
# The result is a modified font, so it gets its own name, as the GUST Font
# License asks. Stroke is in font units, spread over both sides of a stem.
#
# Usage: fontforge -lang=py scripts/embolden-font.py <in.otf> <out.otf> <stroke>
import sys
import fontforge

in_path, out_path, stroke = sys.argv[1], sys.argv[2], int(sys.argv[3])

font = fontforge.open(in_path)
font.selection.all()
font.changeWeight(stroke, "LCG", 0, 0, "auto")
font.fontname = "AdventorHeavy"
font.familyname = "Adventor Heavy"
font.fullname = "Adventor Heavy"
font.generate(out_path)
