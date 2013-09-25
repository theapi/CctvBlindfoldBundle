#!/usr/bin/env python2.7
# @see http://raspi.tv/2013/how-to-use-interrupts-with-python-on-the-raspberry-pi-and-rpi-gpio

import signal, sys, time
import RPi.GPIO as GPIO


GPIO.setmode(GPIO.BCM)

# GPIO 23 set up as inputs.
GPIO.setup(23, GPIO.IN, pull_up_down=GPIO.PUD_UP)


def signal_handler(signal, frame):
    print 'You pressed Ctrl+C!'
    GPIO.cleanup()  # clean up GPIO on exit
    sys.exit(0)

# now we'll define the threaded callback function
# this will run in another thread when our event is detected
def my_callback(channel):
    print(time.ctime())

print "started..."


# The GPIO.add_event_detect() line below set things up so that
# It will happen even while the program is in the loop.
GPIO.add_event_detect(23, GPIO.FALLING, callback=my_callback, bouncetime=200)


signal.signal(signal.SIGINT, signal_handler)
print 'Press Ctrl+C to exit'

while 1:
    # listen to sockets, adc etc
    time.sleep(10) # just wait a while for now.


