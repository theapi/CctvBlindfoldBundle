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


// @see http://www.airspayce.com/mikem/arduino/VirtualWire_8h.html
#define RF_TX_PIN 12    // Pin 12 is the default sender pin.
#define DEBUG_LED_TX 13 // Debug led for tx

#define PIR_UP 2        // int.0 interupt 0 on pin 2
#define PIR_DOWN 3      // int.1 interupt 1 on pin 3

// Bit flags for pir state
#define F_UP 1          // 1 = activated  0 = no movement
#define F_DOWN 2        // 1 = activated  0 = no movement
#define F_UP_STAT 3     // 1 = new  0 = old
#define F_DOWN_STAT 4   // 1 = new  0 = old

unsigned long count = 0;

long previousMillis = 0;
long interval = 1000; 

long pirReset = 4000;
long lastPirUp = 0;
long lastPirDown = 0;

// Store the pir states on interupt.
volatile byte flags = 0;

void pirUp() 
{
  // Motion detected.
  bitSet(flags, F_UP);
  bitSet(flags, F_UP_STAT);
  Serial.println("UP detected");
  /*
   if (digitalRead(PIR_UP) == LOW) { // falling
     // No more motion.
     bitClear(flags, F_UP);
     Serial.println("UP off");
   } else {
     // Motion detected.
     bitSet(flags, F_UP);
     Serial.println("UP detected");
   }
   */
}

void pirDown() 
{
  // Motion detected.
  bitSet(flags, F_DOWN);
  bitSet(flags, F_DOWN_STAT);
  Serial.println("DOWN detected");
     
  /*
   if (digitalRead(F_DOWN) == LOW) { // falling
     // No more motion.
     bitClear(flags, F_DOWN);
     Serial.println("DOWN off");
   } else {
     // Motion detected.
     bitSet(flags, F_DOWN);
     Serial.println("DOWN detected");
     //Serial.println(bitRead(flags, F_DOWN));
   }
   */
}

void setup()
{
    pinMode(DEBUG_LED_TX, OUTPUT);
    
    pinMode(PIR_UP, INPUT);     // declare sensor as input
    pinMode(PIR_DOWN, INPUT);
    
    bitClear(flags, F_UP);
    bitClear(flags, F_DOWN);
     
  
    Serial.begin(9600);
    Serial.println("setup");
    
    vw_set_tx_pin(RF_TX_PIN);
    // Initialise the IO and ISR
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);      // Bits per sec
    
    
    attachInterrupt(0, pirUp, RISING);
    attachInterrupt(1, pirDown, RISING);

}

void loop()
{
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    // TODO: reset previousMillis when overruns max value.

    // Can't set use millis in the interupt.
    if (bitRead(flags, F_UP_STAT)) {
      lastPirUp = currentMillis;
      bitClear(flags, F_UP_STAT);
    }
    
    if (bitRead(flags, F_DOWN_STAT)) {
      lastPirDown = currentMillis;
      bitClear(flags, F_DOWN_STAT);
    }

    
    if (bitRead(flags, F_UP) && currentMillis - lastPirUp > pirReset) {
      bitClear(flags, F_UP);
      Serial.println("UP off");
    }
      
    if (bitRead(flags, F_DOWN) && currentMillis - lastPirDown > pirReset) {
      bitClear(flags, F_DOWN);
      Serial.println("DOWN off");
    }
      
    if (bitRead(flags, F_UP) || bitRead(flags, F_DOWN)) { 
      char buf[50];
      sprintf(buf, "count=%lu,flags=%u", count, flags); 
    
      //Serial.println(bitRead(flags, F_DOWN));
      //Serial.println(bitRead(flags, F_UP));
    
      if (bitRead(flags, F_UP)) { 
        strcat (buf, " UP ");
      }
      
      if (bitRead(flags, F_DOWN)) { 
        strcat (buf, " DOWN ");
      }
        
      Serial.println(buf);
    
      //const char *buf = "hello";
      digitalWrite(DEBUG_LED_TX, true); // Flash a light to show transmitting
      vw_send((uint8_t *)buf, strlen(buf));
      vw_wait_tx(); // Wait until the whole message is gone
      digitalWrite(DEBUG_LED_TX, false);
      
      count++;
    }
    
  }
  
  

}
