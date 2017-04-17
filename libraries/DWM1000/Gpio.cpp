/*
 * Gpio.cpp
 *
 *  Created on: Nov 17, 2015
 *      Author: lieven
 */

#include <Gpio.h>
#include "Arduino.h"
#include <gpio_c.h>
#include "esp8266_peri.h"

/*
 * 		MODE_DISABLED = 0,
 MODE_OUTPUT_OPEN_DRAIN,
 MODE_OUTPUT_PUSH_PULL,
 MODE_INPUT,
 MODE_INPUT_PULLUP
 */

const char* Gpio::_sMode[] = { "DIS", "OOD", "OPP", "INP", "IPU" };
const int arduinoModes[] = { INPUT, OUTPUT_OPEN_DRAIN, OUTPUT, INPUT,
INPUT_PULLUP };

Gpio::Gpio(uint8_t pin) {
	_pin = 1 << pin;
	_mode = MODE_DISABLED;
	_digitalValue = 1;
	_input = true;
	setMode("IUIE");
}

Gpio::~Gpio() {

}

/*
 *  <Input/Output/None> <Outputdrain/Pushpull> <Interrupt/None> <Disable,Rising,Falling,Change,Low,High>
 */

Erc Gpio::setMode(const char* str) {
	uint32_t mode;
	if (str[0] == 'I' && str[1] == 'N')
		mode = INPUT;
	else if (str[0] == 'I' && str[1] == 'P')
		mode = INPUT_PULLUP;
	else if (str[0] == 'O' && str[1] == 'P')
		mode = OUTPUT;
	else if (str[0] == 'O' && str[1] == 'O')
		mode = OUTPUT_OPEN_DRAIN;
	else if (str[0] == 'N') {
		mode = DISABLED;
	} else
		return EINVAL;

	pinMode(_pin, mode);

	return E_OK;
}
#include <string.h>
Erc Gpio::getMode(char* str) {
	char mode[5];
	strcpy(mode,"----");
	if (GPEP(_pin)) {	// output enable
		mode[0] = 'O';
		if ( GPC(_pin) &  GPCD ) {
			mode[1]='D';
		} else {
			mode[1]='P';
		};

	} else {
		mode[0] = 'I';


	}
	strcpy(str,mode);
	return E_OK;
}

Erc Gpio::digitalWrite(uint8_t i) {
	if (i)
		::digitalWrite(_pin, 1);
	else
		::digitalWrite(_pin, 0);
	return E_OK;
}

Erc Gpio::digitalRead(uint8_t* i) {
	*i = ::digitalRead(_pin);
	return E_OK;
}

