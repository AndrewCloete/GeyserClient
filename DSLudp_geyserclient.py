import socket
import time
import json
import sys

import RPi.GPIO as GPIO

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(27, GPIO.OUT)



if(len(sys.argv) != 4):
        print('Usage: pyhton pythonUDPclient <udp_IP_address> <udp_port> <geyser_id>')
        sys.exit(0)


UDP_IP = sys.argv[1]
UDP_PORT = int(sys.argv[2])
GEYSER_ID = long(sys.argv[3])

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2)

data = {"id":GEYSER_ID, "t1":55, "t2":35, "e":"false"}
while True:
        data_str = json.dumps(data)
        sock.sendto(data_str, (UDP_IP, UDP_PORT))
        try:
                recv_data, recv_addr = sock.recvfrom(4096)
        except socket.error:
                print('UDP recieve timeout')

        print("Settings from NSCL: " + recv_data)
        json_recv_data = json.loads(recv_data)
        led_state = json_recv_data["e"]

        if led_state == 'true':
                GPIO.output(27, True)
        elif led_state == 'false':
                GPIO.output(27, False)

        data = {"id":GEYSER_ID, "t1":55, "t2":35, "e":led_state}
        time.sleep(5)

