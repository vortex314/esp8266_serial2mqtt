ARDUINO_VARIANT = nodemcu
SERIAL_PORT = /dev/ttyUSB1
UPLOAD_SPEED = 921600 # 230400
SERIAL_BAUD = 115200
# uncomment and set the right serail baud according to your sketch (default to 115200)
#SERIAL_BAUD = 115200
# uncomment this to use the 1M SPIFFS mapping
#SPIFFS_SIZE = 1
USER_DEFINE = -DWIFI_SSID=\"Merckx3\" -DWIFI_PSWD=\"LievenMarletteEwoutRonald\" \
	-I../Common/inc -I../pubsubclient/src -DMQTT_MAX_PACKET_SIZE=300
USER_LINK = -L../Common/Debug -lCommon -lmain
OTA_IP = 192.168.1.184
OTA_PORT = 8266 
OTA_AUTH = password
USER_LIBS= pubsubsclient
include ../Esp8266-Arduino-Makefile/esp8266Arduino.mk 
