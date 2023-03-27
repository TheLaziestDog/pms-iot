 /*
  @owner TEAM NATECH
  @license GNU GENERAL PUBLIC LICENSE
  This is the prototyping codebase for the NATECH PMS-IOT Project
  *THIS CODEBASE IS ONLY PERMITTED TO BE USED FOR PROTOTYPING PURPOSES*
*/

// Importing Libraries / Nge-import Library
   #include <Wire.h> // I2C (Wire) Communication Library / library komunikasi i2c  
   #include <LiquidCrystal_I2C.h> // LCD I2C Library / library lcd i2c
   #include "DHTesp.h" // DHT Sensors Library For ESP / library dht buat esp

   // RTC Library / Library RTC
   #include "RTClib.h"
   #include <SPI.h> // This library is added to handle RTC library error / Ini buat ngehandle error dari library RTC

   // Googlesheets Data Logging Libraries / Library Googlesheets
   #include "TRIGGER_WIFI.h"
   #include "TRIGGER_GOOGLESHEETS.h"

   // DFRobot Libraries
   #include "DFRobot_OxygenSensor.h" // O2 Sensor Library / Library sensor oksigen

DHTesp dht;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);
int lcdSwitchTimer = 0;

// Other Sensors
const byte dhtPin = 5; // GPIO 5 = D1
const byte ldrPin = 12; // GPIO 12 = D6

// RELAY
const byte solenoid = 0; // GPIO 0 = D3
const byte fan = 2; // GPIO 2 = D4
const byte ledStrip = 14; // GPIO 14 = D5

// CAPACITIVE SOIL MOISTURE
const int wetRange = 2;   //needs more calibartion
const int dryRange = 19;  //needs more calibartion
const byte soilMoisturePin = 13; // GPIO 13 = D7
float soilMoistureValue = 0;
float soilMoisturePercent=0;

// GOOGLE SHEETS
char columnsName[ ][20] = {"temperatureData", "humidityData", "soil", "brightness", "oxygen", "carbondioxide"};
String gasID = "AKfycbwh-JNCP1SYJSe2LjZOUHJNa1Hy_BhEK8AnbC5d_9Elgg6B0Djp_rUNEWOcLfXhlklB";                                         
int paramsAmount = 6;    

const char* userSSID = "Ondel-ondel"; // SSID 
const char* userPass = "qsdi6469"; // Password

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
float co2Concentration = 0;

String airCondition = "GOOD"; 

