import serial
import time
import json
import sys


# --------------------- Sanity checking arguments ------------------------
if(len(sys.argv) != 2):
        print('Usage: pyhton pythonUDPclient <geyser_ID>')
        sys.exit(0)

try:
	GEYSER_ID = long(sys.argv[1])
except ValueError:
	print('ID not valid: (Value between 1230 and 1240)')
	sys.exit(0)
# -----------------------------------------------------------------------

def readlineCR(port):
    rv = ""
    while True:
        ch = port.read()
        rv += ch
        if ch=='\r' or ch=='':
            return rv

port = serial.Serial("/dev/ttyAMA0", baudrate=115200, timeout=3.0)

data = {
		"Ver": + 0001,
		"ID": GEYSER_ID ,
		"Tstamp": int(time.time()),
		"Rstate": "OFF",
		"Vstate": "OPEN",
		"Gstate": "OK",
		"T1": 55,
		"T2": 60,
		"T3": 30,
		"T4": 25,
		"KW": 0.0,
		"KWH": 0.0,
		"HLmin": 0.0,
		"HLtotal": 0.0,
		"CLmin": 0.0,
		"CLtotal": 0.0
		}
while True:
        data['Tstamp'] = int(time.time());
        data_str = json.dumps(data)
        port.flushOutput()
        port.flushInput()
        port.write(data_str)
        recv_data = readlineCR(port)
        print("Settings from NSCL: " + recv_data)

        try:
                json_recv_data = json.loads(recv_data)
                if json_recv_data["status"] == "ACK":
			print("SUCCESS")
        except ValueError:
                print("ERROR - JSON decode error. Recv data from server corrupt")
        time.sleep(5)
