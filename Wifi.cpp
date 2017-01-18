/*
 * Wifi.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Wifi.h"
#include <Config.h>

Wifi::Wifi() :
    Actor("wifi")
{
    switchState(H("disconnected"));
    _ssid = "ssid";
    _password = "password";
    _hostname = Sys::hostname();
}

Wifi::~Wifi()
{
}

void Wifi::init()
{

}

void Wifi::switchState(int st)
{
    if (st != Actor::state()) {
        Actor::state(st);
        if (st == H("connected")) {
            LOGF(" Wifi Connected.");
            LOGF(" ip : %s ", WiFi.localIP().toString().c_str());
            eb.publish(H("wifi"),H("connected"));
        } else {
            LOGF(" Wifi Disconnected.");
            eb.publish(H("wifi"),H("disconnected"));
        }
    }
}

void Wifi::setup()
{
    WiFi.hostname(_hostname);
    WiFi.begin(_ssid.c_str(), _password.c_str());
    WiFi.enableSTA(true);
}

void Wifi::loop()
{
    if (WiFi.status() != WL_CONNECTED) {
        LOGF("Connecting to %s ... ",_ssid.c_str());
        WiFi.begin(_ssid.c_str(), _password.c_str());

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            LOGF(" still connecting ");
            return;
        } else
        LOGF(" starting Wifi host : '%s' on SSID : '%s' '%s' ", getHostname(),
         getSSID(), getPassword());

    }
    switchState(WiFi.status() == WL_CONNECTED ? H("connected") : H("disconnected"));
}

void Wifi::setConfig(String& ssid, String& password, String& hostname)
{
    _ssid = ssid;
    _password = password;
    _hostname = hostname;
}

const char*  Wifi::getHostname()
{
    return Sys::hostname();
}

const char*  Wifi::getSSID()
{
    return _ssid.c_str();
}

const char*  Wifi::getPassword()
{
    return _password.c_str();
}
