#!/usr/bin/env node
// Generate an SVG <font> from an OTF/TTF, suitable for pebble-fctx-compiler.
//
// SVG fonts are y-up with the baseline at y=0; opentype.js emits y-down, so y
// is negated. Coordinates stay in font units: the fctx compiler rescales them.
//
// Usage: node scripts/gen-svg-font.js <in.otf> <out.svg> <font-id> <chars>
const fs = require('fs');
const opentype = require('opentype.js');

const [, , inPath, outPath, fontId, chars] = process.argv;
if (!inPath || !outPath || !fontId || !chars) {
  console.error('usage: gen-svg-font.js <in.otf> <out.svg> <font-id> <chars>');
  process.exit(1);
}

const font = opentype.parse(fs.readFileSync(inPath));
const em = font.unitsPerEm;

function pathData(glyph) {
  const ny = (v) => -Math.round(v);
  const nx = Math.round;
  const d = glyph.getPath(0, 0, em).commands.map((c) => {
    switch (c.type) {
      case 'M': return `M${nx(c.x)} ${ny(c.y)}`;
      case 'L': return `L${nx(c.x)} ${ny(c.y)}`;
      case 'C': return `C${nx(c.x1)} ${ny(c.y1)} ${nx(c.x2)} ${ny(c.y2)} ${nx(c.x)} ${ny(c.y)}`;
      case 'Q': return `Q${nx(c.x1)} ${ny(c.y1)} ${nx(c.x)} ${ny(c.y)}`;
      default: return '';
    }
  }).join('');
  // OTF contours stop short of their start point, and fctx's fill needs them closed.
  return d && `${d.replace(/(?!^)M/g, 'ZM')}Z`;
}

function esc(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

const glyphs = [];
for (const ch of chars) {
  const g = font.charToGlyph(ch);
  if (g && g.index !== 0) {
    glyphs.push(`    <glyph unicode="${esc(ch)}" horiz-adv-x="${g.advanceWidth}" d="${pathData(g)}"/>`);
  } else {
    console.warn(`no glyph for ${ch}`);
  }
}

fs.writeFileSync(outPath, `<?xml version="1.0" standalone="no"?>
<svg xmlns="http://www.w3.org/2000/svg">
<defs>
  <font id="${fontId}" horiz-adv-x="${Math.round(em / 2)}">
    <font-face units-per-em="${em}" ascent="${font.ascender}" descent="${font.descender}" cap-height="${font.tables.os2.sCapHeight}"/>
${glyphs.join('\n')}
  </font>
</defs>
</svg>
`);
console.log(`wrote ${outPath}: ${glyphs.length} glyphs, em=${em}`);
