/*

 Sleep until motion is detectected,
 then turn on the two ping sensors which detect if someone goes past and in which direction.

*/

// http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
#include <avr/power.h>
#include <avr/sleep.h>

// NewPing uses timer 2 and switches pinging between sensors
// @see https://code.google.com/p/arduino-new-ping/
#include <NewPing.h>

// VirtualWire handles the RF transmission
// @see http://www.airspayce.com/mikem/arduino/
#include <VirtualWire.h>

#define SONAR_NUM     2 // Number of sensors.
#define MAX_DISTANCE 700 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 33 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

// Pins
const byte PIN_PIR = 2;         // PIR on external interupt to wake up the cpu, int.0 interupt 0 on pin 2

const byte PIN_RF_TX = 9;       // Pin 12 is the default sender pin but I want 9.
const byte PIN_RF_POWER = 10;   // Provide power to the transmitter
const byte PIN_PING_POWER = 14; // Controls the transistor switch for both ping sensors.

const byte PIN_TRIG_LEFT = 7;   // Send the ping on the left sensor.
const byte PIN_ECHO_LEFT = 8;   // Listen for the left ping's echo.

const byte PIN_TRIG_RIGHT = 5;  // Send the ping on the right sensor.
const byte PIN_ECHO_RIGHT = 6;  // Listen for the right ping's echo.

// Debug leds
const byte PIN_DEBUG_MOTION = 13; // The PIR detected motion.
const byte PIN_DEBUG_RF_TX = 11; 
 
unsigned long msgId = 0;        // Each transmission has an id so the receiver knows if it missed something.
unsigned long lastTransmit = 0; // The last time a transmission was sent.

long powerTimeout = 60000;      // Time to wait in milliseconds to power down if no detection.
 
unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[SONAR_NUM] = {     // Sensor object array.
  NewPing(PIN_TRIG_LEFT, PIN_ECHO_LEFT, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(PIN_TRIG_RIGHT, PIN_ECHO_RIGHT, MAX_DISTANCE),
};

volatile byte pingPowerState = 0;     // 0 = ping off, 1 = ping on.
volatile unsigned long awakeTime = 0; // How long the cpu has been awake since last wake up.


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
  
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  
  return result;
}

/**
 * Transmit the message.
 */
void transmit(char *buf) 
{
  vw_send((uint8_t *)buf, strlen(buf));
  // NB: ping returns 0 if transmitting while vw_wait_tx() so don't use it.
  //vw_wait_tx(); // Wait until the whole message is gone
  lastTransmit = millis();
}

/**
 * ISR from timer 2 - ISR(TIMER2_COMPA_vect)
 */
void echoCheck() 
{
  // If ping received, set the sensor distance to array.
  if (sonar[currentSensor].check_timer()) { 
    cm[currentSensor] = sonar[currentSensor].ping_result / US_ROUNDTRIP_CM;
  }
}

/**
 * Sensor ping cycle complete, do something with the results.
 */
void oneSensorCycle() 
{
  // The following code would be replaced with your code that does something with the ping results.
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    Serial.print(i);
    Serial.print("=");
    Serial.print(cm[i]);
    Serial.print("cm ");
  }
  Serial.println();
  
}

/*
 * ISR for pin 2 (int.0)
 */
void isrMotion() {
  // Tell the loop to power up the ping sensors.
  pingPowerState = 1;
  // Rest the awake time.
  awakeTime = 0;
}

void setup() 
{
  Serial.begin(115200);
  
  pinMode(PIN_PING_POWER, OUTPUT);
  
  vw_set_tx_pin(PIN_RF_TX);
  vw_setup(2000);      // Bits per sec
  
  pingTimer[0] = millis() + 75;           // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  for (uint8_t i = 1; i < SONAR_NUM; i++) // Set the starting time for each sensor.
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
    
}

void loop() 
{
  // PING )))
  for (uint8_t i = 0; i < SONAR_NUM; i++) { // Loop through all the sensors.
    if (millis() >= pingTimer[i]) {         // Is it this sensor's time to ping?
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;  // Set next time this sensor will be pinged.
      if (i == 0 && currentSensor == SONAR_NUM - 1) oneSensorCycle(); // Sensor ping cycle complete, do something with the results.
      sonar[currentSensor].timer_stop();          // Make sure previous timer is canceled before starting a new ping (insurance).
      currentSensor = i;                          // Sensor being accessed.
      cm[currentSensor] = 0;                      // Make distance zero in case there's no ping echo for this sensor.
      sonar[currentSensor].ping_timer(echoCheck); // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
    }
  }
  
  // Other code that *DOESN'T* analyze ping results can go here.
  
  if (millis() - lastTransmit > 1000) {
    // Proof of concept to power cycle the pings
    pingPowerState =!pingPowerState;
    digitalWrite(PIN_PING_POWER, pingPowerState);
    
    msgId++;
    char buf[50];
    sprintf(buf, "id=%lu", msgId);
    
    transmit(buf); 
  }
  
}

