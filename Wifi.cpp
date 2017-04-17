/*
 * Wifi.cpp
 *
 *  Created on: Jun 27, 2016
 *      Author: lieven
 */

#include "Wifi.h"
#include <Config.h>

Wifi::Wifi(const char* name) :
    Actor(name)
{
    _ssid = "ssid";
    _password = "password";
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
            INFO(" Wifi Connected.");
            INFO(" ip : %s ", WiFi.localIP().toString().c_str());
            eb.event(id(),H("connected")).addKeyValue(H("data"),true);
            eb.send();
        } else {
            WARN(" Wifi Disconnected.");
            eb.event(id(),H("disconnected")).addKeyValue(H("data"),true);
            eb.send();
        }
    }
}

void Wifi::setup()
{
    switchState(H("disconnected"));
    WiFi.hostname(Sys::hostname());
    WiFi.begin(_ssid.c_str(), _password.c_str());
    WiFi.enableSTA(true);
}

void Wifi::loop()
{
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG("Connecting to %s ... ",_ssid.c_str());
        WiFi.begin(_ssid.c_str(), _password.c_str());

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            DEBUG(" still connecting ");
            return;
        } else
            INFO(" starting Wifi host : '%s' on SSID : '%s' '%s' ", getHostname(),
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
