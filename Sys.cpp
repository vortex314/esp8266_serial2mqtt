/*
 * Sys.cpp
 *
 *  Created on: May 15, 2016
 *      Author: lieven
 */
#include <Arduino.h>
#include <Sys.h>
#include <Log.h>

char Sys::_hostname[30]="ESP8266_DEF";
uint64_t Sys::_boot_time=0;

uint64_t Sys::millis()
{
	return ::millis();
}

uint64_t Sys::now()
{
	return _boot_time+Sys::millis();
}

void Sys::setNow(uint64_t n)
{
	_boot_time = n-Sys::millis();
}

void Sys::hostname(const char* h)
{
    LOGF("%s:%d",h,_hostname);
    Sys::delay(100);
	if ( _hostname )
	LOGF(" hostname : %s",_hostname);
    Sys::delay(100);
	strncpy(_hostname , h,strlen(h));
	LOGF(" hostname : %s ",_hostname);
}

void Sys::setHostname(const char* h)
{
	strncpy(_hostname , h,strlen(h)+1);
    return;
}

const char* Sys::hostname()
{
	return _hostname;
}

void Sys::delay(unsigned int delta)
{
	uint32_t end = ::millis()+ delta;
	while ( ::millis() < end );
}