void setup() {
  Serial.begin(115200);

  // Relay
  pinMode(solenoid, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(ledStrip, OUTPUT);

  // Analog Sensors
  pinMode(ldrPin, OUTPUT);
  pinMode(soilMoisturePin, OUTPUT);
  pinMode(co2SensorPin, OUTPUT);

  // Lcd
  lcd.init();
  lcd.backlight();
  
  // Rtc
  rtc.begin(); 
  rtc.adjust(DateTime(__DATE__, __TIME__)); 
  // *IMPORTANT* THIS WILL SET THE RTC DATE & TIME AS THE DATE & TIME IN YOUR LAPTOP, NOT FROM THE RTC ITSELF
  
  // Dht
  dht.setup(dhtPin, DHTesp::DHT11);
  delay(dht.getMinimumSamplingPeriod());

  // Googlesheets
  WIFI_Connect(userSSID, userPass);                                                    
  Google_Sheets_Init(columnsName, gasID, paramsAmount );
}

int analogReadSensor(byte sensors){
  // 0 = LDR, 1 = soil moisture, 2 = co2
  if(sensors == ldrPin){
    digitalWrite(ldrPin, HIGH); 
    digitalWrite(soilMoisturePin, LOW);
    digitalWrite(co2SensorPin, LOW);  
  } else if(sensors == soilMoisturePin){
    digitalWrite(ldrPin, LOW); 
    digitalWrite(soilMoisturePin, HIGH);
    digitalWrite(co2SensorPin, LOW);  
  } else if(sensors == co2SensorPin){
    digitalWrite(ldrPin, LOW); 
    digitalWrite(soilMoisturePin, LOW);
    digitalWrite(co2SensorPin, HIGH);  
  }
  return analogRead(0);
}

float getCo2Data(float voltage){
  if(voltage == 0)
  {
    Serial.print("CO2 Sensor Condition : ");
    Serial.println("Fault");
  }
  else if(voltage < 400)
  {
    Serial.print("CO2 Sensor Condition : ");
    Serial.println("preheating");
  }
  else
  {
    int voltage_diference=voltage-400;
    co2Concentration=voltage_diference*50.0/16.0;
  }
  return co2Concentration;
}

void loop() {
  // Dht
  float temperatureData = dht.getTemperature();
  float humidityData = dht.getHumidity();

  // O2 Sensor
  float oxygenData = Oxygen.getOxygenData(COLLECT_NUMBER);

  // CO2 Sensor
  int co2SVal = analogReadSensor(co2SensorPin);
  float co2SVoltage = co2SVal*(5000/1024.0);
  float co2Data = getCo2Data(co2SVoltage);

  delay(100); // to prevent data contradiction from sensor multiplexing

  // Soil moisture
  soilMoistureValue = analogReadSensor(soilMoisturePin);
  soilMoisturePercent = map(soilMoistureValue, wetRange, dryRange, 0, 100);

  delay(100); // to prevent data contradiction from sensor multiplexing

  // Ldr
  int brightnessData = analogReadSensor(ldrPin);

  // Rtc
  DateTime rtcData = rtc.now();

  // Google Sheets
  Data_to_Sheets(paramsAmount,  temperatureData,  humidityData, soilMoisturePercent, brightnessData, oxygenData, co2Data);
  
  // Reset lcd
  lcd.clear();

  // Serial Monitor
    // Rtc
    Serial.print(rtcData.year(), DEC);
    Serial.print('/');
    Serial.print(rtcData.month(), DEC);
    Serial.print('/');
    Serial.print(rtcData.day(), DEC);
    Serial.print(' ');
    Serial.print(rtcData.hour(), DEC);
    Serial.print(':');
    Serial.print(rtcData.minute(), DEC);
    Serial.print(':');
    Serial.println(rtcData.second(), DEC);
    Serial.println("");

    // Dht
    Serial.print("DHT Sensor Status : ");
    Serial.println(dht.getStatusString());
    Serial.print("Humidity (P): ");
    Serial.println(humidityData, 1);
    Serial.print("Temperature (C): ");
    Serial.println(temperatureData, 1);
    Serial.println();

    // O2 Sensor
    Serial.print("Oxygen Concentration (Vp): ");
    Serial.print(oxygenData);
    Serial.print("%vol");
    Serial.println();

    // Co2 Sensor
    Serial.print("Co2 Concentration : ");
    Serial.println(co2Data);
    Serial.println();
    
    // Ldr
    Serial.print("Brightness Valur (LDR): ");
    Serial.println(brightnessData);
    Serial.println();

    // Soil moisture
    if(soilMoisturePercent >= 100){
      Serial.print("Soil Moisture Value (P): ");
      Serial.println("100%");
    } else if (soilMoisturePercent <= 0){
      Serial.print("Soil Moisture Value (P): ");
      Serial.println("0%");
    } else { 
      Serial.print("Soil Moisture Value (P): ");
      Serial.print(soilMoisturePercent);
      Serial.println("%");
    }
    Serial.print("Soil Moisture Value (R):");
    Serial.println(soilMoistureValue);
    Serial.println();
  
  // LCD
    // Date & Time
    lcd.setCursor(0, 2);
    lcd.print(rtcData.day(), DEC);
    lcd.print('/');
    lcd.print(rtcData.month(), DEC);
    lcd.print('/');
    lcd.print(rtcData.year(), DEC);
    lcd.setCursor(10, 2);
    lcd.print('-');
    lcd.setCursor(12, 2);
    lcd.print(rtcData.hour(), DEC);
    lcd.print(':');
    lcd.print(rtcData.minute(), DEC);
    lcd.print(':');
    lcd.print(rtcData.second(), DEC);
    
    // Menampilkan data sensor
    if(lcdSwitchTimer < 15000){
      // RESET
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(0, 1);
      lcd.print("                    ");

      lcd.setCursor(0, 0);
      lcd.print("Temp Humd Brightness");

      lcd.setCursor(0, 1);
      lcd.print(temperatureData, 1);

      lcd.setCursor(5, 1);
      lcd.print(humidityData, 1);
      
      lcd.setCursor(10, 1);
      lcd.print(brightnessData);

    } else if(lcdSwitchTimer >= 15000){
      // RESET
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(0, 1);
      lcd.print("                    ");

      lcd.setCursor(0, 0);
      lcd.print("   Oxygen     Co2   ");
      
      lcd.setCursor(0, 1);
      lcd.print(soilMoisturePercent, 0);
      lcd.print("%");
      
      lcd.setCursor(5, 1);
      lcd.print("NAN");
      
      lcd.setCursor(9, 1);
      lcd.print(oxygenData, 1); 

      lcd.setCursor(16, 1);
      lcd.print(co2Data, 1); 
      
    } else if(lcdSwitchTimer >= 30000){
      // RESET
      lcd.setCursor(0, 0);
      lcd.print("                    ");
      lcd.setCursor(0, 1);
      lcd.print("                    ");

      lcd.setCursor(0, 0);
      lcd.print("Soil Moist NPK Ratio");
      
      lcd.setCursor(0, 1);
      lcd.print(soilMoisturePercent, 0);
      lcd.print("%");
      
      lcd.setCursor(5, 1);
      lcd.print("NAN");
      
      lcd.setCursor(9, 1);
      lcd.print(oxygenData, 1); 

      lcd.setCursor(16, 1);
      lcd.print(co2Data, 1); 
      
    } else if(lcdSwitchTimer >= 45000){
      lcdSwitchTimer = 0;
    }

    // Jenis anggrek (+karakteristiknya)
    lcd.setCursor(0, 3);
    lcd.print("DS.Flava ---- 80mdpl");

  // Relay

  // Solenoid
  relay(rtcData, solenoid, 0, 2, 20, 2); // solenoid, NO config, di jam 14:20, selama 2 menit
  
  delay(800);
  lcdSwitchTimer += 1000;
}

void relay(DateTime dt, byte pin, byte configure, byte actHour, byte actMinute, byte duration){
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

  if(dt.hour() == actHour && dt.minute() == actMinute){
    digitalWrite(pin, activate);
    Serial.println("RELAY ON");
    Serial.println("");
    }
    
    else if(dt.hour() == actHour && dt.minute() == actMinute+duration+1){
      digitalWrite(pin, inactive);
      Serial.println("RELAY OFF");
      Serial.println("");
    } else {
      Serial.println("RELAY INACTIVE");
      Serial.println("");
    }
}
