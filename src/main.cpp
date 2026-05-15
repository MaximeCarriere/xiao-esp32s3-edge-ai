#include <Arduino.h>

// On the XIAO ESP32-S3, the user LED is on GPIO 21 and is ACTIVE LOW.
// That means writing LOW turns it ON, HIGH turns it OFF.
#define LED_PIN 21

void setup()
{
  Serial.begin(115200);
  delay(1000); // give USB-CDC a moment to enumerate
  Serial.println("hello from XIAO ESP32-S3 Sense");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // start with LED off
}

void loop()
{
  digitalWrite(LED_PIN, LOW); // LED on
  Serial.println("blink: on");
  delay(500);

  digitalWrite(LED_PIN, HIGH); // LED off
  Serial.println("blink: off");
  delay(500);
}