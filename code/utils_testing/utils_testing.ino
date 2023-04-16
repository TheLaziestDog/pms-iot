// UTILS TESTING

/*
  @owner TEAM NATECH
  @license GNU GENERAL PUBLIC LICENSE
  @github TheLaziestDog/pms-iot

  This codebase if used for testing all of the smart incubator utilities (excluding the IoT utilities)
  *THIS CODEBASE IS ONLY PERMITTED TO BE USED FOR TESTING AND CHECKING THE SENSORS AND TOOL CONDITION*
*/

// Importing Libraries

#include <SoftwareSerial.h>
#include "DHT.h"
#include "uRTCLib.h"
#include <SPI.h>

// DFRobot Libraries
#include "DFRobot_OxygenSensor.h"

// Variables

// RTC
uRTCLib rtcData(0x68); // The fixed i2c address of rtc is 0x68

// RELAY
const byte solenoid = 8;
const byte fan = 9;
const byte ledStrip = 10;

// CAPACITIVE SOIL MOISTURE
const byte soilMoisturePin = A1;
const int wetRange = 2;   //needs more calibartion
const int dryRange = 19;  //needs more calibartion

// SOIL NPK SENSOR
// RE and DE Pins set the RS485 module
// to Receiver or Transmitter mode
#define RE 8
#define DE 7

// Modbus RTU requests for reading NPK values
const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11]; // to store NPK values

// Sets up a new SoftwareSerial object
SoftwareSerial mod(2, 3); // 2 = RO pin, 3 = DI pin

const unsigned long npkSensorInterval = 250;
byte npkState = 0;

// O2 Sensor 
#define COLLECT_NUMBER    10             
#define Oxygen_IICAddress ADDRESS_3
/*   I2C slave Address, The default is ADDRESS_3.
       ADDRESS_0               0x70      // iic device address.
       ADDRESS_1               0x71
       ADDRESS_2               0x72
       ADDRESS_3               0x73
*/
DFRobot_OxygenSensor Oxygen;

// Co2 Sensor
const byte co2SensorPin = A3;

// Other Sensors
const byte dhtPin = 5;
const byte ldrPin = A2;
DHT dht(dhtPin, DHT11);

// Millis
unsigned long previousTime_0 = 0;
unsigned long previousTime_1 = 0;

void setup(){
  Serial.begin(115200);
  mod.begin(115200);

  // Relay
  pinMode(solenoid, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(ledStrip, OUTPUT);

  // SOIL NPK SENSOR
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  // Rtc
  URTCLIB_WIRE.begin();
  rtcData.set(0, 56, 12, 1, 2, 4, 23); // 0:12:56 PM 2/4/23
  // set day of week (1=Sunday, 7=Saturday)

  // Dht
  dht.begin();

  // Wait for everything to start on, especially for the dht11 because it's a really slow sensor
  delay(3000); 
} 

void loop(){
  // Millis
  unsigned long currentTime = millis();
  
  // Dht
  float temperatureData = dht.readTemperature();
  float humidityData = dht.readHumidity();

  // Ldr
  int brightnessData = analogRead(ldrPin);

  // O2 Sensor
  float oxygenData = Oxygen.getOxygenData(COLLECT_NUMBER);

  // CO2 Sensor
  int co2SVal = analogRead(co2SensorPin);
  float co2SVoltage = co2SVal*(5000/1024.0);
  float co2Data = getCo2Data(co2SVoltage);

  // Soil Moisture Sensor
  float soilMoistureValue = analogRead(soilMoisturePin);
  float soilMoisturePercent = map(soilMoistureValue, wetRange, dryRange, 0, 100);

  // Soil NPK Sensor
  byte nitroVal,phosVal,potaVal;
  if(currentTime - previousTime_1 >= npkSensorInterval){
    switch (npkState) {
    case 0:
      nitroVal = nitrogen();
      npkState++;
      break;
    case 1:
      phosVal = phosphorous();
      npkState++;
      break;
    case 2:
      potaVal = potassium();
      npkState = 0;
      break;
    }
    previousTime_1 = currentTime;
  }
  char npkBuffer[60];
  sprintf(npkBuffer, "Nitrogen : %dmg/kg, Phosphorus : %dmg/kg, Potassium : %dmg/kg", nitroVal, phosVal, potaVal);

  // Rtc
  rtcData.refresh();
  char dateTimeBuffer[20];
  sprintf(dateTimeBuffer, "%d/%d/%d %d:%d:%d", rtcData.year(), rtcData.month(), rtcData.day(), rtcData.hour(), rtcData.minute(), rtcData.second());
  
  // Error checking 
  if(isnan(temperatureData) || isnan(humidityData)){
    Serial.println(F("Fail to read data from DHT11 sensor"));
  }
  if(isnan(oxygenData)){
    Serial.println(F("Fail to read data from O2 sensor"));
  }
  if(isnan(co2Data)){
    Serial.println(F("Fail to read data from Co2 sensor"));
  }
  if(isnan(soilMoisturePercent)){
    Serial.println(F("Fail to read data from Soil Moisture sensor"));
  }

  // Monitoring
  Serial.println(dateTimeBuffer);

    // Dht
    Serial.print(F("Humidity (P): "));
    Serial.println(humidityData, 1);
    Serial.print(F("Temperature (C): "));
    Serial.println(temperatureData, 1);
    Serial.println();

    // O2 Sensor
    Serial.print(F("Oxygen Concentration (Vp): "));
    Serial.print(oxygenData);
    Serial.print(F("%vol"));
    Serial.println();

    // Co2 Sensor
    Serial.print(F("Co2 Concentration : "));
    Serial.println(co2Data);
    Serial.println();
    
    // Ldr
    Serial.print(F("Brightness Value (LDR): "));
    Serial.println(brightnessData);
    Serial.println();

    // Soil moisture
    if(soilMoisturePercent >= 100){
      Serial.println(F("Soil Moisture Value (P): 100%"));
    } else if (soilMoisturePercent <= 0){
      Serial.println(F("Soil Moisture Value (P): 0%"));
    } else { 
      Serial.print(F("Soil Moisture Value (P): "));
      Serial.print(soilMoisturePercent);
      Serial.println(F("%"));
    }
    Serial.print(F("Soil Moisture Value (R):"));
    Serial.println(soilMoistureValue);
    Serial.println();

    Serial.println(npkBuffer);
    Serial.println();
}

float getCo2Data(float voltage){
  float val = 0;
  
  if(voltage == 0)
  {
    Serial.print(F("CO2 Sensor Condition : "));
    Serial.println(F("Fault"));
  }
  else if(voltage < 400)
  {
    Serial.print(F("CO2 Sensor Condition : "));
    Serial.println(F("preheating"));
    val = 0;
  }
  else
  {
    int voltage_diference=voltage-400;
    val = voltage_diference*50.0/16.0;
  }
  return val;
}

byte nitrogen(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(nitro,sizeof(nitro))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte phosphorous(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(phos,sizeof(phos))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte potassium(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(pota,sizeof(pota))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}