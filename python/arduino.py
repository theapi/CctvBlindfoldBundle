import serial

s = serial.Serial(port='/dev/ttyUSB0', baudrate=9600)
print s
while True:
    line = s.readline()
    line = line.strip()
    print line
