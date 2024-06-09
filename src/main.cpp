#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <WebSocketsClient.h>
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// DS18b20
#include <ArduinoJson.h>

// using namespace websockets;

TaskHandle_t NetworkTask;
TaskHandle_t ThermostatTask;
TaskHandle_t IdleTask; // blink
// TaskHandle_t LEDTask;
// TaskHandle_t FanTask;

void NetworkHandler(void *pvParameters);
void ThermostatHandler(void *pvParameters);
void IdleHandler(void *pvParameters);
void colorTransition(uint32_t targetColor, int transitionTime);
void led(bool on, int delay_ms);
void neo(uint8_t r, uint8_t g, uint8_t b, int delay_ms);
void onWSMessage(websockets::WebsocketsMessage message);
void onWSEvent(websockets::WebsocketsEvent event, String data);

#define LED 2
#define NEO 22
#define NUM_NEO 1
#define FAN 5
#define SENSOR 4

Adafruit_NeoPixel pixels(NUM_NEO, NEO, NEO_GRB + NEO_KHZ800);
OneWire oneWire(SENSOR);
DallasTemperature sensors(&oneWire);

const char *ssid = "Xiaomi 11 Lite 5G NE";
const char *password = "fibonachi";
const char *TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6IjY2NjVhMjI3ZjFhODA4OGI2MTkxMDZmMyIsImNwdUlkIjoiNjY2NWEyMjdmMWE4MDg4YjYxOTEwNmYzIiwibmFtZSI6IlRoZXJtb3giLCJ1c2VySWQiOiI2MzI1N2UxOWIwMDI4MzJmODQ4ZmQ5ZjgiLCJncm91cElkIjoiNjY2NWExZjVmMWE4MDg4YjYxOTEwNmYxIiwidHlwZSI6IlRIRVJNT1NUQVQiLCJpc3MiOiJpb3QtY29yZS1hdXRoLXNlcnZlciIsImV4cCI6MTcyMDUzMDg4OSwiaWF0IjoxNzE3OTM4ODg5fQ.IpA6VxuWpUcuzduDqESJ5-ovn3fVg77jfaL8J1aPSmA";
websockets::WebsocketsClient wsClient;
WebSocketsClient wsC;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_ERROR:
  case WStype_DISCONNECTED:
    Serial.printf("[WebSocket] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    Serial.printf("[WebSocket] Connected to: %s\n", payload);
    // Send a message to the server
    wsC.sendTXT("Hello from ESP32!");
    break;
  case WStype_TEXT:
    Serial.printf("[WebSocket] Received text: %s\n", payload);
    break;
  }
}

float TEMP_THRESHOLD = 32.0;
float CURRENT_TEMP = -127.0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");
  pixels.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FAN, OUTPUT);

  Serial.print("Connecting to WiFi ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  for (int i = 20; i > 0 && WiFi.status() != WL_CONNECTED; i--)
  {
    Serial.print(".");
    neo(100, 0, 125, 200);
    neo(0, 0, 0, 500);
  }
  Serial.print("Connected to WiFi");
  Serial.println(ssid);

  Serial.println("Connecting to WS server...");
  wsClient.onMessage(onWSMessage);
  wsClient.onEvent(onWSEvent);
  wsClient.setInsecure();
  // wsClient.connect("ws://iot.thor-electronics.ir/api/v1/control", 80);
  // bool connection = wsClient.connect("ws://iot.thor-electronics.ir/api/v1/control/");
  // bool connection = wsClient.connectSecure("iot.thor-electronics.ir", 443, "/api/v1/control/");
  // bool connection = wsClient.connect("iot.thor-electronics.ir", 80, "/api/v1/control/");
  bool connection = wsClient.connect("192.168.34.105", 3993, "/api/v1/control/");
  // bool connection = wsClient.connect("ws://echo.websocket.org");
  // bool connection = wsClient.connect("ws://192.168.34.105:3993/api/v1/control/");
  wsClient.setInsecure();
  Serial.print("WS Connection: ");
  Serial.println(connection);

  Serial.print("Connecting to WSC...");
  // wsC.begin("echo.websocket.org", 80, "/");
  // wsC.begin("iot.thor-electronics.ir", 80, "/api/v1/control/");
  // wsC.begin("192.168.34.105", 3993, "/api/v1/control/");
  wsC.onEvent(webSocketEvent);
  wsC.setReconnectInterval(5000);
  // wsClient.send("{}");
  // wsClient.ping();

  // xTaskCreatePinnedToCore(NetworkHandler, "NetworkTask", 10000, NULL, 1, &NetworkTask, 0);
  // delay(500);
  xTaskCreatePinnedToCore(ThermostatHandler, "ThermostatTask", 10000, NULL, 1, &ThermostatTask, 1);
  delay(500);
  // xTaskCreatePinnedToCore(IdleHandler, "IdleTask", 10000, NULL, 10, &IdleTask, 1);
  // delay(500);

  Serial.println("Initialized successfully!");
}

void loop()
{
  // Serial.println("Polling...");
  wsClient.poll();
  // wsC.loop();
  // Serial.println("Polled!");
}

