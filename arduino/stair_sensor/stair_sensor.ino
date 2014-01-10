/*
 Sleep until motion is detectected,
 then turn on the two ping sensors which detect if someone goes past and in which direction.
 Transmit the detected direction.
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
#define MAX_DISTANCE 50 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 33 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

// Flags definitions
#define F_MOTION 1 // 1 = activated  0 = no movement
#define F_LEFT   2 // 1 = detection  0 = no detection
#define F_RIGHT  3 // 1 = detection  0 = no detection
#define F_ACTIVE_LEFT   4 // 1 = detection  0 = no detection
#define F_ACTIVE_RIGHT  5 // 1 = detection  0 = no detection

#define MSG_BUFFER_LEN   30    // How long the transmitted message can be.
#define MSG_QUEUE_LENGTH 3     // How many transmission message to store.
#define MSG_TRANSMIT_NUM 5     // How many times to transmit each message.
#define MSG_TRANSMIT_DELAY 100 // How long to wait before sending the next message.

const byte DISTANCE_THRESHOLD = 40; // Less than this (in cm) activates the detection sequence.

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
 
const byte RF_ID = 1;           // The unique id of this device, so the receive knows where it came from.

unsigned long msgId = 0;        // Each transmission has an id so the receiver knows if it missed something.
unsigned long lastTransmit = 0; // The last time a transmission was sent.
char* msgQueue[MSG_QUEUE_LENGTH];
unsigned long msgTime[MSG_QUEUE_LENGTH];
byte msgTransmitted[MSG_QUEUE_LENGTH]; // Number of times the message has been transmitted.

long powerTimeout = 60000;      // Time to wait in milliseconds to power down if no detection.
 
unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonar[SONAR_NUM] = {     // Sensor object array.
  NewPing(PIN_TRIG_LEFT, PIN_ECHO_LEFT, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(PIN_TRIG_RIGHT, PIN_ECHO_RIGHT, MAX_DISTANCE),
};

byte flags = 0; // Booleans



volatile byte powerState = 0;     // 0 = ping off, 1 = ping on.
volatile unsigned long awakeTime = 0; // When the cpu woke up.


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
 * Create a transmition message
 */
void createTransmitionMsg(char *movementDirection)
{
  msgId++;
  char buf[MSG_BUFFER_LEN];
  // Device rf id, message id, direction char, volts
  sprintf(buf, "%u,%lu,%c,%lu", RF_ID, msgId, movementDirection, readVcc());
  // TODO queue message for transmition multiple times.
  //transmit(buf); // just send it for now.
  msgEnqueue(buf);
}

void msgEnqueue(char *buf) 
{
  unsigned long now = millis();
  byte queued = 0;
  for (uint8_t i = 0; i < MSG_QUEUE_LENGTH; i++) {
    if (msgQueue[i][0] == 0) { // First character is zero.
      msgQueue[i] = buf;
      msgTime[i] = now;
      queued = 1; // Managed to add to the queue.
    }
  }
  
  if (queued == 0) {
    // Didn't get into the queue.
    byte oldestIndex = getOldestMsqQueueIndex(); 

    // Replace the oldest one
    msgQueue[oldestIndex] = buf;
    msgTime[oldestIndex] = now;
  }
}

byte getOldestMsqQueueIndex()
{
  unsigned long now = millis();
  long longestDiff = 0;
  byte oldestIndex = 0;
  // Find the oldest message.
  for (uint8_t i = 0; i < MSG_QUEUE_LENGTH; i++) {
    long diff = now - msgTime[i];
    if (diff > longestDiff) {
      longestDiff = diff;
      oldestIndex = i;
    }
  }
  
  return oldestIndex;
}

/**
 * Send the correct message for the correct number of times.
 */
void processMsgQueue()
{
  if (millis() - lastTransmit > MSG_TRANSMIT_DELAY) {
    // Time to send another transmition if one is waiting.
    byte oldestIndex = getOldestMsqQueueIndex(); 
    // Check for a message.
    if (msgQueue[oldestIndex][0] == 0) { // First character is zero.
      // nothing to do.
      return;
    }

    transmit(msgQueue[oldestIndex]);
    msgTransmitted[oldestIndex]++;
    
    // If the message has been sent enough times, delete it.
    if (msgTransmitted[oldestIndex] > MSG_TRANSMIT_NUM) {
      char buf[MSG_BUFFER_LEN] = {0};
      msgQueue[oldestIndex] = buf;
      msgTime[oldestIndex] = 0;
      msgTransmitted[oldestIndex] = 0;
    }
  }
}

