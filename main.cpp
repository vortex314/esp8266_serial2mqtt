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
EventBus eb(2048,1024);
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
    uid_t _bl;
public:
    Timer():Actor("timer") {
        _counter=0;
        _bl =  H("bootloader");
    }
    void setup() {
        eb.onDst(H("timer")).subscribe(this);
        eb.onEvent(H("mqtt"),0).subscribe(this);
        timeout(1000);
    }

    void onEvent(Cbor& msg) {
        PT_BEGIN();
WAIT_CONNECTED : {
            while(true) {
                eb.request(H("mqtt"),H("connect"),id());
                eb.send();
                timeout(3000);
                PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(H("mqtt"),H("connected")));
                if ( ! timeout()) goto TEST;
                INFO(" waiting mqtt connect .. ");
            }
        }
TEST : {
            while(true) {
                eb.request(_bl,H("resetBootloader"),id());
                eb.send();
                timeout(3000);
                PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(_bl,H("resetBootloader")));
                if ( timeout()) goto TEST;
                eb.request(_bl,H("get"),id());
                eb.send();
                timeout(3000);
                PT_YIELD_UNTIL(timeout() || eb.isReplyCorrect(_bl,H("get")));
                if ( timeout()) goto TEST;
            }
        }

        PT_END();
    }
};
Wifi wifi;
mDNS mdns(wifi);
//Timer timer;
LedBlinker led;
Mqtt mqtt("mqtt",1024);
MqttJson router("router",1024);
System systm;
BootLoader bootloader("bootloader");
UdpServer udp("udp");


#include <main_labels.h>

void setup()
{
    Serial.begin(BAUDRATE, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL); // 8E1 for STM32
    Serial.setDebugOutput(false);
    INFO("version : " __DATE__ " " __TIME__);
    INFO("WIFI_SSID '%s'  ",WIFI_SSID);

    String hostname,ssid,pswd;
    Sys::init();
    INFO("");
    char hn[20];
//   hostname = "wibo_";
//    hostname += ESP.getChipId();
//    sprintf(hn,"wibo_%X",ESP.getChipId());
    strcpy(hn,"wibo");
    hostname = hn;
    INFO(" hostname : %s",hn);
    Sys::hostname(hn);

    logger.level(Log::LOG_TRACE);

    ssid = WIFI_SSID;
    pswd= WIFI_PSWD;
    hostname=hn;
    wifi.setConfig(ssid,pswd,hostname);
    mdns.setConfig(hostname,2000);
    INFO(" starting Wifi host : '%s' on SSID : '%s' '%s' ", wifi.getHostname(),
         wifi.getSSID(), wifi.getPassword());

    eb.onAny().subscribe([](Cbor& msg) {
        Str str(256);
        eb.log(str,msg);
        DEBUG("%s",str.c_str());
    });

    uid.add(labels,LABEL_COUNT);
    wifi.setup();
    mdns.setup();
//    timer.setup();

    mqtt.setup();
    router.setMqttId(mqtt.id());
    router.setup();

    led.setup();
    systm.setup();
    bootloader.setup();
    
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
    bootloader.loop();
}
