//#include <esp_sleep.h>
#include "Arduino.h"
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  5

RTC_DATA_ATTR int bootCount = 0;


//enable GPIO 32 as an output
#define GPIO_ALIM_PIN 32


void print_wakeup_reason(){
   esp_sleep_wakeup_cause_t source_reveil;

   source_reveil = esp_sleep_get_wakeup_cause();

   switch(source_reveil){
      case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Réveil causé par un signal externe avec RTC_IO"); break;
      case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Réveil causé par un signal externe avec RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Réveil causé par un timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Réveil causé par un touchpad"); break;
      default : Serial.printf("Réveil pas causé par le Deep Sleep: %d\n",source_reveil); break;
   }
}

void setup(){
   Serial.begin(9600);
   //Configure GPIO32 comme sortie
    pinMode(GPIO_ALIM_PIN, OUTPUT);
    //Allume le GPIO32
    digitalWrite(GPIO_ALIM_PIN, HIGH);

   ++bootCount;
   Serial.println("----------------------");
   Serial.println(String(bootCount)+ "eme Boot ");

   //Affiche la raison du réveil
   print_wakeup_reason();

   //Configuration du timer
   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
   Serial.println("ESP32 réveillé dans " + String(TIME_TO_SLEEP) + " seconds");

   //Rentre en mode Deep Sleep
   Serial.println("Rentre en mode Deep Sleep");
   Serial.println("----------------------");
   delay(2000);
   esp_deep_sleep_start();
   Serial.println("Ceci ne sera jamais affiché");
}

void loop(){
}