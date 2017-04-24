#include "Peripheral.h"
#include "Arduino.h"
#include <Log.h>

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

void DigitalIn::setMode(uint32_t mode)
{
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

#define NO_GLOBAL_INSTANCES
extern "C" {
 #include "spi.h"
}
#include <SPI.h>

SPIClass __spi;
#define SPI_MAX_BYTES 64
union {
    uint32_t i32;
    uint8_t _spi_out[SPI_MAX_BYTES];
} o;

union {
    uint32_t ii32;
    uint8_t _spi_in[SPI_MAX_BYTES];
} i;



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
#define HSPI 1
void Spi::init()
{
    __spi.begin();
    __spi.setHwCs(false);
    __spi.setFrequency(400000);
 //   __spi.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    __spi.setDataMode(SPI_MODE0);
    __spi.setBitOrder(LSBFIRST);
    
    INFO(" SPI_CTRL  0x%08X",READ_PERI_REG(SPI_CTRL(HSPI)));
    INFO(" SPI_CTRL1 0x%08X",READ_PERI_REG(SPI_CTRL1(HSPI)));
    INFO(" SPI_CTRL2 0x%08X",READ_PERI_REG(SPI_CTRL2(HSPI)));
    INFO(" SPI_CLOCK 0x%08X",READ_PERI_REG(SPI_CLOCK(HSPI)));
    INFO(" SPI_USER  0x%08X",READ_PERI_REG(SPI_USER(HSPI)));
    INFO(" SPI_USER1  0x%08X",READ_PERI_REG(SPI_USER1(HSPI)));
    INFO(" SPI_USER2  0x%08X",READ_PERI_REG(SPI_USER2(HSPI)));

    /*
    spi_init(HSPI);
    spi_mode(HSPI, 0, 0);
    //	spi_clock(HSPI, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
    //   spi_clock(HSPI,10,20);
    if ( _clock==SPI_CLK_1MHZ ) {
        spi_clock(HSPI, 4, 20);
    } else if ( _clock==SPI_CLK_10MHZ ) {
        spi_clock(HSPI, 4, 20);
    }
    //
    //	spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
    //	spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_HIGH_TO_LOW);
    spi_tx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
    spi_rx_byte_order(HSPI, SPI_BYTE_ORDER_LOW_TO_HIGH);
    spi_set_bit_order(0);
    WRITE_PERI_REG(SPI_CTRL2(HSPI), 0xFFFFFFFF);

    WRITE_PERI_REG(SPI_CTRL2(HSPI),
                   (( 0x1 & SPI_CS_DELAY_NUM ) << SPI_CS_DELAY_NUM_S) |//
                   (( 0x1 & SPI_CS_DELAY_MODE) << SPI_CS_DELAY_MODE_S) |//
                   (( 0x1 & SPI_SETUP_TIME )<< SPI_SETUP_TIME_S ) |//
                   (( 0x1 & SPI_HOLD_TIME )<< SPI_HOLD_TIME_S ) |//
                   (( 0x1 & SPI_CK_OUT_LOW_MODE )<< SPI_CK_OUT_LOW_MODE_S ) |//
                   (( 0x1 & SPI_CK_OUT_HIGH_MODE )<< SPI_CK_OUT_HIGH_MODE_S ) |//
                   (( 0x2 & SPI_MISO_DELAY_MODE )<< SPI_MISO_DELAY_MODE_S ) |//
                   (( 0x0 & SPI_MISO_DELAY_NUM ) << SPI_MISO_DELAY_NUM_S) |//
                   (( 0x0 & SPI_MOSI_DELAY_MODE )<< SPI_MOSI_DELAY_MODE_S ) |//
                   (( 0x0 & SPI_MOSI_DELAY_NUM ) << SPI_MOSI_DELAY_NUM_S) |//
                   0);
                   /*
                    *
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
                    */

}

//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
extern "C" int writetospi(uint16 hLen, const uint8 *hbuff, uint32 bLen,
                          const uint8 *buffer)
{
    if ( (hLen+bLen) > 64 ) {
        INFO("SPI write buffer too big %d %d ",hLen,bLen);
        return -1;
    }
    digitalWrite(15, 0);
    memcpy(o._spi_out,hbuff,hLen);
    memcpy(&o._spi_out[hLen],buffer,bLen);
    __spi.transferBytes(o._spi_out,i._spi_in,hLen+bLen);
    digitalWrite(15, 1);
    return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

extern "C" int readfromspi(uint16 hLen, const uint8 *hbuff, uint32 bLen, uint8 *buffer)
{
   if ( (hLen+bLen) > 64 ) {
       INFO("SPI read buffer too big %d %d ",hLen,bLen);
       return -1;
   }
    digitalWrite(15, 0);
    memcpy(o._spi_out,hbuff,hLen);
    memcpy(&o._spi_out[hLen],buffer,bLen);
    __spi.transferBytes(o._spi_out,i._spi_in,hLen+bLen);
    memcpy(buffer,&i._spi_in[hLen],bLen);
    digitalWrite(15, 1);
   return 0;
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
extern "C" void spi_set_rate_low()
{
    __spi.setFrequency(400000);
}
//////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
extern "C" void spi_set_rate_high()
{
    __spi.setFrequency(1000000);
}
