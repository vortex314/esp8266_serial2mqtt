/*
 * Gpio.h
 *
 *  Created on: Nov 17, 2015
 *      Author: lieven
 */

#ifndef GPIO_H_
#define GPIO_H_
#include "Erc.h"

class Gpio {
private:
	uint8_t _pin;
	bool _digitalValue;
	bool _input;
	// I/O : D/P
	typedef enum {
		MODE_DISABLED = 0,
		MODE_OUTPUT_OPEN_DRAIN,
		MODE_OUTPUT_PUSH_PULL,
		MODE_INPUT,
		MODE_INPUT_PULLUP,

	} Mode;
	Mode _mode;
	static const char* _sMode[];
public:
	Gpio(uint8_t pin) ;
	virtual ~Gpio() ;
	Erc setMode(const char* str) ;
	Erc getMode(char* str) ;
	Erc digitalWrite(uint8_t i) ;
	Erc digitalRead(uint8_t* i) ;
	Erc analogRead(int v) ;
	Erc analogWrite(int* v) ;
};

#endif /* GPIO_H_ */
