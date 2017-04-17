/*
 * Sys.cpp
 *
 *  Created on: May 15, 2016
 *      Author: lieven
 */
#include <Arduino.h>
#include <Sys.h>
#include <Log.h>
/*
 * 
 *  ATTENTION : LOGF call Sys::hostname, could invoke another call to Sys::hostname with LOGF,.... 
 * 
 */
char Sys::_hostname[30];
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
	strncpy(_hostname , h,strlen(h)+1);
}

void Sys::setHostname(const char* h)
{
	strncpy(_hostname , h,strlen(h)+1);
}

void Sys::init(){
    sprintf(_hostname,"wibo_%X",ESP.getChipId());
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

extern "C" uint64_t SysMillis() {
    return Sys::millis();
}
