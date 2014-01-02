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
#define DEBUG_LED_TX 11 // Debug led for tx
#define DEBUG_LED_PULSE 12 // Debug led for alive pulse (tmp)
#define DEBUG_LED_MOTION 13

#define PIR_UP_PIN 2        // int.0 interupt 0 on pin 2
#define PIR_DOWN_PIN 3      // int.1 interupt 1 on pin 3

// Bit flags for pir state
#define F_UP 1          // 1 = activated  0 = no movement
#define F_DOWN 2        // 1 = activated  0 = no movement
#define F_UP_STAT 3     // 1 = new  0 = old
#define F_DOWN_STAT 4   // 1 = new  0 = old

unsigned long transmitId = 0;

long motionTime = 0;
long timeout = 60000; 


// Store the pir states on interupt.
volatile byte flags = 0;
volatile byte motion = 0;

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
  digitalWrite(DEBUG_LED_TX, HIGH); // Flash a light to show transmitting
  
  // Send multiple times in the hope it gets through.
  for (byte i=0; i<4; i++) {
    vw_send((uint8_t *)buf, strlen(buf));
    vw_wait_tx(); // Wait until the whole message is gone
    delay(500);
  }
  
  digitalWrite(DEBUG_LED_TX, LOW);
  digitalWrite(RF_POWER_PIN, LOW); // power down the transmitter 
}

// Motion on either interupt
void isrMotion() {
  motion = 1;
}

void setup()
{
   
    pinMode(DEBUG_LED_TX, OUTPUT);
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
  if (!bitRead(flags, F_DOWN) && !bitRead(flags, F_UP)) {
    digitalWrite(DEBUG_LED_MOTION, LOW);
    sleepNow();
  } else {
    digitalWrite(DEBUG_LED_MOTION, HIGH);
  }
  
  // Wait a while waiting for the next pir to detect 
  unsigned long currentMillis = millis();
  if (currentMillis - motionTime > timeout) {
     bitClear(flags, F_UP);
     bitClear(flags, F_DOWN);
  }
  
  
  if (motion) {
    motionTime = currentMillis;
    
    
    if (digitalRead(PIR_DOWN_PIN) == HIGH) {
      //
      // Has the other pir recently detected motion
      if (bitRead(flags, F_UP)) {
        // Report that someone just went down stairs.
        transmitId++;
        char buf[50];
        sprintf(buf, "id=%lu,mv=%u,d=DOWN", transmitId, readVcc());
        transmit(buf);
        bitClear(flags, F_UP);
        bitClear(flags, F_DOWN);
      } else {
        // Remember that it happened
        bitSet(flags, F_DOWN);
      }
      //digitalWrite(DEBUG_LED_MOTION, LOW);
      
    } else if (digitalRead(PIR_UP_PIN) == HIGH) {
      //digitalWrite(DEBUG_LED_MOTION, HIGH);
      // Has the other pir recently detected motion
      if (bitRead(flags, F_DOWN)) {
        // Report that someone just went up stairs.
        transmitId++;
        char buf[50];
        sprintf(buf, "id=%lu,mv=%u,d=UP", transmitId, readVcc());
        transmit(buf);
        bitClear(flags, F_DOWN);
        bitClear(flags, F_UP);
      } else {
        // Remember that it happened
        bitSet(flags, F_UP);
      }
      //digitalWrite(DEBUG_LED_MOTION, LOW);
     
    }  

    // Motion delt with, reset to zero
    motion = 0;
  }
 
}

