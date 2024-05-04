var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener("load", onload);

var flight_triggered = false;
var apogee = 0.0;

// -------------------------------- WEBSOCKETS & PAGE LOADING --------------------------------

function onload(event) {
  initWebSocket();
  update_page();
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

// Shows/hides buttons & updates apogee and flight status
function update_page() {
  document.getElementById("apogee-sensor").innerHTML = apogee;
  if (!flight_triggered) {
    document.getElementById("flight-sensor").innerHTML = "Waiting";
    document.getElementById("flight-sensor").style.color = "#001858";
    document.getElementById("download-button").style.display = "block";
    document.getElementById("start-button").style.display = "block";
    document.getElementById("stop-button").style.display = "none";
  } else {
    document.getElementById("flight-sensor").innerHTML = "Flight armed";
    document.getElementById("flight-sensor").style.color = "#8bd3dd";
    document.getElementById("download-button").style.display = "none";
    document.getElementById("start-button").style.display = "none";
    document.getElementById("stop-button").style.display = "block";
  }
}

// -------------------------------- PAGE INTERACTION --------------------------------

function onMessage(event) {
  console.log(event.data);
  message = event.data;
  if (message == "true") {
    flight_triggered = true;
    update_page();
  } else if (message == "false") {
    flight_triggered = false;
    update_page();
  } else if (isConvertibleToFloat(message)) {
    apogee = parseFloat(message);
  } else {
    console.log(
      "Websocket Message received but not true, false or apogee altitude, ignoring"
    );
  }
}

// Use buttons
const downloadbutton = document.getElementById("download-button");
downloadbutton.addEventListener("click", () => {
  console.log("Download button triggered");
  // ask for download
});
const startbutton = document.getElementById("start-button");
downloadbutton.addEventListener("click", () => {
  console.log("Start button triggered");
  websocket.send("true");
  // do not change local state of flight_triggered, wait for server confimation
});
const stopbutton = document.getElementById("stop-button");
downloadbutton.addEventListener("click", () => {
  console.log("Stop button triggered");
  websocket.send("false");
});
