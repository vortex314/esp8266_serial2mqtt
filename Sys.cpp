/*
 * Sys.cpp
 *
 *  Created on: May 15, 2016
 *      Author: lieven
 */
#include <Arduino.h>
#include <Sys.h>

uint64_t Sys::millis(){
	return ::millis();
}

const char* Sys::hostname(){
	return "ESP8266";
}

void Sys::delay(unsigned int delta){
	uint32_t end = ::millis()+ delta;
	while ( ::millis() < end );
}
