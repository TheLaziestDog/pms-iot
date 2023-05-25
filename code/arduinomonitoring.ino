/*
  All right reserved 2023 - GNU GENERAL PUBLIC LICENSE
  Ini adalah main (general) codebase dari project inkubator anggrek tim NATECH
  Codebase ini berisikan code untuk :
    - Semua utility (lcd, relay, dll) yang dipakai
    - Semua sensor yang dipakai
    - Komunikasi dari sensor ke Google Sheets
*/

// List library
#include <Wire.h> // library komunikasi i2c 
#include <LiquidCrystal_I2C.h> // library lcd i2c
#include "DHTesp.h" // library dht buat esp

// Library RTC
#include "RTClib.h"
#include <SPI.h>

// Library Googlesheets
#include "TRIGGER_WIFI.h"
#include "TRIGGER_GOOGLESHEETS.h"

DHTesp dht;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);
/* 0x27 itu alamat si lcd nya
 * 20 itu jumlah kolom lcd nya (horizontal)
 * 4 itu jumlah baris lcd nya (vertical)
 */

// Other Sensors
const byte dhtPin = 12; // GPIO 12 = D6
const byte ldrSensor = 13; // GPIO 13 = D7

// RELAY
const byte solenoid = 0; // GPIO 0 = D3
const byte fan = 2;
const byte ledStrip = 14;

// CAPACITIVE SOIL MOISTURE
const int wetRange = 2;   //perlu kalibrasi ulang
const int dryRange = 19;  //perlu kalibrasi ulang
const int SensorPin = A0;
float soilMoistureValue = 0;
float soilmoisturepercent=0;

// GOOGLE SHEETS
char column_name_in_sheets[ ][20] = {"Temperature","Humidity", "Soil"};
String Sheets_GAS_ID = "AKfycbzlx085YE-C8r64-WFSf_1wxEvAsHJzxyPoHthucdWyufXj-FG7_EDWFruUPjpTakc";                                         
int No_of_Parameters = 3;    

const char* userSSID = "Ondel-ondel"; // SSID 
const char* userPass = "qsdi6469"; // Password

// O2, CO2, ETC
String airCondition;

void setup() {
  Serial.begin(9600);

  // Relay
  pinMode(solenoid, OUTPUT);
  pinMode(solenoid, OUTPUT);
  pinMode(solenoid, OUTPUT);

  // Lcd
  lcd.init();
  lcd.backlight();
  
  // Rtc
  rtc.begin(); 
  rtc.adjust(DateTime(__DATE__, __TIME__)); // ini ngeset waktu RTC nya ke laptop kita
  
  // Dht
  dht.setup(dhtPin, DHTesp::DHT11); // ini buat ngasih tau ke esp kalo sensor dht11 itu ada di GPIO 12 (D6)
  delay(dht.getMinimumSamplingPeriod());

  // Googlesheets
  WIFI_Connect(userSSID, userPass);                                                    
  Google_Sheets_Init(column_name_in_sheets, Sheets_GAS_ID, No_of_Parameters );
}

void loop() {
  // Dht
  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  // Ldr
  int ldrValue = analogRead(ldrSensor);

  // Rtc
  DateTime waktu = rtc.now();

  // Oxygen & Carbon Dioxide Sensor (belom ada apa-apa)
  airCondition = "GOOD";

  // Soil moisture
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, wetRange, dryRange, 0, 100);

  // Google Sheets
  Data_to_Sheets(No_of_Parameters,  temperature,  humidity, soilmoisturepercent);
  
  // Reset lcd
  lcd.clear();

  // Serial Monitor

  // Rtc
  Serial.print(waktu.year(), DEC);
  Serial.print('/');
  Serial.print(waktu.month(), DEC);
  Serial.print('/');
  Serial.print(waktu.day(), DEC);
  Serial.print(' ');
  Serial.print(waktu.hour(), DEC);
  Serial.print(':');
  Serial.print(waktu.minute(), DEC);
  Serial.print(':');
  Serial.println(waktu.second(), DEC);
  Serial.println("");

  // Dht
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);

  // Soil moisture
  if(soilmoisturepercent >= 100){
    Serial.print("Soil Moisture (P): ");
    Serial.println("100%");
  } else if (soilmoisturepercent <= 0){
    Serial.print("Soil Moisture (P): ");
    Serial.println("0%");
  } else { 
    Serial.print("Soil Moisture (P): ");
    Serial.print(soilmoisturepercent);
    Serial.println("%");
  }
  Serial.print("Soil Moisture (V):");
  Serial.println(soilMoistureValue);

  // Ldr
  Serial.print("Brightness: ");
  Serial.println(ldrValue);
  Serial.println("");
  
  // LCD

  // Date & Time
  lcd.setCursor(0, 2);
  lcd.print(waktu.day(), DEC);
  lcd.print('/');
  lcd.print(waktu.month(), DEC);
  lcd.print('/');
  lcd.print(waktu.year(), DEC);
  lcd.setCursor(10, 2);
  lcd.print('-');
  lcd.setCursor(12, 2);
  lcd.print(waktu.hour(), DEC);
  lcd.print(':');
  lcd.print(waktu.minute(), DEC);
  lcd.print(':');
  lcd.print(waktu.second(), DEC);
  
  // Menampilkan data sensor
  lcd.setCursor(0, 0);
  lcd.print("Air  Soil  Temp Humd");

  lcd.setCursor(0, 1);
  lcd.print(airCondition);
  
  lcd.setCursor(5, 1);
  lcd.print(soilmoisturepercent, 0);
  lcd.print("%");
  
  lcd.setCursor(11, 1);
  lcd.print(temperature, 1);
  
  lcd.setCursor(16, 1);
  lcd.print(humidity, 1);

  // Jenis anggrek (+karakteristiknya)
  lcd.setCursor(0, 3);
  lcd.print("DS.Flava ---- 80mdpl");

  // Relay

  // Solenoid
  relay(waktu, solenoid, 0, 2, 20, 2); // solenoid, NO config, di jam 14:20, selama 2 menit
  
  delay(1000);
}

void relay(DateTime param, byte pin, byte configure, byte actHour, byte actMinute, byte duration){
  // NO = 0, NC = 1

  int activate;
  int inactive;
  Serial.print("MODE ");
  Serial.println(inactive);

  if(configure == 0){
    activate = HIGH; // HIGH
    inactive = LOW; // LOW
  } else {
    activate = LOW; // LOW
    inactive = HIGH; // HIGH
  }

  if(param.hour() == actHour && param.minute() == actMinute){
    digitalWrite(pin, activate);
    Serial.println("RELAY ON");
    Serial.println("");
    }
    
    else if(param.hour() == actHour && param.minute() == actMinute+duration+1){
      digitalWrite(pin, inactive);
      Serial.println("RELAY OFF");
      Serial.println("");
    } else {
      Serial.println("RELAY INACTIVE");
      Serial.println("");
    }
}
