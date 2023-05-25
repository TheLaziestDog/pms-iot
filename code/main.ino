/*
  FINAL CODE of NATECH PMS PROJECT
  @owner TEAM NATECH
  @license GNU GENERAL PUBLIC LICENSE
  @github https://github.com/TheLaziestDog/pms-iot/
*/

/*
  PMS-IOT Features : 
    Monitoring  
    - Monitor plant & incubator
    - Shows the sensors data on a LCD display

    Control
    - Controls incubator temperature and humidity.
    - Regulates how much sunlight the plant will get.
    - Protects the plant from potential threats such as insects, pests, and diseases.

    Automation
    - Provide the plant with sufficient and appropriate nutrition at the right time based on the time and NPK sensor data.
    - Water the plant automatically based on the time, soil moisture, temperature, and humidity data that the sensor gather..

    IoT
    - Sends data sensors to multiple IoT platforms such as a custom spreadsheet database and Blynk IoT.
    - Regularly collect all of the (conditions) data automatically and systematically to provide the users with ready to use (matured) data for further analysis of the plant condition.
    - Can integrate with any existing system management.
*/

// Importing 
  #include <Arduino.h>
  #include <SoftwareSerial.h>
  #include <Wire.h> // I2C (Wire) Communication Library 
  #include <LiquidCrystal_I2C.h> // LCD I2C Library
  #include "DHTesp.h" // DHT Sensors Library For ESP

  // RTC Library
  #include "uRTCLib.h" // RTC Library
  #include <SPI.h> // This library is added to handle RTC library error

  // IoT Libraries
  #include <ESP8266WiFi.h>
  #include "HTTPSRedirect.h"

  // DFRobot Libraries
  #include "DFRobot_OxygenSensor.h" // O2 Sensor Library

// ==== Sensors & Utilities ====

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);
const unsigned long updateDisplayInterval = 1000;

int lcdSwitchTimer = 0;
int timerIncrement = 15;

// RTC
uRTCLib rtcData(0x68); // The fixed i2c address of rtc is 0x68

// CAPACITIVE SOIL MOISTURE
const byte soilMoisturePin = 2; // GPIO 2 = D4
const int wetRange = 239;   //needs more calibartion
const int dryRange = 595;  //needs more calibartion

// RELAY
const byte solenoid = 14; // GPIO 14 = D5
const byte fan = 12; // GPIO 12 = D6
const byte ledStrip = 13; // GPIO 13 = D7

// SOIL NPK SENSOR
// RE and DE Pins set the RS485 module
// to Receiver or Transmitter mode
#define RE 14 // GPIO 14 = D5
#define DE 12 // GPIO 12 = D6

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
/*   iic slave Address, The default is ADDRESS_3.
       ADDRESS_0               0x70      // iic device address.
       ADDRESS_1               0x71
       ADDRESS_2               0x72
       ADDRESS_3               0x73
*/
DFRobot_OxygenSensor Oxygen;

// Co2 Sensor
const byte co2SensorPin = 15; // GPIO 15 = D8

// Other Sensors
DHTesp dht;
const byte dhtPin = 16; // GPIO 16 = D0
const byte ldrPin = 0; // GPIO 0 = D3

// AnalogReadSensor
byte analogReadState = 0;
const unsigned long analogReadDelayInterval = 100;

// Millis
unsigned long previousTime_0 = 0;
unsigned long previousTime_1 = 0;
unsigned long previousTime_2 = 0;
unsigned long previousTime_3 = 0;

// ==== Climate Control ====

// Automatic watering condition
const byte drySoil = 16; // Dry soil value (on soil moisture sensor), NEEDS CALIBRATION
const byte wateringDuration = 1; // Automatic watering duration, NEEDS CALIBRATION
const byte wateringDayInterval = 6; // Watering every 6 days
bool isWateringTime = false;

// Orchid ideal temperature on day and night
const float dayTemp = 30.0;
const float nightTemp = 24.0;

// Orchid ideal humidity
const float idealHumidity = 70.0;

// ==== IoT ====
const char* ssid     = "gemaauraku";
const char* password = "sagaradhia121307";

// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbxS8SrBs6YozBn8yIJrLIJVnbr9Lip9SpMsfiMt_J57GC4frb-8CohRPuZR7PmxDpuk";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"ESP DATA LOGGING\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(10);
  Serial.println('\n');

  // ==== Sensors & Utilities ====

  // Relay
  pinMode(solenoid, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(ledStrip, OUTPUT);

  // Analog Sensors
  pinMode(ldrPin, OUTPUT);
  pinMode(soilMoisturePin, OUTPUT);
  pinMode(co2SensorPin, OUTPUT);

  // Dht
  dht.setup(dhtPin, DHTesp::DHT11);
  delay(dht.getMinimumSamplingPeriod());


  // Lcd
  lcd.init();
  lcd.backlight();
  
  // Rtc
  URTCLIB_WIRE.begin();
  //rtcData.set(DateTime(__DATE__, __TIME__));
  rtcData.set(0, 35, 16, 4, 24, 5, 23); // 0:12:56 PM 2/4/23
  // index starts from 1 not 0
  // rtcData.set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
  // set day of week (1=Sunday, 7=Saturday)
  previousTime_2 = rtcData.day();

  // ==== IoT ====
  
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       Serial.println("Connected");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  
  // Wait for everything to start on, the lcd, and especially for the dht11 because it's a really slow sensor
  delay(3000);
}

int analogReadSensor(byte pin){
  digitalWrite(pin, HIGH);
  switch (pin){
  case ldrPin:
    digitalWrite(soilMoisturePin, LOW);
    digitalWrite(co2SensorPin, LOW);
    break;
  case soilMoisturePin:
    digitalWrite(ldrPin, LOW);
    digitalWrite(co2SensorPin, LOW);
    break;
  case co2SensorPin:
    digitalWrite(soilMoisturePin, LOW);
    digitalWrite(ldrPin, LOW);
    break;
  }
  return analogRead(0);
}

