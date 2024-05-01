var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener("load", onload);

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
  // update connection status
}

function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
  // update connection status
}

function onMessage(event) {}

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

// Make buttons appear/disappear

// Update connection & last flight apogee
