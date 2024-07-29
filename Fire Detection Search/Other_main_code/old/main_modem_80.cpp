#include "Arduino.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_sleep.h>
#include "driver/adc.h"
#include "esp32-hal-cpu.h"


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
    //esp_sleep_enable_modem_sleep();
    	
//setCpuFrequencyMhz(40);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello World");
  configurerModeModemSleep();
  setCpuFrequencyMhz(80);
}

void loop() {
      // Obtenir la fréquence du processeur
  uint32_t freq = ESP.getCpuFreqMHz();

  // Afficher la fréquence sur le moniteur série
  Serial.print("Fréquence du processeur : ");
  Serial.print(freq);
  Serial.println(" MHz");

  delay(1000); // Attendre 1 seconde avant de répéter
  // put your main code here, to run repeatedly:
}