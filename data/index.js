/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initButton();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    let data = JSON.parse(event.data);
    console.log(data);

    document.getElementById('led1').className = data.button1_status;
    document.getElementById('led2').className = data.button2_status;
    document.getElementById('led3').className = data.button3_status;
    document.getElementById('led4').className = data.button4_status;
    document.getElementById('led5').className = data.button5_status;
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('toggle1').addEventListener('click', onToggle);
    document.getElementById('toggle2').addEventListener('click', onToggle);
    document.getElementById('toggle3').addEventListener('click', onToggle);
    document.getElementById('toggle4').addEventListener('click', onToggle);
    document.getElementById('toggle5').addEventListener('click', onToggle);
}

function onToggle(event) {
    console.log(event);
    sourceButton = event.srcElement.id;
    console.log("TOGGLE");
    console.log(sourceButton);
    websocket.send(JSON.stringify({'action':sourceButton}));
}
