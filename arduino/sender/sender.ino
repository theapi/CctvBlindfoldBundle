/*
 One PIR sensor detects motion at the bottom of the stairs: DOWN.
 Another detects motion at the top of the stairs: UP.
 
 Sends an RF message when one both sensors have been triggered,
 indicating the direction of travel.
 The message is sent multiple times to give it the best chance of getting through.
*/

#include <VirtualWire.h>
// http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
#include <avr/power.h>
#include <avr/sleep.h>

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

volatile boolean pulse = false;


// Store the pir states on interupt.
volatile byte flags = 0;

volatile byte motionUp = 0;
volatile byte motionDown = 0;

volatile byte motion = 0;

// Motion on either interupt
void isrMotion() {
  motion = 1;
}

// Interupt on PIR_UP_PIN
void isrPirUp() 
{
  // Motion detected.
  bitSet(flags, F_UP);
  bitSet(flags, F_UP_STAT);
  motionUp = 1;
  /*
  // Cannot detect both at once, so see if the other fired too.
  if (!bitRead(flags, F_DOWN_STAT)) {
    if (digitalRead(PIR_DOWN_PIN) == HIGH) {
      isrPirDown();
    }
  }
  */
  
}

// Interupt on PIR_DOWN_PIN
void isrPirDown() 
{
  // Motion detected.
  bitSet(flags, F_DOWN);
  bitSet(flags, F_DOWN_STAT);
  motionDown = 1;
  
  /*
  // Cannot detect both at once, so see if the other fired too.
  if (!bitRead(flags, F_UP_STAT)) {
    if (digitalRead(PIR_UP_PIN) == HIGH) {
      isrPirUp();
    }
  }
  */
}

/**
 * @see http://forum.arduino.cc/index.php/topic,85627.0.html
 */
void sleepNow()
{
  /* Now is the time to set the sleep mode. In the Atmega8 datasheet
   * http://www.atmel.com/dyn/resources/prod_documents/doc2486.pdf on page 35
   * there is a list of sleep modes which explains which clocks and 
   * wake up sources are available in which sleep modus.
   *
   * In the avr/sleep.h file, the call names of these sleep modus are to be found:
   *
   * The 5 different modes are:
   *     SLEEP_MODE_IDLE         -the least power savings 
   *     SLEEP_MODE_ADC
   *     SLEEP_MODE_PWR_SAVE
   *     SLEEP_MODE_STANDBY
   *     SLEEP_MODE_PWR_DOWN     -the most power savings
   *
   *  the power reduction management <avr/power.h>  is described in 
   *  http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
   */

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here

  sleep_enable();          // enables the sleep bit in the mcucr register
  // so sleep is possible. just a safety pin 

  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();


  sleep_mode();            // here the device is actually put to sleep!!

  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep:
  // disable sleep...

  power_all_enable();

}

/**
 * The SecretVoltmeter 
 * @see https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
 */
long readVcc() {
  long result;
  
  // Power up the adc so the volts can be read.
  //power_adc_enable();
  
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  
  // Turn off the adc again to save power.
  //power_adc_disable();
  
  return result;
}

void transmit(char *buf) {
  digitalWrite(RF_POWER_PIN, HIGH); // power up the transmitter
  digitalWrite(DEBUG_LED_TX, true); // Flash a light to show transmitting
  vw_send((uint8_t *)buf, strlen(buf));
  vw_wait_tx(); // Wait until the whole message is gone
  digitalWrite(DEBUG_LED_TX, false);
  digitalWrite(RF_POWER_PIN, LOW); // power down the transmitter 
}

void setup()
{
    // @see http://forum.arduino.cc/index.php/topic,85627.0.html
    //power_adc_disable();
    
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
    
    // Been having issues with two interupt pins seemingly setting each other off.
    // so both call the same isr and only one motion is used.
    attachInterrupt(0, isrMotion, RISING);
    attachInterrupt(1, isrMotion, RISING);

}

void loop() 
{
  sleepNow();
  
  if (motion) {
    
    if (digitalRead(PIR_DOWN_PIN) == HIGH) {
      //digitalWrite(DEBUG_LED_MOTION, HIGH);
      
      char buf[50];
      sprintf(buf, "c=%lu,mv=%u,F_DOWN_STAT", count, readVcc());
      transmit(buf);
      
      count++;
    } else if (digitalRead(PIR_UP_PIN) == HIGH) {
      //digitalWrite(DEBUG_LED_MOTION, HIGH);
    
      char buf[50];
      sprintf(buf, "c=%lu,mv=%u,F_UP_STAT", count, readVcc());
      transmit(buf);
      
      count++;
    }  
    
    // Motion delt with, reset to zero
    motion = 0;
  }
  
  /*
  if (motionUp) {
    motionUp = 0;
    char buf[50];
    sprintf(buf, "c=%lu,F_UP_STAT", count);
    transmit(buf);
    
    count++;
  }
  
  if (motionDown) {
    motionDown = 0;
    char buf[50];
    sprintf(buf, "c=%lu,F_DOWN_STAT", count);
    transmit(buf);
    
    count++;
  }
  */
  
  /*
  // Movement detected
  if (bitRead(flags, F_UP_STAT)) {
    bitClear(flags, F_UP_STAT);
    bitClear(flags, F_UP);
    
    if (digitalRead(PIR_UP_PIN) == HIGH) {
      //digitalWrite(DEBUG_LED_MOTION, HIGH);
    
      char buf[50];
      sprintf(buf, "c=%lu,F_UP_STAT", count);
      transmit(buf);
      
      count++;
    }
    

    
  } 
  
  if (bitRead(flags, F_DOWN_STAT)) {
    bitClear(flags, F_DOWN_STAT);
    bitClear(flags, F_DOWN);
    
    if (digitalRead(PIR_DOWN_PIN) == HIGH) {
      //digitalWrite(DEBUG_LED_MOTION, HIGH);
      
      char buf[50];
      sprintf(buf, "c=%lu,F_DOWN_STAT", count);
      transmit(buf);
      
      count++;
    }
    
    
  }
  */

}

void XXloop()
{
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    // TODO: reset previousMillis when overruns max value.
    
    pulse = !pulse;
    digitalWrite(DEBUG_LED_PULSE, pulse);
    

    // Initial movement detected
    if (bitRead(flags, F_UP_STAT)) {
      lastPirUp = currentMillis;
      //Serial.println("UP on");
      bitClear(flags, F_UP_STAT);
      digitalWrite(DEBUG_LED_MOTION, HIGH);
    }
    
    // Initial movement detected
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
    //if (pulse) {
      char buf[50];
      sprintf(buf, "count=%lu,mv=%u,currentMillis=%lu", count, readVcc(), currentMillis); 
      //sprintf(buf, "count=%lu,flags=%u", count, flags); 

      if (bitRead(flags, F_UP)) { 
        strcat(buf, " UP ");
      }
      
      if (bitRead(flags, F_DOWN)) { 
        strcat(buf, " DOWN ");
      }
        
      
        
      //Serial.println(buf);
      
      /*
      digitalWrite(RF_POWER_PIN, HIGH); // power up the transmitter
      digitalWrite(DEBUG_LED_TX, true); // Flash a light to show transmitting
      vw_send((uint8_t *)buf, strlen(buf));
      vw_wait_tx(); // Wait until the whole message is gone
      digitalWrite(DEBUG_LED_TX, false);
      digitalWrite(RF_POWER_PIN, LOW); // power down the transmitter
      */
      transmit(buf);
      
      count++;
    }
    
  }
  
  

}
