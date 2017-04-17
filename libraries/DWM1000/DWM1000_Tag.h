/*
 * DWM1000_Tag.h
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#ifndef DWM1000_Tag_H_
#define DWM1000_Tag_H_
#include <Log.h>
#include <EventBus.h>
#include <Peripheral.h>

class DWM1000_Tag: public Actor
{
    uint32_t _count;
    Spi _spi;
    static DWM1000_Tag* _tag;
    uint32_t _interrupts;
    bool interrupt_detected;
    uint32_t _resps;
    uint32_t _frame_len;
public:
    DWM1000_Tag(const char* name);
    virtual ~DWM1000_Tag();
    void mode(uint32_t m);
    void setup();
    void resetChip();
    void initSpi();
    static void my_dwt_isr();
    bool isInterruptDetected();
    bool clearInterrupt();
    void enableIsr();
    void onEvent(Cbor& msg);
};

#endif /* DWM1000_Tag_H_ */
