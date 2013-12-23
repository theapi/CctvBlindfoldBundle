// Simple example of how to use VirtualWire to receive messages
// Implements a simplex (one-way) receiver with an Rx-B1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: receiver.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $
#include <VirtualWire.h>

// Pin 11 is the default receiver pin.
// @see http://www.airspayce.com/mikem/arduino/VirtualWire_8h.html#ae62b601260ae59e7e83c1e63ae0c064b

//TMP36 Pin Variables
int sensorPin = 0; //the analog pin the TMP36's Vout (sense) pin is connected to
long interval = 10000;
long previousMillis = 0;

void setup() {
  pinMode(13, OUTPUT);
  
  // initialize serial communication
  Serial.begin(9600);
  Serial.println("setup");
  
  // Initialise the IO and ISR
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);      // Bits per sec
  // vw_set_tx_pin(11);
  vw_rx_start();       // Start the receiver PLL running
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;  
    //getting the voltage reading from the temperature sensor
    int reading = analogRead(sensorPin);
    float temperatureC = (5.0 * reading * 100.0)/1024.0;
    Serial.print("T:");
    Serial.println(temperatureC); 
  }
  

    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
        int i;
        digitalWrite(13, true); // Flash a light to show received good message
        // Message with a good checksum received, dump it.
        Serial.print("R: ");
        
        for (i = 0; i < buflen; i++)
        {
            Serial.print(char(buf[i]));
            //Serial.print(" ");
        }
        Serial.println("");
        digitalWrite(13, false);
    }
  
  /*
  unsigned long currentMillis = millis();
  Serial.println(currentMillis);
  Serial.println("hello");
  delay(1000);
  */
}
