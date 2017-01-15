/*
 * mDNS.h
 *
 *  Created on: Jul 2, 2016
 *      Author: lieven
 */

#ifndef MDNS_H_
#define MDNS_H_
#include <Actor.h>
#include <Wifi.h>
#include <ESP8266mDNS.h>
class mDNS :public Actor {
	String _service;
	uint16_t _port;
	Wifi& _wifi;
public:
	mDNS(Wifi& src);
	virtual ~mDNS();
	void onEvent(Cbor& msg);
	void loop();
	void onWifiConnected();
	void setup();
	void setConfig(String& service,uint16_t port);
	IPAddress query(const char* service);
};

#endif /* MDNS_H_ */
