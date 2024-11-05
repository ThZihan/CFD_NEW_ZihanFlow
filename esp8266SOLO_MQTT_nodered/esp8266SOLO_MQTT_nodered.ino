#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

// Unique device ID (Change this for each device: 1 to 10)
int deviceid = 1;

// Wi-Fi credentials
#define WIFI_SSID "IUB-Wave"
#define WIFI_PASSWORD ""

// MQTT server details
#define MQTT_HOST IPAddress(103, 237, 39, 27)
#define MQTT_PORT 1883

// Base MQTT Topic
#define MQTT_BASE_TOPIC "Zihan/device"

// Initialize variables
float temp = 0;
float hum = 0;
float Pressure = 0;
float alt = 0;
float PM1 = 0;
float PM25 = 0;
float PM10 = 0;
float CO2 = 0;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time data was published
const long interval = 10000;        // Interval at which to publish sensor readings (10 seconds)

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged. PacketId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Initialize random seed
  randomSeed(micros());

  // Set up Wi-Fi event handlers
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  // Set up MQTT event handlers
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();

  // Publish data at specified intervals
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Randomly generate sensor data
    temp = random(200, 350) / 10.0;        // Temperature between 20.0 and 35.0°C
    hum = random(300, 700) / 10.0;         // Humidity between 30.0% and 70.0%
    Pressure = random(9800, 10500) / 10.0; // Pressure between 980.0 and 1050.0 hPa
    alt = random(0, 500);                  // Altitude between 0 and 500 meters
    PM1 = random(0, 50);                   // PM1 between 0 and 50 µg/m³
    PM25 = random(0, 50);                  // PM2.5 between 0 and 50 µg/m³
    PM10 = random(0, 50);                  // PM10 between 0 and 50 µg/m³
    CO2 = random(400, 1000);               // CO2 between 400 and 1000 ppm

    // Create JSON payload
    String payload = "{";
    payload += "\"deviceid\":" + String(deviceid) + ",";
    payload += "\"temp\":" + String(temp) + ",";
    payload += "\"hum\":" + String(hum) + ",";
    payload += "\"pressure\":" + String(Pressure) + ",";
    payload += "\"altitude\":" + String(alt) + ",";
    payload += "\"pm1\":" + String(PM1) + ",";
    payload += "\"pm25\":" + String(PM25) + ",";
    payload += "\"pm10\":" + String(PM10) + ",";
    payload += "\"co2\":" + String(CO2);
    payload += "}";

    // Construct the topic with device ID
    String topic = String(MQTT_BASE_TOPIC) + String(deviceid) + "/sensor";

    // Publish sensor data to MQTT topic
    mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
    Serial.printf("Published data to topic %s: %s\n", topic.c_str(), payload.c_str());
  }
}
