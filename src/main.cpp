#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPIFFS.h>

const char *ssid = "";
const char *password = "";

#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int BUTTON_PIN = 23;

// Button press timeouts, all in miliseconds
const unsigned long DOT_TIME = 100;
const unsigned long DASH_TIME = DOT_TIME * 3;

// Button release timeouts
const unsigned long LETTER_GAP = DOT_TIME * 3;
const unsigned long RESET_GAP = DOT_TIME * 10;

// Don't change these values
bool lastState = HIGH;
bool willReset = false;
unsigned long lastChangeTime = 0;
String currentMorseWord = "";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocket wsSound("/ws-sound");
TaskHandle_t Task1;

/*
  ws-sound comms:
  - START ......... Start the audio signal
  - STOP .......... Stop the audio signal

  ws comms:
  - MORSE:STRING ......... The morse string received from the ESP32
  - TRANSLATE:STRING ..... The translated string received from the ESP32
  - CLEAR ................ Clear the morse and translated strings
*/

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    Serial.printf("WebSocket data received from client #%u: %s\n", client->id(), (char *)data);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void Task1code(void *parameter)
{
  bool lastButtonState = HIGH;
  for (;;)
  {
    bool currentButtonState = digitalRead(BUTTON_PIN);

    if (currentButtonState != lastButtonState)
    {
      if (currentButtonState == LOW)
      {
        wsSound.textAll("START");
      }
      else
      {
        wsSound.textAll("STOP");
      }
      lastButtonState = currentButtonState;
    }
  }
}

char morseToChar(const String &s)
{
  struct
  {
    const char *code;
    char letter;
  } table[] = {
      {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'}, {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'}, {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}, {nullptr, 0}};
  for (int i = 0; table[i].code; ++i)
    if (s == table[i].code)
      return table[i].letter;
  return '?';
}

String decodeMorse(const String &morseCode)
{
  String decodedMessage = "";
  String currentLetter = "";

  for (unsigned int i = 0; i < morseCode.length(); ++i)
  {
    char c = morseCode[i];
    if (c == ' ')
    {
      if (currentLetter.length() > 0)
      {
        decodedMessage += morseToChar(currentLetter);
        currentLetter = ""; // reset for next letter
      }
    }
    else
    {
      currentLetter += c;
    }
  }

  // Decode the last letter if any
  if (currentLetter.length() > 0)
  {
    decodedMessage += morseToChar(currentLetter);
  }

  return decodedMessage;
}

void clearSystems()
{
  ws.textAll("CLEAR");
  currentMorseWord = "";
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("Morse Code:");
  display.println(" ");
  display.println("Translated:");
  display.println(" ");
  display.display();
}

void updateSystems()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  ws.textAll("MORSE:" + currentMorseWord);
  display.println("Morse Code:");
  display.println(currentMorseWord);

  String decodedMessage = decodeMorse(currentMorseWord);
  ws.textAll("TRANSLATE:" + decodedMessage);
  display.println("Translated:");
  display.println(decodedMessage);
  display.display();
};

void loop()
{
  ws.cleanupClients();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected, trying to reconnect...");
    WiFi.reconnect();
  }

  bool cur = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (cur != lastState)
  {
    unsigned long delta = now - lastChangeTime;

    if (willReset)
    {
      clearSystems();
      willReset = false;
    }

    if (lastState == LOW)
    { // button was held LOW, now HIGH → classify dot/dash
      if (delta < DOT_TIME * 1.5)
        currentMorseWord += '.';
      else if (delta < DASH_TIME * 1.5)
        currentMorseWord += '-';
      // you can add an “else” for too-long presses if desired
    }
    else
    { // button was HIGH, now LOW → classify gap
      if (delta >= RESET_GAP)
      {
        willReset = true;
      }
      else if (delta >= LETTER_GAP)
      {
        currentMorseWord += ' '; // letter gap
      }
      // else intra-letter gap: do nothing
    }

    updateSystems();

    lastChangeTime = now;
    lastState = cur;
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (1)
      ;
  }

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    while (1)
      ;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to " + String(ssid));
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(200);
  }
  Serial.println();
  Serial.println("IP address: " + String(WiFi.localIP().toString()));

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(("Connected to " + String(ssid)).c_str());
  display.println(("IP: " + String(WiFi.localIP().toString())).c_str());
  display.display();

  ws.onEvent(onEvent);
  wsSound.onEvent(onEvent);
  server.addHandler(&ws);
  server.addHandler(&wsSound);

  // setup complete //

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/")
    .setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Not found"); });

  server.begin();
  lastChangeTime = millis();

  xTaskCreatePinnedToCore(
      Task1code, // Function to implement the task
      "Task1",   // Name of the task
      10000,     // Stack size in words
      NULL,      // Task input parameter
      1,         // Priority of the task
      &Task1,    // Task handle
      1);        // Core where the task should run
}
