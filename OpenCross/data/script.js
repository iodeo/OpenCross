
var connection = new WebSocket('ws://opencross.local:81/', ['arduino']);

connection.onopen = function ()       { 
  connection.send('m?'); 
  setTimeout(getReady, 2000);
  };

connection.onerror = function (error) { console.log('WebSocket Error ', error); };
connection.onmessage = function (e)   { 
    console.log('Server: ', e.data);
    if (e.data.substring(0,1) == 'm') {
      var v = parseInt(e.data.substring(1));
      if (v>=0 & v<=99) {
        if (v < 10) v = '0'+v;
        document.getElementById('mm').innerHTML = v;
      }
    }
  };
  
function getReady() {
  document.getElementById('ctn').classList.remove("blur");
  document.getElementById('load').style.visibility = 'hidden';
}

function decTim() {
  var e = document.getElementById('mm');
  var v = parseInt(e.innerHTML);
  if (v > 0) v--;
  if (v < 10) v = '0'+v;
  e.innerHTML = v;
}

function incTim() {
  var e = document.getElementById('mm');
  var v = parseInt(e.innerHTML);
  if (v < 99) v++;
  if (v < 10) v = '0'+v;
  e.innerHTML = v;
}

function decLev() {
  var e = document.getElementById('lv');
  var v = parseInt(e.innerHTML);
  if (v > 0) v--;
  e.innerHTML = v;
}

function incLev() {
  var e = document.getElementById('lv');
  var v = parseInt(e.innerHTML);
  if (v < 9) v++;
  e.innerHTML = v;
}

var counter;

function countdown() {
  var e1 = document.getElementById('mm');
  var e2 = document.getElementById('ss');
  var v1 = parseInt(e1.innerHTML)*60 + parseInt(e2.innerHTML);
  if (v1 > 0) v1--;
  if (v1 == 0) {
    sendLevel0();
    pauseVid();
    clearInterval(counter);
    document.getElementById('play').classList.replace('pause','play');
    document.getElementById('ctn').style.backgroundColor = '#fff';
  }
  else sendLevel();
  var v2 = v1%60;
  v1 = (v1-v2)/60;
  if (v1 < 10) v1 = '0'+v1;
  e1.innerHTML = v1;
  if (v2 < 10) v2 = '0'+v2;
  e2.innerHTML = v2;
}

function play() {
  var e = document.getElementById('play');
  if (e.classList.contains('play')) {
    sendTime();
    playVid();
    counter = setInterval(countdown, 1000);
    e.classList.replace('play','pause');
    document.getElementById('ctn').style.backgroundColor = '#ccc';
  }
  else {
    sendLevel0();
    pauseVid();
    clearInterval(counter);
    e.classList.replace('pause','play');
    document.getElementById('ctn').style.backgroundColor = '#fff';
  }
}

function stop() {
  var e = document.getElementById('play');
  if (e.classList.contains('pause')) {
    sendLevel0();
    pauseVid();
    clearInterval(counter);
    e.classList.replace('pause','play');
    document.getElementById('ctn').style.backgroundColor = '#fff';
  }
  document.getElementById('mm').innerHTML = '00';
  document.getElementById('ss').innerHTML = '00';
}

function sendLevel() {
  var lv = parseInt(document.getElementById('lv').innerHTML);
  console.log('send: lv'+lv);
  connection.send('l'+lv);
}

function sendLevel0() {
  console.log('send: l0');
  connection.send('l0');
}

function sendTime() {
  var mm = parseInt(document.getElementById('mm').innerHTML);
  console.log('send: m'+mm);
  connection.send('m'+mm);
}

function playVid() {
  document.getElementById('vi').play();
}

function pauseVid() {
  document.getElementById('vi').pause();
}