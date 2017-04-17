#include "Peripheral.h"
#include "Arduino.h"

DigitalIn::DigitalIn(uint32_t pin)
{
    _pin=pin;
    _interruptFunction =0;
    _instance=0;
    _port=0;
    _mode=0;
}

DigitalIn::~DigitalIn()
{
}

void DigitalIn::setMode(uint32_t mode) {
    _mode=mode;
}

void DigitalIn::setInterruptHandler(InterruptFunction f,void* instance)
{
    _interruptFunction = f;
    _instance=instance;
}

void DigitalIn::init()
{
    pinMode(_pin,INPUT);
    if ( _mode & DIN_INT_ENABLED) {
//        attachInterrupt(digitalPinToInterrupt(_pin),_interruptFunction,CHANGE);
    }
}


extern "C" {
#include "spi.h"
}

Spi::Spi(uint32_t base)
{
    _base = base;
}

Spi::~Spi()
{
}

void Spi::setMode(uint32_t mode)
{
    _mode=mode;
}

void Spi::setClock(uint32_t clock)
{
    _clock=clock;
}

void Spi::init()
{
    spi_init(HSPI);
    spi_mode(HSPI, 0, 0);
    //	spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
    spi_clock(HSPI, 10, 20);//
//	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
//	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
    spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
    spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
    spi_set_bit_order(0);
    WRITE_PERI_REG(SPI_CTRL2(HSPI), 0xFFFFFFFF);

    WRITE_PERI_REG(SPI_CTRL2(HSPI),
                   (( 0xF & SPI_CS_DELAY_NUM ) << SPI_CS_DELAY_NUM_S) |//
                   (( 0x1 & SPI_CS_DELAY_MODE) << SPI_CS_DELAY_MODE_S) |//
                   (( 0xF & SPI_SETUP_TIME )<< SPI_SETUP_TIME_S ) |//
                   (( 0xF & SPI_HOLD_TIME )<< SPI_HOLD_TIME_S ) |//
                   (( 0xF & SPI_CK_OUT_LOW_MODE )<< SPI_CK_OUT_LOW_MODE_S ) |//
                   (( 0xF & SPI_CK_OUT_HIGH_MODE )<< SPI_CK_OUT_HIGH_MODE_S ) |//
                   (( 0x2 & SPI_MISO_DELAY_MODE )<< SPI_MISO_DELAY_MODE_S ) |//
                   (( 0x0 & SPI_MISO_DELAY_NUM ) << SPI_MISO_DELAY_NUM_S) |//
                   (( 0x0 & SPI_MOSI_DELAY_MODE )<< SPI_MOSI_DELAY_MODE_S ) |//
                   (( 0x0 & SPI_MOSI_DELAY_NUM ) << SPI_MOSI_DELAY_NUM_S) |//
                   0);
}
