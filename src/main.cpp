#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// DS18b20

using namespace websockets;

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
void onWSMessage(WebsocketsMessage message);
void onWSEvent(WebsocketsEvent event, String data);

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
const char *ws_server_host = "iot.thor-electronics.ir";
const uint16_t ws_server_port = 80;
WebsocketsClient wsClient;

float TEMP_THRESHOLD = 32.0;

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
  for (int i = 10; i > 0 && WiFi.status() != WL_CONNECTED; i--)
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
  // wsClient.connect("ws://iot.thor-electronics.ir/api/v1/control", 80);
  bool connection = wsClient.connect(ws_server_host, ws_server_port, "/api/v1/control");
  Serial.println(connection);
  // wsClient.send("{}");
  // wsClient.ping();

  // xTaskCreatePinnedToCore(NetworkHandler, "NetworkTask", 10000, NULL, 1, &NetworkTask, 0);
  // delay(500);
  // xTaskCreatePinnedToCore(ThermostatHandler, "ThermostatTask", 10000, NULL, 1, &ThermostatTask, 1);
  // delay(500);
  // xTaskCreatePinnedToCore(IdleHandler, "IdleTask", 10000, NULL, 10, &IdleTask, 1);
  // delay(500);

  Serial.println("Initialized successfully!");
}

void loop()
{
  // Serial.println("Polling...");
  wsClient.poll();
  // Serial.println("Polled!");
}

void NetworkHandler(void *pvParameters)
{
  Serial.println("Initializing network task...");
  while (true)
  {
    // wsClient.poll();
    Serial.println("Network...");
    // neo
    delay(5000);
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

void onWSMessage(WebsocketsMessage message)
{
  Serial.print("Got WS Message: ");
  Serial.println(message.data());
}

void onWSEvent(WebsocketsEvent event, String data)
{
  Serial.print("WS event: ");
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connection Opened");
    neo(0, 255, 50, 1000);
    // todo: send token
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    neo(255, 0, 10, 1000);
    Serial.print("Connection Closed: ");
    Serial.println(wsClient.getCloseReason());
  }
  else if (event == WebsocketsEvent::GotPing)
  {
    neo(255, 255, 255, 100);
    Serial.println("Got a Ping!");
  }
  else if (event == WebsocketsEvent::GotPong)
  {
    neo(255, 255, 255, 100);
    Serial.println("Got a Pong!");
  }
}
