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

// RELAY
const byte dhtPin = 12; // GPIO 12 = D6
const byte solenoid = 0;
const byte fan = 2;
const byte ledStrip = 14;

const int OnHour = 7; //SET TIME TO ON RELAY (24 HOUR FORMAT)
const int OnMin = 23;
const int OffHour = 7; //SET TIME TO OFF RELAY
const int OffMin = 25 ;

// CAPACITIVE SOIL MOISTURE
const int wetRange = 2;   //perlu kalibrasi ulang
const int dryRange = 19;  //perlu kalibrasi ulang
const int SensorPin = A0;
float soilMoistureValue = 0;
float soilmoisturepercent=0;

// GOOGLE SHEETS
char column_name_in_sheets[ ][20] = {"temperature","humidity"};                        /*1. The Total no of column depends on how many value you have created in Script of Sheets;2. It has to be in order as per the rows decided in google sheets*/
String Sheets_GAS_ID = "AKfycbyGk5g3AZDWbjOOKE0uMHEvotC6vcWB6yQwJq3QUxi1-gqchcf8py7jIx26QJAv9d_H";                                         /*This is the Sheets GAS ID, you need to look for your sheets id*/
int No_of_Parameters = 2;   
const char* userSSID = "Ondel-ondel"; // SSID 
const char* userPass = "qsdi6469"; // Password

// O2, CO2, ETC
String airCondition;

void setup() {
  Serial.begin(9600); 

  // Relay
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
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  // Rtc
  DateTime waktu = rtc.now();
  Data_to_Sheets(No_of_Parameters,  temperature,  humidity);

  // Oxygen & Carbon Dioxide Sensor (belom ada apa-apa)
  airCondition = "GOOD";

  // Soil moisture
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, wetRange, dryRange, 0, 100);

  // Reset lcd
  lcd.clear();

  // Serial Monitor

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
  Serial.print(waktu.second(), DEC);
  Serial.println();

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
   if(waktu.hour() == OnHour && waktu.minute() == OnMin){
    digitalWrite(solenoid, LOW);
   }

  if(waktu.hour() == OnHour && waktu.minute() == OnMin){
    digitalWrite(solenoid, HIGH);
    Serial.println("SOLENOID ON");
    }
    
    else if(waktu.hour() == OffHour && waktu.minute() == OffMin){
      digitalWrite(solenoid, LOW);
      Serial.println("SOLENOID OFF");
    }   
  
  delay(1000);
}