void NetworkHandler(void *pvParameters)
{
  Serial.println("Initializing network task...");
  while (true)
  {
    StaticJsonDocument<255> doc;
    doc["ok"] = true;
    doc["signal"] = "STATE_UPDATED";
    doc["payload"]["temperature"] = CURRENT_TEMP;
    doc["update"]["temperature"] = CURRENT_TEMP;
    String jsonStr;
    serializeJson(doc, jsonStr);
    Serial.print("Sending temp update: ");
    Serial.println(jsonStr);
    wsClient.send(jsonStr);
    led(true, 100);
    led(false, 1000);
  }
}

void ThermostatHandler(void *pvParameters)
{
  Serial.println("Initializing thermostat handler...");
  neo(10, 255, 20, 0);
  while (true)
  {
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    CURRENT_TEMP = temperatureC;
    Serial.print("Temp: ");
    Serial.print(temperatureC);
    Serial.print("C ");
    if (temperatureC < -125.0)
    {
      Serial.println("Failed to check temperature!");
      neo(255, 0, 0, 100);
      neo(10, 2, 0, 100);
    }
    else if (temperatureC >= TEMP_THRESHOLD)
    {
      digitalWrite(FAN, HIGH);
      Serial.println("Fan is ON");
      colorTransition(pixels.Color(255, 30, 0), 3000);
    }
    else
    {
      digitalWrite(FAN, LOW);
      Serial.println("Fan is OFF");
      colorTransition(pixels.Color(0, 50, 255), 3000);
    }
    // delay(1000);
    StaticJsonDocument<255> doc;
    doc["ok"] = true;
    doc["signal"] = "STATE_UPDATED";
    // doc["payload"]["temperature"] = temperatureC;
    doc["update"]["temperature"] = temperatureC;
    String jsonStr;
    serializeJson(doc, jsonStr);
    Serial.print("Sending temp update: ");
    Serial.println(jsonStr);
    wsClient.send(jsonStr);
    led(true, 100);
    led(false, 1000);
  }
}

void IdleHandler(void *pvParameters)
{
  Serial.println("Initializing idle handler ...");
  while (true)
  {
    led(true, 75);
    led(false, 400);
    // colorTransition(pixels.Color(10, 0, 0), pixels.Color(0, 0, 10), 2000);
    // colorTransition(pixels.Color(0, 0, 10), pixels.Color(0, 10, 0), 1000);
    // colorTransition(pixels.Color(0, 10, 0), pixels.Color(10, 0, 0), 300);
  }
}

void colorTransition(uint32_t targetColor, int transitionTime)
{
  uint32_t currentColor = pixels.getPixelColor(0);
  uint8_t r1, g1, b1, r2, g2, b2;
  int steps = 256;
  r1 = (currentColor >> 16) & 0xFF;
  g1 = (currentColor >> 8) & 0xFF;
  b1 = currentColor & 0xFF;
  r2 = (targetColor >> 16) & 0xFF;
  g2 = (targetColor >> 8) & 0xFF;
  b2 = targetColor & 0xFF;
  float rInc = (r2 - r1) / (float)steps;
  float gInc = (g2 - g1) / (float)steps;
  float bInc = (b2 - b1) / (float)steps;
  for (int i = 0; i <= steps; i++)
  {
    uint8_t r = r1 + (rInc * i);
    uint8_t g = g1 + (gInc * i);
    uint8_t b = b1 + (bInc * i);
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
    delay(transitionTime / steps);
  }
}

void led(bool on, int delay_ms)
{
  digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
  delay(delay_ms);
}

void neo(uint8_t r, uint8_t g, uint8_t b, int delay_ms)
{
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
  delay(delay_ms);
}

void onWSMessage(websockets::WebsocketsMessage message)
{
  Serial.print("Got WS Message: ");
  Serial.println(message.data());
  StaticJsonDocument<255> doc;
  DeserializationError error = deserializeJson(doc, message.data());
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  float targetTemperature = doc["command"]["targetTemperature"];
  if (targetTemperature)
  {
    TEMP_THRESHOLD = targetTemperature;
    Serial.print("Target Temperature: ");
    Serial.println(targetTemperature);
  }
}

void onWSEvent(websockets::WebsocketsEvent event, String data)
{
  Serial.print("WS event: ");
  if (event == websockets::WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connection Opened");
    neo(0, 255, 50, 1000);
    while (!wsClient.available())
    {
      neo(0, 0, 0, 500);
      neo(250, 250, 250, 500);
      Serial.print(".");
    }
    StaticJsonDocument<255> doc;
    doc["signal"] = "AUTHENTICATE";
    doc["payload"]["token"] = TOKEN;
    String jsonStr;
    serializeJson(doc, jsonStr);
    Serial.print("Sending authenticate message: ");
    Serial.println(jsonStr);
    wsClient.send(jsonStr);
  }
  else if (event == websockets::WebsocketsEvent::ConnectionClosed)
  {
    neo(255, 0, 10, 1000);
    Serial.print("Connection Closed: ");
    Serial.println(wsClient.getCloseReason());
  }
  else if (event == websockets::WebsocketsEvent::GotPing)
  {
    neo(255, 255, 255, 100);
    Serial.println("Got a Ping!");
  }
  else if (event == websockets::WebsocketsEvent::GotPong)
  {
    neo(255, 255, 255, 100);
    Serial.println("Got a Pong!");
  }
}
