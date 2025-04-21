/*
    ws-sound comms:
    - START ......... Start the audio signal
    - STOP .......... Stop the audio signal

    ws comms:
    - MORSE:STRING ......... The morse string received from the ESP32
    - TRANSLATE:STRING ..... The translated string received from the ESP32
    - CLEAR ................ Clear the morse and translated strings
*/

// Morse code dictionary
const morseCodeMap = {
    'A': '.-', 'B': '-...', 'C': '-.-.', 'D': '-..', 'E': '.', 'F': '..-.',
    'G': '--.', 'H': '....', 'I': '..', 'J': '.---', 'K': '-.-', 'L': '.-..',
    'M': '--', 'N': '-.', 'O': '---', 'P': '.--.', 'Q': '--.-', 'R': '.-.',
    'S': '...', 'T': '-', 'U': '..-', 'V': '...-', 'W': '.--', 'X': '-..-',
    'Y': '-.--', 'Z': '--..', '0': '-----', '1': '.----', '2': '..---',
    '3': '...--', '4': '....-', '5': '.....', '6': '-....', '7': '--...',
    '8': '---..', '9': '----.'
};

// Function to convert text to Morse code
function textToMorse(text) {
    return text.toUpperCase().split('').map(char => {
        if (char === ' ') return '   '; // 3 spaces for word separation
        return morseCodeMap[char] ? morseCodeMap[char] + ' ' : char + ' ';
    }).join('');
}

// Convert text to Morse code as user types
function setupMorseConverter() {
    const textInput = document.getElementById('textInput');
    const morseOutput = document.getElementById('morseOutput');

    textInput.addEventListener('input', () => {
        const text = textInput.value;
        if (text) {
            const morse = textToMorse(text);
            morseOutput.textContent = morse;
        } else {
            morseOutput.textContent = '';
        }
    });
}

document.addEventListener('DOMContentLoaded', () => {
    setupMorseConverter();
});

const LOW = 0.0;
const HIGH = 0.2;

let audioCtx;
let osc;
let gain;
let audioInitialized = false;
let websocketInitialized = false;
let soundWebsocketInitialized = false;

function setupAudio() {
    if (audioInitialized) return;

    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    osc = audioCtx.createOscillator();
    gain = audioCtx.createGain();

    osc.type = 'sine';
    osc.frequency.value = 800;
    gain.gain.value = 0.0;

    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start(audioCtx.currentTime);

    audioInitialized = true;

    document.getElementById('audioStatus').textContent = 'Audio initialized';
    document.getElementById('audioStatus').className = 'status connected';
}

let gateway = `ws://${window.location.hostname}/ws`;
let soundGateway = `ws://${window.location.hostname}/ws-sound`;
let websocket;
let soundWebsocket;
let statusElement;
let initializeButton;

window.addEventListener('load', () => {
    statusElement = document.getElementById('connectionStatus');
    initializeButton = document.getElementById('initializeBtn');

    initializeButton.addEventListener('click', () => {
        setupAudio();

        if (!websocketInitialized || !websocket || websocket.readyState === WebSocket.CLOSED) {
            initWebSocket();
            websocketInitialized = true;
        }

        if (!soundWebsocketInitialized || !soundWebsocket || soundWebsocket.readyState === WebSocket.CLOSED) {
            initSoundWebSocket();
            soundWebsocketInitialized = true;
        }
    });
});

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    statusElement.textContent = 'Connecting...';
    statusElement.className = 'status disconnected';

    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function initSoundWebSocket() {
    console.log('Trying to open a Sound WebSocket connection…');

    soundWebsocket = new WebSocket(soundGateway);
    soundWebsocket.onopen = () => console.log('Sound WebSocket connected');
    soundWebsocket.onclose = () => console.log('Sound WebSocket closed');
    soundWebsocket.onmessage = onSoundMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    statusElement.textContent = 'Connected';
    statusElement.className = 'status connected';
    initializeButton.textContent = 'Reconnect';
}

function onClose(event) {
    console.log('Connection closed');
    statusElement.textContent = 'Connection lost';
    statusElement.className = 'status disconnected';
}

// Function that receives sound messages
function onSoundMessage(event) {
    if (!audioInitialized) {
        console.log("Audio not initialized yet");
        return;
    }

    if (event.data == "START") {
        gain.gain.setValueAtTime(HIGH, audioCtx.currentTime);
    } else if (event.data == "STOP") {
        gain.gain.setValueAtTime(LOW, audioCtx.currentTime);
    } else {
        console.log("Unknown sound message: " + event.data);
    }
}

// Function that receives the message from the ESP32
function onMessage(event) {
    if (event.data.startsWith("MORSE:")) {
        const morseString = event.data.substring(6);
        document.getElementById('morseresult').textContent = "Morse: " + morseString;
    } else if (event.data.startsWith("TRANSLATE:")) {
        const translatedString = event.data.substring(10);
        document.getElementById('translatedresult').textContent = "Translated: " + translatedString;
    } else if (event.data == "CLEAR") {
        document.getElementById('morseresult').textContent = "Morse: ";
        document.getElementById('translatedresult').textContent = "Translated: ";
    } else {
        console.log("Unknown message: " + event.data);
    }
}
