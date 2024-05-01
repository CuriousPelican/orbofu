var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener("load", onload);

// -------------------------------- WEBSOCKETS & PAGE LOADING --------------------------------

function onload(event) {
  initWebSocket();
}

function initWebSocket() {
  console.log("Trying to open a WebSocket connection…");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log("Connection opened");
  // Update connection status
  document.getElementById("connection-sensor").innerHTML = "Connected";
  document.getElementById("connection-sensor").style.color = "#f582ae";
}

function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
  // Update connection status
  document.getElementById("connection-sensor").innerHTML = "Disconnected";
  document.getElementById("connection-sensor").style.color = "#8bd3dd";
}

// -------------------------------- HELPER FUNCTIONS --------------------------------

// Script to download & rename
function downloadFileWithTimestamp(url) {
  // const fileName = "data.csv";
  // const url = `http://ip/path/to/${fileName}`;

  const now = new Date();
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, "0"); // zeros added at start for single digit months (1 --> 01)
  const day = String(now.getDate()).padStart(2, "0");
  const hour = String(now.getHours()).padStart(2, "0");
  const minutes = String(now.getMinutes()).padStart(2, "0");
  const seconds = String(now.getSeconds()).padStart(2, "0");

  const timestampedFileName = `${year}-${month}-${day}-${hour}-${minutes}-${seconds}.csv`;

  const link = document.createElement("a");
  link.href = url;
  link.download = timestampedFileName;
  link.style.display = "none";
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

// Returns if convertible to float or not
function isConvertibleToFloat(str) {
  const num = parseFloat(str);
  return !isNaN(num);
}

// -------------------------------- PAGE INTERACTION --------------------------------

function onMessage(event) {
  // if message is about flight triggered
  // else message is about altitude
}

// functions to make buttons appear/disappear

// functions to use buttons
