#include <Wire.h>
#include "Seeed_SHT35.h"
#include "DFRobot_BME280.h"
#include "PMS.h"
#include "MHZ19.h"

#define SEA_LEVEL_PRESSURE 1013.0f
float temp;
float hum;
float pressure;
float alt;
float pm1;
float pm25;
float pm10;

String str = "";

DFRobot_BME280_IIC bme(&Wire, 0x77);

PMS pms(Serial2);
PMS::DATA data; 

MHZ19 myMHZ19;                                             // Constructor for library
unsigned long getDataTimer = 0;
SHT35 sht35(21);  // Assuming SCL is connected to pin 21

void setup() {
  Serial.begin(9600);
  sht35.init();
  //MH-Z19B
  Serial1.begin(9600);  //MH-Z19B Serial
  myMHZ19.begin(Serial1);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration(true);                              // Turn auto calibration ON (OFF autoCalibration(false))
  //PMS5003
  Serial2.begin(9600);  //PMS5003 Serial
  pms.passiveMode();    // Switch to passive mode
  pms.wakeUp();
  //BME280
  while (bme.begin() != DFRobot_BME280_IIC::eStatusOK) {
        Serial.println("bme begin failed");
        delay(2000);
    }
  Serial3.begin(115200);
}

void loop() {
  sht35.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &temp, &hum);
  pressure = bme.getPressure();
  alt = bme.calAltitude(SEA_LEVEL_PRESSURE, pressure);

  //PMS5003 Readings
  //pms.wakeUp();
  //delay(2000);
  pms.requestRead();
  if (pms.readUntil(data))
  {
    pm1 = data.PM_AE_UG_1_0;
    pm25 = data.PM_AE_UG_2_5;
    pm10 = data.PM_AE_UG_10_0;
  }
  //pms.sleep();

  static int CO2=0;
  int8_t temp2;
  if (millis() - getDataTimer >= 2000){
        CO2 = myMHZ19.getCO2();  // Request CO2 (as ppm)                                
        getDataTimer = millis();
   }

  //Serial Printing Data
  Serial.println("------xxxxxxxxxxxxxxxxxxxxxxxx--------");
  Serial.print("Temp: ");
  Serial.println(temp);
  Serial.print("Humi: ");
  Serial.println(hum);
  Serial.print("Air Pressure: ");
  Serial.println(pressure);
  Serial.print("Altitude: ");
  Serial.println(alt);
  Serial.print("PM1.0: ");
  Serial.println(pm1);
  Serial.print("PM2.5: ");
  Serial.println(pm25);
  Serial.print("PM10.0: ");
  Serial.println(pm10);
  Serial.print("CO2: ");
  Serial.println(CO2);

  delay(1000);
}
