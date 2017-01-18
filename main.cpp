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
#include <MqttJson.h>
#include <System.h>
#include <BootLoader.h>
#include <UdpServer.h>
#include <MqttJson.h>

uint32_t BAUDRATE = 115200;

Uid uid(200);
EventBus eb(2048,300);
Log logger(256);




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
MqttJson router("router");
System systm;
BootLoader bootloader("bootloader");
UdpServer udp("udp");


#include <main_labels.h>

void setup()
{
    Serial.begin(BAUDRATE, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
    Serial.setDebugOutput(false);
    LOGF("version : " __DATE__ " " __TIME__);
    LOGF("WIFI_SSID '%s'  ",WIFI_SSID);

    String hostname,ssid,pswd;
    Sys::init();
    LOGF("");
    char hn[20];
//   hostname = "wibo_";
//    hostname += ESP.getChipId();
//    sprintf(hn,"wibo_%X",ESP.getChipId());
    strcpy(hn,"wibo");
    hostname = hn;
    LOGF("%s",hn);
    Sys::hostname(hn);
 
    logger.level(Log::LOG_TRACE);

    ssid = WIFI_SSID;
    pswd= WIFI_PSWD;
    hostname=hn;
    wifi.setConfig(ssid,pswd,hostname);
    mdns.setConfig(hostname,2000);
    LOGF(" starting Wifi host : '%s' on SSID : '%s' '%s' ", wifi.getHostname(),
         wifi.getSSID(), wifi.getPassword());

    eb.onAny().subscribe([](Cbor& msg) {
        Str str(256);
        eb.log(str,msg);
        LOGF("%s",str.c_str());
    });

    uid.add(labels,LABEL_COUNT);
    wifi.setup();
    mdns.setup();
    timer.setup();
    
    mqtt.setup();
    router.setMqttId(mqtt.id());
    router.setup();

    led.setup();
    systm.setup();
    bootloader.setup();
    eb.onEvent(bootloader.id(), 0).subscribe(&router,(MethodHandler) &MqttJson::ebToMqtt);
    udp.setup();
//	eb.onEvent(H("system"),H("state")).subscribe(&router,(MethodHandler)&Router::ebToMqtt); // publisize timer-state events


    return;
}
extern "C"  void loop()
{
    eb.eventLoop();
    wifi.loop();
    mqtt.loop();
    mdns.loop();
    udp.loop();
}
