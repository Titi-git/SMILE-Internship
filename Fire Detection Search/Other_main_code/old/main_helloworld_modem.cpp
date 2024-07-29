#include "Arduino.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_sleep.h>


// Fonction pour configurer et activer le mode modem sleep
void configurerModeModemSleep() {
    // Désactiver le WiFi et le Bluetooth
    esp_wifi_stop();
    esp_bt_controller_disable();
    
    // Configurer les options de sommeil
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
    
    // Activer le mode modem sleep
    esp_sleep_enable_modem_sleep();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello World");
  configurerModeModemSleep();
}

void loop() {
  // put your main code here, to run repeatedly:
}