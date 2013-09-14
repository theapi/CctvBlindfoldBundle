#!/usr/bin/env python

# http://www.youtube.com/watch?v=Dc16mKFA7Fo

import time, sys, exceptions
import RPi.GPIO as GPIO


GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)

ControlPin = [18,22,24,26]

for pin in ControlPin:
  GPIO.setup(pin,GPIO.OUT)
  GPIO.output(pin, 0)


seq = [ [1,0,0,0],
        [1,1,0,0],
        [0,1,0,0],
        [0,1,1,0],
        [0,0,1,0],
        [0,0,1,1],
        [0,0,0,1],
        [1,0,0,1] ]

# 512 steps is a full rotation

def anticlockwise(steps):
    for i in range(steps):
        for halfstep in reversed(range(8)):
            for pin in range(4):
                GPIO.output(ControlPin[pin], seq[halfstep][pin])
            time.sleep(0.001)


def clockwise(steps):
    for i in range(steps):
        for halfstep in range(8):
            for pin in range(4):
                GPIO.output(ControlPin[pin], seq[halfstep][pin])
            time.sleep(0.001)



if len (sys.argv) != 2 :
    print "Usage: sudo python blindfold.py steps"
    sys.exit (1)


try:
    steps = int(sys.argv[1])
except exceptions.ValueError:
    print "'steps' must be an integer, 512 is a full rotation"
    sys.exit (1)



if steps > 0:
    clockwise(steps)
elif steps < 0:
    anticlockwise(steps * -1)
else:
    print "Huh?"


GPIO.cleanup()
