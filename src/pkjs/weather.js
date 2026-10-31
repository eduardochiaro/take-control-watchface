// tempC, a WMO weather code and is_day from Open-Meteo -> the line under THE BOARD.
function describe(tempC, code, isDay, units) {
  var temp = Math.round(units === 'f' ? tempC * 9 / 5 + 32 : tempC);
  var condition, advice;
  if (code >= 95) {
    condition = 'Storm';
    advice = 'Stay inside';
  } else if ((code >= 71 && code <= 77) || code === 85 || code === 86) {
    condition = 'Snow';
    advice = 'Wear boots';
  } else if (code >= 51) {
    condition = 'Rain';
    advice = tempC < 25 ? 'Take a coat' : 'Stay dry';
  } else if (code === 45 || code === 48) {
    condition = 'Fog';
    advice = 'Drive slow';
  } else {
    condition = code <= 1 ? 'Clear' : 'Cloudy';
    if (tempC <= 5) {
      advice = 'Bundle up';
    } else if (tempC <= 15) {
      advice = 'Take a coat';
    } else if (tempC >= 30) {
      advice = 'Drink water';
    } else if (!isDay) {
      advice = 'Sleep well';
    } else {
      advice = code <= 1 ? 'Wear shades' : 'Carry on';
    }
  }
  return temp + '° / ' + condition + ' / ' + advice;
}

module.exports = describe;
