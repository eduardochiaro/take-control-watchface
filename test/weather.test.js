var assert = require('assert');
var describe = require('../src/pkjs/weather');

assert.strictEqual(describe(15, 61, true, 'c', 0), '15° / Rain / Coat mandated');
assert.strictEqual(describe(15, 61, true, 'f', 0), '59° / Rain / Coat mandated');
assert.strictEqual(describe(28, 80, true, 'c', 0), '28° / Rain / Avoid moisture');
assert.strictEqual(describe(-3.4, 73, true, 'c', 0), '-3° / Snow / Boots mandated');
assert.strictEqual(describe(18, 95, false, 'c', 0), '18° / Storm / Remain indoors');
assert.strictEqual(describe(10, 45, true, 'c', 0), '10° / Fog / Proceed slowly');
assert.strictEqual(describe(2, 0, true, 'c', 0), '2° / Clear / Conserve heat');
assert.strictEqual(describe(32, 3, true, 'c', 0), '32° / Cloudy / Hydration due');
assert.strictEqual(describe(22, 0, true, 'c', 0), '22° / Clear / Shades approved');
assert.strictEqual(describe(22, 0, false, 'c', 0), '22° / Clear / Rest approved');
assert.strictEqual(describe(22, 2, true, 'c', 0), '22° / Cloudy / Remain aligned');

// The whole range of rand lands on a line, top end included.
assert.strictEqual(describe(22, 2, true, 'c', 0.999), '22° / Cloudy / Status nominal');
assert.ok(/ \/ .+$/.test(describe(22, 2, true, 'c')));

// Every line fits the screen and the font, which has no other punctuation.
Object.keys(describe.ADVICE).forEach(function(key) {
  describe.ADVICE[key].forEach(function(line) {
    assert.ok(line.length <= 15, line);
    assert.ok(/^[A-Za-z -]+$/.test(line), line);
  });
});
console.log('ok');
