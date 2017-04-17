/*
 * LedBlinker.h
 *
 *  Created on: Jul 1, 2016
 *      Author: lieven
 */

#ifndef LEDBLINKER_H_
#define LEDBLINKER_H_

#include <Actor.h>
#include <Arduino.h>
#include <EventBus.h>

class LedBlinker: public Actor {
	uint32_t _interval; //
	bool _isOn;
    uid_t _mqtt;
    uid_t _wifi;
public:
	LedBlinker();
	virtual ~LedBlinker();
	void setInterval(uint32_t interval);
    void setWifi(uid_t wifi);
    void setMqtt(uid_t mqtt);
	void blinkSlow();
	void onEvent(Cbor&);
	void loop();
	void init();
	void setup();
};

#endif /* LEDBLINKER_H_ */
