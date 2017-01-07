#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <pins_arduino.h>
#include <Sys.h>
#include <Wifi.h>
#include <LedBlinker.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <mDNS.h>
#include <ctype.h>
#include <uart.h>
#include <Udp.h>
#include <ctype.h>
#include <cstdlib>
#include <Base64.h>
#include <Config.h>
#include <PubSubClient.h>
#include <EventBus.h>
#include <Mqtt.h>

uint32_t BAUDRATE = 115200;
// Wifi wifi;


EventBus eb(2048,300);

//________________________________________________Se_________________
#ifndef WIFI_SSID
#define WIFI_SSID "Merckx3"
#endif
#ifndef WIFI_PSWD
#define WIFI_PSWD "LievenMarletteEwoutRonald"
#endif
Str line(20);
class Timer : public Actor
{
uint32_t _counter;
public:
	Timer():Actor("timer") {
_counter=0;
	}
	void setup() {
		eb.onDst(H("timer")).subscribe(this);
		eb.onEvent(H("mqtt"),0).subscribe(this);
		timeout(1000);
	}

	void onEvent(Cbor& msg) {
		PT_BEGIN();
		PT_YIELD_UNTIL(eb.isEvent(H("mqtt"),H("connected")));
SUBSCRIBING : {
			while(true) {
				timeout(10000);
				eb.request(H("mqtt"),H("subscribe"),id()).addKeyValue(H("topic"),"dst/esp8266/#").addKeyValue(H("qos"),1);
				eb.send();
				PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(H("mqtt"),H("subscribe")));
				if ( ! timeout()) goto PUBLISHING;
			}
		}
PUBLISHING : {
			while(true) {
				timeout(100);
				PT_YIELD_UNTIL(timeout());
				timeout(1000);
				line.clear();
				line.append(_counter);
				eb.request(H("mqtt"),H("publish"),id()).addKeyValue(H("topic"),"dst/stm32").addKeyValue(H("message"),line);
				eb.send();
				PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(H("mqtt"),H("publish")));
				_counter++;
			}
		}
		PT_END();
	}
};
Wifi wifi;
mDNS mdns(wifi);
Timer timer;
LedBlinker led;
Mqtt mqtt;

extern void logCbor(Cbor&);

extern "C" void setup()
{
	Serial.begin(BAUDRATE, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_FULL);
	Serial.setDebugOutput(false);
	LOGF(" version : " __DATE__ " " __TIME__);
	String ssid(20);
	ssid=WIFI_SSID;
	String pswd(50);
	pswd=WIFI_PSWD;
	String hostname(20);
	hostname="ESP8277";
	wifi.setConfig(ssid,pswd,hostname);
	LOGF(" starting Wifi host : '%s' on SSID : '%s' '%s' ", wifi.getHostname(),
	     wifi.getSSID(), wifi.getPassword());
	delay(100);	// flush serial delay
	eb.onAny().subscribe(logCbor);

	wifi.setup();
	mdns.setup();
	timer.setup();
	mqtt.setup();

	return;
}

extern "C" void loop()
{
	eb.eventLoop();
	wifi.loop();
	mqtt.loop();
}
