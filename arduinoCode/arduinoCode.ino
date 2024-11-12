#include <Wire.h>
#include "Seeed_SHT35.h"
#include "DFRobot_BME280.h"
#include "PMS.h"
#include "MHZ19.h"

#define SEA_LEVEL_PRESSURE 1013.0f

float temp, hum, pressure, pm1, pm25, pm10;
int CO2 = 0;

// Accumulators for averaging
float tempSum = 0, humSum = 0, pressureSum = 0;
float pm1Sum = 0, pm25Sum = 0, pm10Sum = 0;
int CO2Sum = 0;
int dataCount = 0;

bool dataReady = false;

DFRobot_BME280_IIC bme(&Wire, 0x77);
PMS pms(Serial2);
PMS::DATA data;
MHZ19 myMHZ19;

SHT35 sht35(21);

unsigned long dataTimer = 0;
unsigned long sendDataTimer = 0;

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  sht35.init();

  // MH-Z19B setup
  Serial1.begin(9600);
  myMHZ19.begin(Serial1);
  myMHZ19.autoCalibration(true);

  // PMS5003 setup
  Serial2.begin(9600);
  pms.passiveMode();
  pms.wakeUp();
  delay(100);  // Small delay for PMS5003 sensor wake-up

  // BME280 setup
  while (bme.begin() != DFRobot_BME280_IIC::eStatusOK) {
    Serial.println("BME280 initialization failed!");
    delay(2000);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Collect data every 3 seconds
  if (currentMillis - dataTimer >= 3000) {
    dataReady = false;

    // Read SHT35 (temperature & humidity)
    sht35.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &temp, &hum);
    tempSum += temp;
    humSum += hum;

    // Read BME280 (pressure)
    pressure = bme.getPressure();
    pressureSum += pressure;

    // Read PMS5003 (PM1, PM2.5, PM10)
    pms.requestRead();
    if (pms.readUntil(data)) {
      pm1 = data.PM_AE_UG_1_0;
      pm25 = data.PM_AE_UG_2_5;
      pm10 = data.PM_AE_UG_10_0;
      pm1Sum += pm1;
      pm25Sum += pm25;
      pm10Sum += pm10;
    }

    // Read MH-Z19B (CO2)
    CO2 = myMHZ19.getCO2();
    CO2Sum += CO2;

    // Increment data count and set flag
    dataCount++;
    dataReady = true;
    dataTimer = currentMillis;
  }

  // Ensure data is sent exactly every 15 seconds
  if (currentMillis - sendDataTimer >= 15000) {
    // Calculate averages or use last collected data if not enough samples
    float avgTemp = (dataCount > 0) ? (tempSum / dataCount) : temp;
    float avgHum = (dataCount > 0) ? (humSum / dataCount) : hum;
    float avgPressure = (dataCount > 0) ? (pressureSum / dataCount) : pressure;
    float avgPM1 = (dataCount > 0) ? (pm1Sum / dataCount) : pm1;
    float avgPM25 = (dataCount > 0) ? (pm25Sum / dataCount) : pm25;
    float avgPM10 = (dataCount > 0) ? (pm10Sum / dataCount) : pm10;
    int avgCO2 = (dataCount > 0) ? (CO2Sum / dataCount) : CO2;

    // Clear Serial buffer (optional but ensures no leftover data)
    while (Serial3.available()) {
      Serial3.read();
    }

    // Send data over UART3
    Serial3.print("T:"); Serial3.print(avgTemp);
    Serial3.print("|H:"); Serial3.print(avgHum);
    Serial3.print("|P:"); Serial3.print(avgPressure);
    Serial3.print("|PM1:"); Serial3.print(avgPM1);
    Serial3.print("|PM25:"); Serial3.print(avgPM25);
    Serial3.print("|PM10:"); Serial3.print(avgPM10);
    Serial3.print("|CO2:"); Serial3.print(avgCO2);
    Serial3.println();

    // Debugging output
    Serial.println(String(avgTemp) + " " + String(avgHum) + " " + String(avgPressure) + " " +
                   String(avgPM1) + " " + String(avgPM25) + " " + String(avgPM10) + " " + String(avgCO2));

    // Reset accumulators and counters
    tempSum = humSum = pressureSum = 0;
    pm1Sum = pm25Sum = pm10Sum = 0;
    CO2Sum = 0;
    dataCount = 0;
    dataReady = false;

    // Reset send data timer
    sendDataTimer = currentMillis;
  }
}
