/*
 * Sys.cpp
 *
 *  Created on: May 15, 2016
 *      Author: lieven
 */
#include <Arduino.h>
#include <Sys.h>

char Sys::_hostname[30]="UNDEFINED";
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

void Sys::hostname(const char* hostname)
{
	LOGF(" hostname : %s",_hostname);
	strncpy(_hostname , hostname,sizeof(_hostname));
	LOGF(" hostname : %s ",_hostname);
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
