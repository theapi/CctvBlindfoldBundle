// transmitter.pde
//
// Simple example of how to use VirtualWire to transmit messages
// Implements a simplex (one-way) transmitter with an TX-C1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: transmitter.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $
#include <VirtualWire.h>

// Pin 12 is the default sender pin.
// @see http://www.airspayce.com/mikem/arduino/VirtualWire_8h.html#ae62b601260ae59e7e83c1e63ae0c064b

unsigned long count = 0;

void setup()
{
    pinMode(13, OUTPUT);
  
    Serial.begin(9600);
    Serial.println("setup");
    // Initialise the IO and ISR
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);      // Bits per sec

}
void loop()
{
    unsigned long currentMillis = millis();
    char buf[50];
    sprintf(buf, "count=%lu,millis=%lu", count, currentMillis); 
    //sprintf(buf, "count=%lu", count); 
  
  
    //const char *buf = "hello";
    digitalWrite(13, true); // Flash a light to show transmitting
    vw_send((uint8_t *)buf, strlen(buf));
    vw_wait_tx(); // Wait until the whole message is gone
    digitalWrite(13, false);
    
    count++;
    delay(1000);
}
