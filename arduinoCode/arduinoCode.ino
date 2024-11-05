#include <Wire.h>
#include "Seeed_SHT35.h"
#include "DFRobot_BME280.h"
#include "PMS.h"
#include "MHZ19.h"

#define SEA_LEVEL_PRESSURE 1013.0f

float temp, hum, pressure, pm1, pm25, pm10;
int CO2 = 0;

DFRobot_BME280_IIC bme(&Wire, 0x77);
PMS pms(Serial2);
PMS::DATA data;
MHZ19 myMHZ19;  // CO2 sensor

SHT35 sht35(21);  // Assuming SCL is connected to pin 21

unsigned long getDataTimer = 0;

void setup() {
  Serial.begin(9600);     // Debugging Serial
  Serial3.begin(115200);  // Data Transmission Serial
  sht35.init();

  // MH-Z19B setup
  Serial1.begin(9600);
  myMHZ19.begin(Serial1);
  myMHZ19.autoCalibration(true);

  // PMS5003 setup
  Serial2.begin(9600);
  pms.passiveMode();
  pms.wakeUp();

  // BME280 setup
  while (bme.begin() != DFRobot_BME280_IIC::eStatusOK) {
    Serial.println("BME280 initialization failed!");
    delay(2000);
  }
}

void loop() {
  // Read SHT35 (temperature & humidity)
  sht35.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &temp, &hum);

  // Read BME280 (pressure)
  pressure = bme.getPressure();

  // Read PMS5003 (PM1, PM2.5, PM10)
  pms.requestRead();
  if (pms.readUntil(data)) {
    pm1 = data.PM_AE_UG_1_0;
    pm25 = data.PM_AE_UG_2_5;
    pm10 = data.PM_AE_UG_10_0;
  }

  // Read MH-Z19B (CO2)
  if (millis() - getDataTimer >= 2000) {
    CO2 = myMHZ19.getCO2();
    getDataTimer = millis();
  }

  // Send data over UART3
  Serial3.print("T:"); Serial3.print(temp);
  Serial3.print("|H:"); Serial3.print(hum);
  Serial3.print("|P:"); Serial3.print(pressure);
  Serial3.print("|PM1:"); Serial3.print(pm1);
  Serial3.print("|PM25:"); Serial3.print(pm25);
  Serial3.print("|PM10:"); Serial3.print(pm10);
  Serial3.print("|CO2:"); Serial3.print(CO2);
  Serial3.println();

  delay(1000);  // Adjust delay as needed
}
