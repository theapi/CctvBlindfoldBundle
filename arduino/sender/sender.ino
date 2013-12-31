/*
 One PIR sensor detects motion at the bottom of the stairs: DOWN.
 Another detects motion at the top of the stairs: UP.
 
 Sends an RF message when one both sensors have been triggered,
 indicating the direction of travel.
 The message is sent multiple times to give it the best chance of getting through.
*/

#include <VirtualWire.h>


// @see http://www.airspayce.com/mikem/arduino/VirtualWire_8h.html
#define RF_TX_PIN 9    // Pin 12 is the default sender pin so change it here.
#define RF_POWER_PIN 10 // Provide power to the transmitter
#define DEBUG_LED_TX 13 // Debug led for tx
#define DEBUG_LED_PULSE 12 // Debug led for alive pulse (tmp)
#define DEBUG_LED_MOTION 11

#define PIR_UP_PIN 2        // int.0 interupt 0 on pin 2
#define PIR_DOWN_PIN 3      // int.1 interupt 1 on pin 3

// Bit flags for pir state
#define F_UP 1          // 1 = activated  0 = no movement
#define F_DOWN 2        // 1 = activated  0 = no movement
#define F_UP_STAT 3     // 1 = new  0 = old
#define F_DOWN_STAT 4   // 1 = new  0 = old

unsigned long count = 0;

long previousMillis = 0;
long interval = 1000; 

long pirReset = 3000; // How long to ignore further detects.
long lastPirUp = 0;
long lastPirDown = 0;

boolean pulse = false;


// Store the pir states on interupt.
volatile byte flags = 0;

// Interupt on PIR_UP_PIN pin gone high.
void isrPirUp() 
{
  // Motion detected.
  bitSet(flags, F_UP);
  bitSet(flags, F_UP_STAT);
  
  // Cannot detect both at once, so see if the other fired too.
  if (!bitRead(flags, F_DOWN_STAT)) {
    if (digitalRead(PIR_DOWN_PIN) == HIGH) {
      isrPirDown();
    }
  }
  
}

// Interupt on PIR_DOWN_PIN pin gone high.
void isrPirDown() 
{
  // Motion detected.
  bitSet(flags, F_DOWN);
  bitSet(flags, F_DOWN_STAT);
  
  // Cannot detect both at once, so see if the other fired too.
  if (!bitRead(flags, F_UP_STAT)) {
    if (digitalRead(PIR_UP_PIN) == HIGH) {
      isrPirUp();
    }
  }
}

void setup()
{
    pinMode(DEBUG_LED_TX, OUTPUT);
    pinMode(DEBUG_LED_PULSE, OUTPUT);
    pinMode(DEBUG_LED_MOTION, OUTPUT);
    
    
    pinMode(RF_POWER_PIN, OUTPUT);
    pinMode(PIR_UP_PIN, INPUT);
    pinMode(PIR_DOWN_PIN, INPUT);
    
    bitClear(flags, F_UP);
    bitClear(flags, F_DOWN);
    bitClear(flags, F_UP_STAT);
    bitClear(flags, F_DOWN_STAT);
     
  
    //Serial.begin(9600);
    //Serial.println("setup");
    
    vw_set_tx_pin(RF_TX_PIN);
    vw_setup(2000);      // Bits per sec
    
    
    attachInterrupt(0, isrPirUp, RISING);
    attachInterrupt(1, isrPirDown, RISING);

}

void loop()
{
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    // TODO: reset previousMillis when overruns max value.
    
    pulse = !pulse;
    digitalWrite(DEBUG_LED_PULSE, pulse);
    

    // Can't set use millis in the interupt.
    if (bitRead(flags, F_UP_STAT)) {
      lastPirUp = currentMillis;
      //Serial.println("UP on");
      bitClear(flags, F_UP_STAT);
      digitalWrite(DEBUG_LED_MOTION, HIGH);
    }
    
    if (bitRead(flags, F_DOWN_STAT)) {
      lastPirDown = currentMillis;
      //Serial.println("DOWN on");
      bitClear(flags, F_DOWN_STAT);
      digitalWrite(DEBUG_LED_MOTION, HIGH);
    }

    
    if (bitRead(flags, F_UP) && currentMillis - lastPirUp > pirReset) {
      bitClear(flags, F_UP);
      //Serial.println("UP off");
      digitalWrite(DEBUG_LED_MOTION, LOW);
    }
      
    if (bitRead(flags, F_DOWN) && currentMillis - lastPirDown > pirReset) {
      bitClear(flags, F_DOWN);
      //Serial.println("DOWN off"); 
      digitalWrite(DEBUG_LED_MOTION, LOW);
    }
      
    if (bitRead(flags, F_UP) || bitRead(flags, F_DOWN) || pulse) { 
      char buf[50];
      sprintf(buf, "count=%lu,flags=%u", count, flags); 
   
      if (bitRead(flags, F_UP)) { 
        strcat (buf, " UP ");
      }
      
      if (bitRead(flags, F_DOWN)) { 
        strcat (buf, " DOWN ");
      }
        
      //Serial.println(buf);
      
      digitalWrite(RF_POWER_PIN, HIGH); // power up the transmitter
      digitalWrite(DEBUG_LED_TX, true); // Flash a light to show transmitting
      vw_send((uint8_t *)buf, strlen(buf));
      vw_wait_tx(); // Wait until the whole message is gone
      digitalWrite(DEBUG_LED_TX, false);
      digitalWrite(RF_POWER_PIN, LOW); // power down the transmitter
      count++;
    }
    
  }
  
  

}
