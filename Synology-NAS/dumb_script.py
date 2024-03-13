#!/usr/bin/python
import os
import time
import json
import requests

# the shutdown timer
SHUTDOWN_DELAY = 10 * 60

# the shutdown command
SHUTDOWN_CMD = "synoshutdown --shutdown"

DUMB_MACHINE = "esp8266_ip_address"
API_ADDRESS = "/api/states"


# INPUT: Synology Effect Number
# OUTPUT: bash command
# Synology DiskStation NAS LED and Beeps
# You can make your diskstation beep or change the LED state from the command line.
# Must be run as root.
# Example to make a long beep: `# echo 3 > /dev/ttyS1`
#
# Echo one of these for the desired effect:
# 2 - short beep
# 3 - longer beep
# 4 - blue power LED on
# 5 - blue power LED blink
# 6 - blue power LED off
# 7 - status LED off
# 8 - status LED on
# 9 - status LED blink
def signal(var):
    return "echo " + str(var) + " > /dev/ttyS1"

HOST_UP  = os.system(f"ping -c 1 {DUMB_MACHINE} > /dev/null") == 0
if(HOST_UP):
    response = requests.get("http://" + DUMB_MACHINE + API_ADDRESS, timeout=(12, None))

    while (response.status_code != 200):
        response = requests.get("http://" + DUMB_MACHINE + API_ADDRESS)

    #print(response.json())

    # normal mode
    os.system(signal(4))
    os.system(signal(8))

    timeLooseLine = response.json()["TimelinelooseEpoc"]
    timeNow = response.json()["TimeNowEpoc"]

    if (response.json()["mode"] == "battery"):
        diff = int(timeNow) - int(timeLooseLine)
        print("--- Running on battery for " + str(int(diff/60)) + " minutes ---")

        # the end, shutdown!
        if (diff > SHUTDOWN_DELAY):
            print("Will power down now, running SHUTDOWN_CMD")
            os.system(SHUTDOWN_CMD)        

        # extra final heartbeat warning
        elif (diff > SHUTDOWN_DELAY - 60 ):
            print("Final heartbeat warning!")
            os.system(signal(3))
            time.sleep(1)
            os.system(signal(3))
            time.sleep(1)
            os.system(signal(3))

        # warn user that we are running on battery
        elif (diff > 3 * 60):
            print("Warning Signal Beep")
            os.system(signal(2))
            time.sleep(1)
            os.system(signal(9))
else:
    print("DUMB_MACHINE (" + DUMB_MACHINE + ") is down!")
