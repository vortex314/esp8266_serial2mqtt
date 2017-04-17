/*
 * LedBlinker.cpp
 *
 *  Created on: Jul 1, 2016
 *      Author: lieven
 */

#include "LedBlinker.h"

#define PIN 16

LedBlinker::LedBlinker() :
    Actor("Led")
{
    _interval = 10;
    _isOn = true;
}

LedBlinker::~LedBlinker()
{
}

void LedBlinker::setup()
{
    init();
    timeout(100);
    eb.onSrc(_wifi).call(this);
    eb.onSrc(_mqtt).call(this);
}


void LedBlinker::init()
{
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, 1);

}

void LedBlinker::onEvent(Cbor& cbor)
{
    if (timeout()) {
        if (_isOn) {
            _isOn = false;
            digitalWrite(PIN, 1);
        } else {
            _isOn = true;
            digitalWrite(PIN, 0);
        }
        timeout(_interval);
    } else if ( eb.isEvent(_wifi,H("connected"))) {
        setInterval(100);
    } else if ( eb.isEvent(_wifi,H("disconnected"))) {
        setInterval(10);
    } else if ( eb.isEvent(_mqtt,H("connected"))) {
        setInterval(1000);
    } else if ( eb.isEvent(_mqtt,H("disconnected"))) {
        setInterval(100);
    }
}

void LedBlinker::setInterval(uint32_t interval)
{
    _interval=interval;
}

void LedBlinker::setWifi(uid_t wifi)
{
    _wifi=wifi;
}

void LedBlinker::setMqtt(uid_t mqtt)
{
    _mqtt=mqtt;
}
