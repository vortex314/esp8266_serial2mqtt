#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <Bytes.h>

//------------------------------------------------ DIGITALIN
typedef void (*InterruptFunction)(void*);

#define DIN_PULLUP  1
#define DIN_INT_ON_CHANGE 2
#define DIN_INT_ON_RISING  4
#define DIN_INT_ON_FALLING 8
#define DIN_INT_ENABLED  (DIN_INT_ON_CHANGE | DIN_INT_ON_RISING | DIN_INT_ON_FALLING)

class DigitalIn
{
    uint32_t _port;
    uint8_t _pin;
    uint32_t _mode;
    InterruptFunction _interruptFunction;
    void* _instance;
public:
    DigitalIn(uint32_t pin);
    DigitalIn(uint32_t port,uint8_t pin);
    ~DigitalIn();
    void setMode(uint32_t mode);
    void init();
    int read();
    void setInterruptHandler(InterruptFunction f,void* instance);
};
//---------------------------------------------- SPI
#define SPI_CLK_1MHZ 1
#define SPI_CLK_10MHZ 2

class Spi
{
    uint32_t _base;
    uint8_t _mode;
    uint32_t _clock;
    bool lsbFirst;
public:
    Spi(uint32_t base);
    ~Spi();
    void setMode(uint32_t mode);
    void setClock(uint32_t hz);
    void setLsbFirst(bool lsbFirst);
    void init();
    void exchange(Bytes& in,Bytes& out);
};

#endif // SPI_H
