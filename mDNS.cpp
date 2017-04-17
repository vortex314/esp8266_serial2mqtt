/*
 * mDNS.cpp
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#include "mDNS.h"
//MDNSResponder responder;

mDNS::mDNS(Wifi& wifi) :
    Actor("mdns") ,_wifi(wifi)
{
    _port = 2000;
    _service = "wibo";
}

mDNS::~mDNS()
{
}

void mDNS::setConfig(String& service, uint16_t port)
{
    _service = service;
    _port = port;
}

void mDNS::setup()
{
    eb.onEvent(H("wifi"),H("connected")).call(this,(MethodHandler)&mDNS::onWifiConnected);
}

void mDNS::onEvent(Cbor& cbor)
{

}

void mDNS::onWifiConnected()
{
    if (!MDNS.begin(WiFi.hostname().c_str(),WiFi.localIP())) {
        WARN("Error setting up MDNS responder!");
    }
    INFO("publish service %s:%d",_service.c_str(),_port);
//    MDNS.addService(_service, "tcp", _port);
//    MDNS.addServiceTxt(_service,"tcp","key","value");
//    MDNS.addService("ws", "tcp", 81);
    MDNS.addService("wibo", "udp", _port);
    MDNS.addServiceTxt("wibo", "udp","mcu","STM32");
//    responder.begin(WiFi.hostname().c_str());
}

void mDNS::loop()
{
    MDNS.update();
//   responder.update();
}

IPAddress mDNS::query(const char* service)
{
    for(int i=0; i< 5; i++) {
        INFO(" looking for MQTT host ");

        int number = MDNS.queryService(service, "tcp");

        if (number > 0) {

            for (uint8_t result = 0; result < number; result++) {
                int port = MDNS.port(result);
                String host = MDNS.hostname(result);
                IPAddress IP = MDNS.IP(result);
                INFO("Service Found [%u] %s (%u.%u.%u.%u) port = %u\n", result,
                     host.c_str(), IP[0], IP[1], IP[2], IP[3], port);
                return IP;
            }
        }
    }
    return IPAddress(37,187,106,16);	// test.mosquitto.org
}
