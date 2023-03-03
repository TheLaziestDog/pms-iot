#include <Wire.h> // library buat komunikasi i2c
#include <LiquidCrystal_I2C.h> // library buat lcd i2c
#include "DHTesp.h" // library dht buat esp
#include <SPI.h>  // not used here, but needed to prevent a RTClib compile error
#include "RTClib.h"

DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 20, 4); //0x27 itu address lcd (buat ngecek pake yang i2c_address_scanner.ino), 16 itu kolom dari lcd nya, 2 itu baris dari lcd nya

const byte dhtPin = 0; // GPIO 0 = D3
RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  lcd.init();
  rtc.begin();  // Init RTC
  rtc.adjust(DateTime(__DATE__, __TIME__)); // ini ngeset waktu RTC nya ke laptop kita
  dht.setup(dhtPin, DHTesp::DHT11); // ini buat ngasih tau ke esp kalo sensor dht11 itu ada di GPIO 0 (D3)
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());

  DateTime waktu = rtc.now();
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  //* Serial Monitor
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
  */

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
    
  lcd.setCursor(2, 0);
  lcd.print("SMART  INCUBATOR")

  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  
  lcd.setCursor(6, 1);
  lcd.print(waktu.year(), DEC);
  lcd.print(":");
  lcd.print(waktu.month(), DEC);
  lcd.print(":");
  lcd.print(waktu.day(), DEC);

  lcd.backlight(); // buat nyalain lampu backlight
  lcd.setCursor(0, 2);
  lcd.print("Temperature: ");
  lcd.setCursor(13, 2);
  lcd.print(temperature);

  // Kelembapan
  lcd.setCursor(0, 3);
  lcd.print("Moisture: ");
  lcd.setCursor(10, 3);
  lcd.print(humidity);
  
  delay(1000);
}
