#!/usr/bin/env python2.7
# @see http://RasPi.tv

import signal, sys
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)

# GPIO 23 set up as inputs. Pulled down.
# 23 will go to 3V3 (3.3V)
GPIO.setup(23, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)


def signal_handler(signal, frame):
    print 'You pressed Ctrl+C!'
    GPIO.cleanup()  # clean up GPIO on exit
    sys.exit(0)

# now we'll define the threaded callback function
# this will run in another thread when our event is detected
def my_callback(channel):
    print "Rising edge detected on port 23 - even though, in the main thread,"
    print "- how cool?\n"

print "You will also need a second button connected so that when pressed"
print "it will connect GPIO port 23 (pin 18) to 3V3 (pin 1)"


# The GPIO.add_event_detect() line below set things up so that
# when a rising edge is detected on port 23, regardless of whatever
# else is happening in the program, the function "my_callback" will be run
# It will happen even while the program is in the loop.
GPIO.add_event_detect(23, GPIO.RISING, callback=my_callback, bouncetime=200)


signal.signal(signal.SIGINT, signal_handler)
print 'Press Ctrl+C to exit'

while 1:
    # listen to sockets, adc etc
    time.sleep(10) # just wait a while for now.



