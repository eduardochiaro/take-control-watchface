var assert = require('assert');
var describe = require('../src/pkjs/weather');

assert.strictEqual(describe(15, 61, true, 'c'), '15° / Rain / Take a coat');  // the mockup
assert.strictEqual(describe(15, 61, true, 'f'), '59° / Rain / Take a coat');
assert.strictEqual(describe(28, 80, true, 'c'), '28° / Rain / Stay dry');
assert.strictEqual(describe(-3.4, 73, true, 'c'), '-3° / Snow / Wear boots');
assert.strictEqual(describe(18, 95, false, 'c'), '18° / Storm / Stay inside');
assert.strictEqual(describe(10, 45, true, 'c'), '10° / Fog / Drive slow');
assert.strictEqual(describe(2, 0, true, 'c'), '2° / Clear / Bundle up');
assert.strictEqual(describe(32, 3, true, 'c'), '32° / Cloudy / Drink water');
assert.strictEqual(describe(22, 0, true, 'c'), '22° / Clear / Wear shades');
assert.strictEqual(describe(22, 0, false, 'c'), '22° / Clear / Sleep well');
assert.strictEqual(describe(22, 2, true, 'c'), '22° / Cloudy / Carry on');
console.log('ok');
