#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#define WIFI_SSID "IUB_BUS_Shuttle"
#define WIFI_PASSWORD "fabber@Bus"

// MQTT server details
#define MQTT_HOST IPAddress(103, 237, 39, 27)
#define MQTT_PORT 1883

#define MQTT_BASE_TOPIC "Zihan/device"
int deviceid = 2;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;
Ticker dataSendTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

String dataBuffer = "";
bool dataReady = false;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP8266 setup");

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  // Set up a timer to send data every 15 seconds
  dataSendTimer.attach(15, sendData);
}

void loop() {
  // Read data from Arduino Mega (UART3)
  while (Serial.available()) {
    char c = Serial.read();
    dataBuffer += c;

    if (c == '\n') {
      dataReady = true;
    }
  }
}

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
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
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
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void sendData() {
  if (dataReady) {
    float temp, hum, pressure, pm1, pm25, pm10;
    int CO2;

    int result = sscanf(dataBuffer.c_str(), "T:%f|H:%f|P:%f|PM1:%f|PM25:%f|PM10:%f|CO2:%d", &temp, &hum, &pressure, &pm1, &pm25, &pm10, &CO2);

    if (result == 7) {
      char payload[200];
      snprintf(payload, sizeof(payload), "{\"deviceid\":%d,\"temp\":%.2f,\"hum\":%.2f,\"pressure\":%.2f,\"pm1\":%.2f,\"pm25\":%.2f,\"pm10\":%.2f,\"co2\":%d}",
               deviceid, temp, hum, pressure, pm1, pm25, pm10, CO2);

      char topic[50];
      snprintf(topic, sizeof(topic), "%s%d/sensor", MQTT_BASE_TOPIC, deviceid);

      mqttClient.publish(topic, 1, true, payload);
      Serial.printf("Published data to topic %s: %s\n", topic, payload);
    } else {
      Serial.println("Error: Invalid data format.");
    }

    // Clear buffer and reset flag
    dataBuffer = "";
    dataReady = false;
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged. PacketId: ");
  Serial.println(packetId);
}