/**
 * Transmit a message.
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
  
  // Check for activation.
  if (cm[0] < DISTANCE_THRESHOLD) {
    // Flag that it happend.
    bitSet(flags, F_LEFT);
  } else {
    bitClear(flags, F_LEFT);
  }
  if (cm[1] < DISTANCE_THRESHOLD) {
    // Flag that it happend.
    bitSet(flags, F_RIGHT);
  } else {
    bitClear(flags, F_RIGHT); 
  }
  
  handlePingFlags();
}

void handlePingFlags()
{
  if (!bitRead(flags, F_LEFT) && !bitRead(flags, F_RIGHT)) {
    
    // Waiting for activation.
    return;
    
  } else if (bitRead(flags, F_LEFT) && bitRead(flags, F_RIGHT)) {
    
    // Both beams are broken
    return;
    
  } else if (bitRead(flags, F_LEFT)) {
    
    // Left beam is broken only
    if (bitRead(flags, F_ACTIVE_RIGHT)) {
      // Right beam was activate first.
      bitClear(flags, F_ACTIVE_RIGHT);
      
      // Send left < right message
      createTransmitionMsg("<");
      
    } else {
      // Start of activation.
      bitSet(flags, F_ACTIVE_LEFT);
    }
  
  } else if (bitRead(flags, F_RIGHT)) {
    
    // Right beam is broken only
    if (bitRead(flags, F_ACTIVE_LEFT)) {
      // Left beam was activate first.
      bitClear(flags, F_ACTIVE_LEFT);
      
      // Send left > right message
      createTransmitionMsg(">");
      
    } else {
      // Start of activation.
      bitSet(flags, F_ACTIVE_RIGHT);
    }
    
  }

}

/*
 * ISR for pin 2 (int.0)
 */
void isrMotion() {
  bitSet(flags, F_MOTION);
}

void setup() 
{
  Serial.begin(115200);
  
  // Zero all the flags.
  flags = 0;
  
  pinMode(PIN_PIR, INPUT);
  
  pinMode(PIN_RF_POWER, OUTPUT);
  pinMode(PIN_PING_POWER, OUTPUT);
  
  pinMode(PIN_DEBUG_MOTION, OUTPUT);
  
  // Init the message queue.
  for (uint8_t i = 0; i < MSG_QUEUE_LENGTH; i++) {
      char buf[MSG_BUFFER_LEN] = {0};
      msgQueue[i] = buf;
      msgTime[i] = 0;
      msgTransmitted[i] = 0;
  }
  
  vw_set_tx_pin(PIN_RF_TX);
  vw_setup(2000);      // Bits per sec
  
  pingTimer[0] = millis() + 75;           // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  for (uint8_t i = 1; i < SONAR_NUM; i++) // Set the starting time for each sensor.
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
    
  // Attach the external interupt to the PIR.
  attachInterrupt(0, isrMotion, RISING);
    
}

void loop() 
{
  
  // The motion ISR sets a flag so we know it detected motion.
  if (bitRead(flags, F_MOTION)) {
    bitClear(flags, F_MOTION);
    if (powerState == 0) {
      // Remember when the cpu woke up.
      awakeTime = millis();
    }
    
    // Ensure the ping sensors are powered.
    powerState = 1;
    // Turn on the ping power.
    digitalWrite(PIN_PING_POWER, HIGH);
    // Turn on the rf power.
    digitalWrite(PIN_RF_POWER, HIGH);
    
    // Turn on the debug light.
    digitalWrite(PIN_DEBUG_MOTION, HIGH);   
  }
  
  if (powerState == 0) {
    // Turn off the power to the ping sensors.
    digitalWrite(PIN_PING_POWER, LOW);
    // Turn off the rf power.
    digitalWrite(PIN_RF_POWER, LOW);
    
    // Turn off the debug light.
    digitalWrite(PIN_DEBUG_MOTION, LOW);

    // Zero all the flags.
    flags = 0;
  
    // Sleep until the next motion.
    sleepNow();
  } 
  
  
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
  
  long nowMillis = millis();
  
  if (nowMillis - lastTransmit > 1000) {
    msgId++;
    char buf[50];
    sprintf(buf, "id=%lu", msgId);
    transmit(buf); 
  }
  
  processMsgQueue();
  
  if (nowMillis - awakeTime > powerTimeout) {
    //TODO: and not waiting for second sensor
    
    // Powerdown at the start of the next loop
    powerState = 0;
  }
  
}

