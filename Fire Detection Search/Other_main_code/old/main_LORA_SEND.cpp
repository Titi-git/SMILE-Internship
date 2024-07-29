#include "Arduino.h"

#define RX_Pin 16
#define TX_Pin 17
#define BAUDRATE  115200


//SoftwareSerial Lora(RX_Pin, TX_Pin);//  RX, TX


void setup() {
    Serial.begin(BAUDRATE);
    Serial2.begin(BAUDRATE,SERIAL_8N1,RX_Pin,TX_Pin);
    String cmd = "AT+BAND=868000000";
    Serial2.println(cmd);
    //Lora.println(cmd);
    if(Serial2.available()>0){
        Serial.write(Serial2.read());
    }
    delay(10);

    cmd = "AT+BAND?";
    Serial2.println(cmd);
    //Lora.println(cmd);
    if(Serial2.available()>0){
        Serial.write(Serial2.read());
    }
    delay(100);

    cmd = "AT+PARAMETER=12,7,1,4";
    Serial2.println(cmd);
    //Lora.println(cmd);
    if(Serial2.available()>0){
        Serial.write(Serial2.read());
    }
    delay(10);

    cmd = "AT+PARAMETER?";
    Serial2.println(cmd);
    //Lora.println(cmd);
    if(Serial2.available()>0){
        Serial.write(Serial2.read());
    }
    delay(10);

}

void loop() {
    String cmd = "AT+SEND=1,5,HELLO";
    Serial2.println(cmd);
    //Lora.println(cmd);
    while(Serial2.available()>0){
        Serial.write(Serial2.read());
    }
    delay(1000);
}