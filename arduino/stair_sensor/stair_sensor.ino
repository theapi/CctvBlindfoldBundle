/*

 Sleep until motion is detectected,
 then turn on the two ping sensors which detect if someone goes past and in which direction.

*/

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
const byte PIN_RF_TX = 9;       // Pin 12 is the default sender pin but I want 9.
const byte PIN_PING_POWER = 14; // Controls the transistor switch for both ping sensors.

const byte PIN_TRIG_LEFT = 7;   // Send the ping on the left sensor.
const byte PIN_ECHO_LEFT = 8;   // Listen for the left ping's echo.

const byte PIN_TRIG_RIGHT = 5;  // Send the ping on the right sensor.
const byte PIN_ECHO_RIGHT = 6;  // Listen for the right ping's echo.

byte pingPowerState = 1;        // 0 = sensor off, 1 = sensor on.
 
unsigned long msgId = 0;        // Each transmission has an id so the receiver knows if it missed something.
unsigned long lastTransmit = 0; // The last time a transmission was sent.
 
unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[SONAR_NUM] = {     // Sensor object array.
  NewPing(PIN_TRIG_LEFT, PIN_ECHO_LEFT, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(PIN_TRIG_RIGHT, PIN_ECHO_RIGHT, MAX_DISTANCE),
};


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

