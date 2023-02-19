    /*
     * I2C Scanner
     * THIS ONLY WORK FOR ARDUINO DEVICES, SUCH AS THE ARDUINO UNO
     * Referenced from Elangsakti.com
     */
     
    #include <Wire.h>
     
    void setup() {
        Wire.begin();
        Serial.begin(9600);
        while(!Serial);
     
        Serial.println("#==============o0o===============#");
        Serial.println("#           I2C Scanner          #");
        Serial.println("# Referenced from Elangsakti.com #");
        Serial.println("#================================#");
     
        Cari_Alamat();
    }
     
    void loop() { }
     
    void Cari_Alamat()  {
        byte respon, alamat, modul = 0;
     
        Serial.println("Scanning...");
        Serial.println();
        
        for(alamat=0; alamat<127; alamat++){
            Wire.beginTransmission(alamat);
            respon = Wire.endTransmission();
            switch(respon){
                case 0:
                    Serial.print("  ");
                    Serial.print(modul+1);
                    Serial.print(". Alamat = 0x");
                    if( alamat < 16 ) Serial.print("0");
                    Serial.println(alamat, HEX);
                    modul++;
     
                    break;
     
                case 4:
                    Serial.print("  - Error ");
                    if( alamat < 16 ) Serial.print("0");
                    Serial.println(alamat, HEX);
            }
        }
        Serial.println();
     
        if(modul > 0){
            Serial.print("Ditemukan ");
            Serial.print(modul);
            Serial.println(" modul I2C.");
        }else{
            Serial.println("Tidak ada modul I2C.");
        }
     
        delay(2000);
    }
