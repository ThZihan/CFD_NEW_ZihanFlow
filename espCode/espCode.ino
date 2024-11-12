#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "IUB_BUS_Shuttle"
#define WIFI_PASSWORD "fabber@Bus"

// MQTT server details
#define MQTT_HOST IPAddress(103, 237, 39, 27)
#define MQTT_PORT 1883

// Base MQTT Topic
#define MQTT_BASE_TOPIC "Zihan/device"

// Unique device ID
int deviceid = 3;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

char dataBuffer[256];  // Character array for incoming data buffer
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP8266 setup");

  // Set up Wi-Fi and MQTT event handlers
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop() {
  // Ensure WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

  // Read data from Arduino Mega (UART3)
  while (Serial.available()) {
    char c = Serial.read();

    // Prevent buffer overflow
    if (bufferIndex < sizeof(dataBuffer) - 1) {
      dataBuffer[bufferIndex++] = c;
    }

    // Process and send data if we detect the end of a packet
    if (c == '\n') {
      dataBuffer[bufferIndex] = '\0';  // Null-terminate the string
      if (processAndSendData(dataBuffer)) {
        Serial.println("Data sent to MQTT successfully.");
      } else {
        Serial.println("Error: Invalid data format.");
      }
      bufferIndex = 0;  // Reset buffer index after processing
    }
  }
}

void connectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(5, connectToWifi);  // Retry connection every 5 seconds
}

void connectToMqtt() {
  if (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);  // Retry MQTT connection every 2 seconds
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged. PacketId: ");
  Serial.println(packetId);
}

bool processAndSendData(const char* data) {
  float temp, hum, pressure, pm1, pm25, pm10;
  int CO2;

  // Parse the data received from Arduino
  int result = sscanf(data, "T:%f|H:%f|P:%f|PM1:%f|PM25:%f|PM10:%f|CO2:%d", &temp, &hum, &pressure, &pm1, &pm25, &pm10, &CO2);

  // Check if parsing was successful
  if (result != 7) {
    return false;
  }

  // Construct JSON payload
  String payload = "{";
  payload += "\"deviceid\":" + String(deviceid) + ",";
  payload += "\"temp\":" + String(temp) + ",";
  payload += "\"hum\":" + String(hum) + ",";
  payload += "\"pressure\":" + String(pressure) + ",";
  payload += "\"pm1\":" + String(pm1) + ",";
  payload += "\"pm25\":" + String(pm25) + ",";
  payload += "\"pm10\":" + String(pm10) + ",";
  payload += "\"co2\":" + String(CO2);
  payload += "}";

  // Construct MQTT topic
  String topic = String(MQTT_BASE_TOPIC) + String(deviceid) + "/sensor";

  // Publish data to MQTT topic
  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
    Serial.printf("Published data to topic %s: %s\n", topic.c_str(), payload.c_str());
    return true;
  } else {
    Serial.println("Error: MQTT client not connected.");
    return false;
  }
}
