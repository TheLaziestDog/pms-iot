// ini code gua @gemasagara semua yang bikin

#include <Wire.h> // library buat komunikasi i2c
#include <LiquidCrystal_I2C.h> // library buat lcd i2c
#include "DHTesp.h" // library dht buat esp

DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2); //0x27 itu address lcd (buat ngecek pake yang i2c_address_scanner.ino), 16 itu kolom dari lcd nya, 2 itu baris dari lcd nya

void setup() {
  lcd.init();
  Serial.begin(9600);
  dht.setup(0, DHTesp::DHT11); // ini buat ngasih tau ke esp kalo sensor dht11 itu ada di GPIO 0 (D3)
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  // SERIAL MONITOR
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

  lcd.backlight(); // buat nyalain lampu backlight
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.setCursor(6, 0);
  lcd.print(temperature);

  // Kelembapan
  lcd.setCursor(0, 1);
  lcd.print("Kelembapan: ");
  lcd.setCursor(12, 1);
  lcd.print(humidity);
  
  delay(1000);
}
