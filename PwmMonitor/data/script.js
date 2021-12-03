
var connection = new WebSocket('ws://opencross.local:81/', ['arduino']);

connection.onopen = function ()       { connection.send('Connect ' + new Date()); };
connection.onerror = function (error) { 
    console.log('WebSocket Error ', error);
    document.getElementById("dbg").innerText += 'ERROR:' + error + '\n';
  };
connection.onmessage = function (e)   { 
    console.log('Server: ', e.data);
    document.getElementById("dbg").innerText += 'SERVER:' + e.data + '\n';
    if (e.data == 'rpmUp') {
      document.getElementById('rpm').classList.toggle("light", true);
    }
    if (e.data == 'rpmDown') {
      document.getElementById('rpm').classList.toggle("light", false);
    }
  };
function fupd(element) {
  var fv = document.getElementById('fi').value;
  document.getElementById('fv').innerHTML = fv;
  console.log(fv);
  connection.send('f' + fv*1000);
}

function rupd(element) {
  var rv = document.getElementById('ri').value;
  document.getElementById('rv').innerHTML = Math.round(rv*100/255);
  console.log(rv);
  connection.send('r' + rv);
}
