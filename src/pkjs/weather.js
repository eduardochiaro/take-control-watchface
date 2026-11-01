// The Board's directives. Keep to 15 characters and the glyphs in the font, or the line
// shrinks past legible: the watch scales it to fit the screen.
var ADVICE = {
  storm: ['Remain indoors', 'Cease transit', 'Await all-clear', 'Shelter on site'],
  snow: ['Boots mandated', 'Footwear policy', 'Boots approved', 'Secure footing'],
  coat: ['Coat mandated', 'Coat is policy', 'Coat approved', 'Outerwear memo'],
  damp: ['Avoid moisture', 'Mitigate damp', 'Remain dry', 'Umbrella issued'],
  fog: ['Proceed slowly', 'Visibility low', 'Limit velocity', 'Await clarity'],
  cold: ['Conserve heat', 'Retain heat', 'Add more layers', 'Layers mandated'],
  hot: ['Hydration due', 'Hydrate now', 'Mandatory water', 'Water approved'],
  night: ['Rest approved', 'Clock out', 'Rest is policy', 'Sleep scheduled'],
  sun: ['Shades approved', 'Eyewear advised', 'Deploy shades', 'Sun authorized'],
  overcast: ['Remain aligned', 'Await directive', 'Stay on task', 'Status nominal']
};

// tempC, a WMO weather code and is_day from Open-Meteo -> the line under THE BOARD.
// rand is in [0, 1), Math.random() unless a test pins it.
function describe(tempC, code, isDay, units, rand) {
  var temp = Math.round(units === 'f' ? tempC * 9 / 5 + 32 : tempC);
  var condition, advice;
  if (code >= 95) {
    condition = 'Storm';
    advice = 'storm';
  } else if ((code >= 71 && code <= 77) || code === 85 || code === 86) {
    condition = 'Snow';
    advice = 'snow';
  } else if (code >= 51) {
    condition = 'Rain';
    advice = tempC < 25 ? 'coat' : 'damp';
  } else if (code === 45 || code === 48) {
    condition = 'Fog';
    advice = 'fog';
  } else {
    condition = code <= 1 ? 'Clear' : 'Cloudy';
    if (tempC <= 5) {
      advice = 'cold';
    } else if (tempC <= 15) {
      advice = 'coat';
    } else if (tempC >= 30) {
      advice = 'hot';
    } else if (!isDay) {
      advice = 'night';
    } else {
      advice = code <= 1 ? 'sun' : 'overcast';
    }
  }
  var lines = ADVICE[advice];
  var pick = lines[Math.floor((rand === undefined ? Math.random() : rand) * lines.length)];
  return temp + '° / ' + condition + ' / ' + pick;
}

module.exports = describe;
module.exports.ADVICE = ADVICE;
