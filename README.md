# DUMB UPS - Synology NAS (and ESP8266)
This small project stems from the need to make an old UPS "smart" in order to notify network-connected devices of power outages. There are two versions available, one more advanced and one more basic.

# I Version - Dead Man Server
The first version consists of a "Dead Man Server," which is a device not connected to the UPS power supply but directly to the household electrical network. When this device goes offline (dead), I will know that the power-connection has been lost.


# II Version - API Server
The second version, will be an API server, a device connected directly to the UPS power supply and with physical connections to the UPS LEDs. This will allow us to read the system status and any power outages. It goes without saying that this type of implementation must be adapted to each specific NAS, and it involves opening it up and making solder connections inside the UPS's motherboard.

# Synology NAS 
Once the ESP8266 server is configured, the next step is to connect to the Synology NAS via SSH and upload the "dump_script.py" file. Then, you'll need to set up a cron job that will execute this script every minute.


## Acknowledgements  
- [power_safety.sh - Original bash ispiration script](https://gist.github.com/zeraien/5df1af8603d23b46e3cdd7adbd5764c2)
