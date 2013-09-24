#!/usr/bin/python
# Monitor the various inputs (ADC, switch) and report on a socket.
# Thanks to http://www.binarytides.com/code-chat-application-server-client-sockets-python/

import socket, select, atexit, signal, sys
import RPi.GPIO as io
from time import sleep
from Adafruit_ADS1x15 import ADS1x15


def signal_handler(signal, frame):
    print 'You pressed Ctrl+C!'
    sys.exit(0)

def broadcast_data (message):
    #Do not send the message to master socket and the client who has send us the message
    for socket in CONNECTION_LIST:
        if socket != server_socket :
            try :
                socket.send(message)
            except :
                # broken socket connection may be, chat client pressed ctrl+c for example
                socket.close()
                CONNECTION_LIST.remove(socket)

if __name__ == "__main__":

    io.setmode(io.BCM)
    
    switch_pin = 23
    io.setup(switch_pin, io.IN, pull_up_down=io.PUD_DOWN)  # activate input with Pull Down - Ov
	
    signal.signal(signal.SIGINT, signal_handler)
    #print 'Press Ctrl+C to exit'
    
    ADS1015 = 0x00  # 12-bit ADC
    ADS1115 = 0x01  # 16-bit ADC
     
    # Initialise the ADC using the default mode (use default I2C address)
    # Set this to ADS1015 or ADS1115 depending on the ADC you are using!
    adc = ADS1x15(ic=ADS1015)
     
    # List to keep track of socket descriptors
    CONNECTION_LIST = []
    RECV_BUFFER = 4096 # Advisable to keep it as an exponent of 2
    PORT = 8889
     
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("0.0.0.0", PORT))
    server_socket.listen(1)
 
    # Add server socket to the list of readable connections
    CONNECTION_LIST.append(server_socket)


    while 1:
      
        # Check the toggle switch for a press
        if io.input(door_pin):
            print("TOGGLE")
            broadcast_data('TOGGLE')
      
        # Read channel 0 in single-ended mode, +/-4.096V, 250sps
        #reading = adc.readADCSingleEnded(3, 1024, 860)

        #if reading > 0.5:
        #    print reading
        #    broadcast_data(reading)

        #time.sleep(0.01)
		
        # Get the list sockets which are ready to be read through select
        read_sockets,write_sockets,error_sockets = select.select(CONNECTION_LIST,[],[],0.01)
			
      	for sock in read_sockets:
            #New connection
            if sock == server_socket:
                # Handle the case in which there is a new connection recieved through server_socket
                sockfd, addr = server_socket.accept()
                CONNECTION_LIST.append(sockfd)
                print "Client (%s, %s) connected" % addr
        		
            #Some incoming message from a client
            else:
                # Data recieved from client, process it
                try:
                    # In Windows, sometimes when a TCP program closes abruptly,
                    # a "Connection reset by peer" exception will be thrown
                    data = sock.recv(RECV_BUFFER)
                    # ignore it, this is broadcast only           
        			 
                except:
                    print "Client (%s, %s) is offline" % addr
                    sock.close()
                    CONNECTION_LIST.remove(sock)
                    continue
			
	
			
