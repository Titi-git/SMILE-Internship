#include "Arduino.h"
#include <esp_sleep.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello World");
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