void loop() {
  // ==== Variables Initialization ====
  
  // Millis
  unsigned long currentTime = millis();

  // Dht
  float temperatureData = dht.getTemperature();
  float humidityData = dht.getHumidity();

  // O2 Sensor
  float oxygenData = Oxygen.getOxygenData(COLLECT_NUMBER);

  // CO2 Sensor
  int co2SVal;
  float co2SVoltage;
  float co2Data;

  // Soil moisture
  float soilMoistureValue;
  float soilMoisturePercent;
  
  // Ldr
  int brightnessData;

  // IoT
  static bool flag = false;
  int usableBrightnessData;
  float usableSoilMoistureData;
  float usableCo2Data;

  // ==== Sensors Data Reading ====

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

  Serial.println();

  // Soil NPK Sensor
  byte nitroVal,phosVal,potaVal;
  /*if(currentTime - previousTime_1 >= npkSensorInterval){
    switch (npkState) {
    case 0:
      nitroVal = nitrogen();
      npkState++;
      Serial.println("Nitrogen Value Accessed Succesfully");
      break;
    case 1:
      phosVal = phosphorous();
      npkState++;
      Serial.println("Phosphorus Value Accessed Succesfully");
      break;
    case 2:
      potaVal = potassium();
      npkState = 0;
      Serial.println("Potassium Value Accessed Succesfully");
      break;
    }
    previousTime_1 = currentTime;
  }*/
  char npkRatBuffer[8];
  sprintf(npkRatBuffer, "%d-%d-%d", nitroVal, phosVal, potaVal);
  
  // Rtc
  rtcData.refresh();
  char dateTimeBuffer[20];
  sprintf(dateTimeBuffer, "%d/%d/%d %d:%d:%d", rtcData.year(), rtcData.month(), rtcData.day(), rtcData.hour(), rtcData.minute(), rtcData.second());

  Serial.println(dateTimeBuffer);
  Serial.println();
  
  // ==== LCD ====
  lcd.setCursor(0, 2);
  lcd.print(dateTimeBuffer);  

  lcd.setCursor(0, 3);
  lcd.print(F("DS.Flava ---- 80mdpl"));

  // Displaying Sensors Data
  if(lcdSwitchTimer < 45){
    if(lcdSwitchTimer < 15){
      lcd.setCursor(0, 0);
      lcd.print(F("Temp Humd Brightness"));
      
      brightnessData = analogReadSensor(ldrPin);
      usableBrightnessData = brightnessData;
      
      lcd.setCursor(0, 1);
      lcd.print(temperatureData, 1);

      lcd.setCursor(5, 1);
      lcd.print(humidityData, 1);

      lcd.setCursor(10, 1);
      lcd.print(brightnessData);
    } 
    else if(lcdSwitchTimer >= 15 && lcdSwitchTimer <= 30){
      lcd.setCursor(0, 0);
      lcd.print(F("   Oxygen     Co2   "));
      
      co2SVal = analogReadSensor(co2SensorPin);
      co2SVoltage = co2SVal*(5000/1024.0);
      co2Data = getCo2Data(co2SVoltage);

      usableCo2Data = co2Data;

      lcd.setCursor(3, 1);
      lcd.print("nan");
      lcd.print(F("%"));

      lcd.setCursor(14, 1);
      lcd.print("nan");
      lcd.print(F("%"));  
    } 
    else if(lcdSwitchTimer >= 30 && lcdSwitchTimer <= 45){
      lcd.setCursor(0, 0);
      lcd.print(F("Soil Moist NPK Ratio"));

      soilMoistureValue = analogReadSensor(soilMoisturePin);
      soilMoisturePercent = map(soilMoistureValue, wetRange, dryRange, 0, 100);

      if(soilMoisturePercent >= 100){
        soilMoisturePercent = 100;
      } else if (soilMoisturePercent <= 0){
        soilMoisturePercent = 0;
      }

      usableSoilMoistureData = soilMoisturePercent;

      lcd.setCursor(0, 1);
      lcd.print(soilMoisturePercent, 2);
      lcd.print(F("%"));

      lcd.setCursor(11, 1);
      lcd.print("N-P-K");
    } 
  } else {
    lcdSwitchTimer = 0;
  }

  // Refresh Display
  if(currentTime - previousTime_0 >= updateDisplayInterval){
    clearDisplay();
    lcdSwitchTimer++;
    previousTime_0 = currentTime;
  }

  // ==== Relay ====
  
  if(rtcData.day() - previousTime_2 >= wateringDayInterval){
    if(soilMoisturePercent >= drySoil){
      isWateringTime = true;
    }
    previousTime_2 = rtcData.day();
  }

  if(isWateringTime == true && rtcData.hour() == 12){
    if(rtcData.minute() >= wateringDuration+1){
      digitalWrite(solenoid, LOW);
      isWateringTime = false;
    } else {
      digitalWrite(solenoid, HIGH);
    }
  }

  if(rtcData.hour() >= 11 && rtcData.hour() <= 15){
    if(temperatureData >= dayTemp){
      digitalWrite(fan, HIGH);
    } else {
      digitalWrite(fan, LOW);
    }
  } 
  
  if(rtcData.hour() >= 19 && rtcData.hour() <= 1) {
    if(temperatureData >= nightTemp){
      digitalWrite(fan, HIGH);
    } else {
      digitalWrite(fan, LOW);
    }
  }

  if(rtcData.hour() >= 19 && rtcData.hour() <= 5){
    digitalWrite(ledStrip, HIGH);
  } else {
    digitalWrite(ledStrip, LOW);
  }

  // ==== IoT ====
  if(currentTime - previousTime_3 >= 60000){ // this means all of the data will be sent to the google sheets every 60000 miliseconds or 1 minute
    if (!flag){
      client = new HTTPSRedirect(httpsPort);
      client->setInsecure();
      flag = true;
      client->setPrintResponseBody(true);
      client->setContentTypeHeader("application/json");
    }
    if (client != nullptr){
      if (!client->connected()){
        client->connect(host, httpsPort);
      }
    }
    else{
      Serial.println("Error creating client object!");
    }
    
    // Create json object string to send to Google Sheets
    payload = payload_base + "\"" + temperatureData + "," + humidityData + "," + usableBrightnessData + "," + usableSoilMoistureData + "," + npkRatBuffer + "," + oxygenData + "," + co2Data + "\"}";
    
    // Publish data to Google Sheets
    Serial.println("Publishing data...");
    Serial.println(payload);
    if(client->POST(url, host, payload)){ 
      // do stuff here if publish was successful
    }
    else{
      // do stuff here if publish was not successful
      Serial.println("Error while connecting");
    }

    previousTime_3 = currentTime;
  }
  
}

float getCo2Data(float voltage){
  float val = 0;
  
  if(voltage == 0)
  {
    Serial.print(F("CO2 Sensor Condition : "));
    Serial.println(F("Fault"));
    Serial.println();
  }
  else if(voltage < 400)
  {
    Serial.print(F("CO2 Sensor Condition : "));
    Serial.println(F("preheating"));
    Serial.println();
    val = 0;
  }
  else
  {
    int voltage_diference=voltage-400;
    val = voltage_diference*50.0/16.0;
  }
  return val;
}

void clearDisplay(){
  for(int i = 0; i <= 3; i++){
    lcd.setCursor(0, i);
    lcd.print(F("                    "));
  }
}

// NPK Sensor
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
