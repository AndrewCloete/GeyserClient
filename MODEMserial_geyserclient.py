import serial
import time
import json
import sys
import RPi.GPIO as GPIO


if(len(sys.argv) != 2):
        print('Usage: pyhton pythonUDPclient <geyser_id>')
        sys.exit(0)

def readlineCR(port):
    rv = ""
    while True:
        ch = port.read()
        rv += ch
        if ch=='\r' or ch=='':
            return rv


GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(27, GPIO.OUT)


GEYSER_ID = long(sys.argv[1])

port = serial.Serial("/dev/ttyAMA0", baudrate=115200, timeout=3.0)

data = {"id":GEYSER_ID, "t1":55, "t2":35, "e":"false"}
while True:
        data_str = json.dumps(data)
        port.flushOutput()
        port.flushInput()
        port.write(data_str)
        recv_data = readlineCR(port)
        print("Settings from NSCL: " + recv_data)


        led_state = 'false' #Default (Safe) state
        try:
                json_recv_data = json.loads(recv_data)
                led_state = json_recv_data["e"]
                print("SUCCESS - LED state: " + led_state)
        except ValueError:
                print("ERROR - JSON decode error. Recv data from server corrupt")

        if led_state == 'true':
                GPIO.output(27, True)
        elif led_state == 'false':
                GPIO.output(27, False)
        else:
                GPIO.output(27, False)

        data = {"id":GEYSER_ID, "t1":55, "t2":35, "e":led_state}
        time.sleep(5)

