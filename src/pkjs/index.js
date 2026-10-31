var Clay = require('@rebble/clay');
var describe = require('./weather');
var clay = new Clay(require('./config.json'));

var REFRESH_MS = 30 * 60 * 1000;

function units() {
  try {
    return JSON.parse(localStorage.getItem('clay-settings')).UNITS || 'c';
  } catch (e) {
    return 'c';
  }
}

// Open-Meteo, no key needed. On failure the watch keeps showing the last line.
function fetchWeather(lat, lon) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    try {
      var now = JSON.parse(xhr.responseText).current;
      Pebble.sendAppMessage({ WEATHER: describe(now.temperature_2m, now.weather_code, now.is_day === 1, units()) });
    } catch (e) {
      console.log('Weather failed: ' + e.message);
    }
  };
  xhr.onerror = function() { console.log('Weather failed: network'); };
  xhr.open('GET', 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon +
    '&current=temperature_2m,weather_code,is_day');
  xhr.send();
}

function refresh() {
  navigator.geolocation.getCurrentPosition(function(pos) {
    fetchWeather(pos.coords.latitude, pos.coords.longitude);
  }, function(err) {
    console.log('Location failed: ' + err.message);
  }, { timeout: 15000, maximumAge: REFRESH_MS });
}

Pebble.addEventListener('ready', function() {
  refresh();
  setInterval(refresh, REFRESH_MS);
});

// After Clay's own listener has saved the settings: the units may have changed.
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) {
    refresh();
  }
});
