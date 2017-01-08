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
#include <Router.h>
#include <System.h>

uint32_t BAUDRATE = 115200;
// Wifi wifi;


Uid uid(100);
EventBus eb(2048,300);

Str str(300);
void logCbor(Cbor& cbor)
{
	cbor.offset(0);
	uid_t key,value;
	str.clear();
	Cbor::PackType ct;
	cbor.offset(0);
	while (cbor.hasData()) {
		cbor.get(key);
		const char* label = uid.label(key);
		str.append('"').append(label).append("\":");
		if (label[0]=='#' ) {
			cbor.get(value);
			str.append('"').append(uid.label(value)).append("\"");
		} else {
			ct = cbor.tokenToString(str);
			if (ct == Cbor::P_BREAK || ct == Cbor::P_ERROR)
				break;
		}
		if (cbor.hasData())
			str << ",";
	};
	LOGF("%s", str.c_str());
}

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
WAIT_CONNECTED :
		PT_YIELD_UNTIL(eb.isEvent(H("mqtt"),H("connected")));
		goto PUBLISHING;
SUBSCRIBING : {
			while(true) {
				timeout(3000);
				eb.request(H("mqtt"),H("subscribe"),id()).addKeyValue(H("topic"),"dst/esp8266/#").addKeyValue(H("qos"),1);
				eb.send();
				PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(H("mqtt"),H("subscribe")));
				if ( ! timeout()) goto PUBLISHING;
			}
		}
PUBLISHING : {
			while(true) {
				timeout(5000);
				PT_YIELD_UNTIL(timeout());
				line.clear();
				line.append(_counter);
//				eb.request(H("mqtt"),H("publish"),id()).addKeyValue(H("topic"),"dst/stm32").addKeyValue(H("message"),line);
//				eb.send();
				eb.request(H("system"),H("state"),id())
				.addKeyValue(EB_DST_DEVICE,H("stm32"))
				.addKeyValue(H("time"),Sys::millis())
				.addKeyValue(H("count"),line)
				.addKeyValue(H("$bytes"),(Bytes&)line)
				.addKeyValue(H("boolean"),false)
				.addKeyValue(H("float"),1.23)
				.addKeyValue(H("#parity"),H("even"));
				eb.send();
				timeout(1000);
				PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(H("mqtt"),H("publish")) || eb.isEvent(H("mqtt"),H("disconnected")));
				if (  eb.isEvent(H("mqtt"),H("disconnected")))
					goto WAIT_CONNECTED;
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
Router router;
System systm;

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
	hostname="esp8266";
	Sys::hostname("esp8266");
	wifi.setConfig(ssid,pswd,hostname);
	LOGF(" starting Wifi host : '%s' on SSID : '%s' '%s' ", wifi.getHostname(),
	     wifi.getSSID(), wifi.getPassword());
	delay(100);	// flush serial delay
	eb.onAny().subscribe(logCbor);

	wifi.setup();
	mdns.setup();
	timer.setup();
	mqtt.setup();
	router.setup();
	led.setup();
	systm.setup();
//	eb.onEvent(H("system"),H("state")).subscribe(&router,(MethodHandler)&Router::ebToMqtt); // publisize timer-state events


	return;
}

extern "C" void loop()
{
	eb.eventLoop();
	wifi.loop();
	mqtt.loop();
	mdns.loop();
}
/*,
H"dst"),H("src"),H("request"),H("reply"),H("event"),H("error"),H("Actor"),H("bootTime"),H("clientId"),H("connect"),H("connected"),H("data"),H("disconnect"),H("disconnected"),H("#dst"),
H("#dst_device"),H("error"),H("error_msg"),H("#event"),H("#from"),H("host"),H("hostname"),H("id"),H("init"),H("keep_alive"),H("line"),H("message"),H("motor"),H("mqtt"),H("now"),
H("prefix"),H("props"),H("publish"),H("published"),H("register"),H("Relay"),H("#reply"),H("#request"),H("reset"),H("Router"),H("rxd"),H("set"),H("setup"),H("#src"),H("#src_device"),H("state"),
H("status"),H("subscribe"),H("system"),H("timeout"),H("topic"),H("upTime"),H("will_message"),H("will_topic"),
*/