/*
 * DWM1000_Anchor_Tag.h
 *
 *  Created on: Feb 12, 2016
 *      Author: lieven
 */

#ifndef DWM1000_Anchor_H_
#define DWM1000_Anchor_H_

#include <EventBus.h>
#include <Peripheral.h>

class DWM1000_Anchor: public Actor
{
    uint32_t _count;
    Spi _spi;
    DigitalIn _irq;
    uint32_t _interrupts;
    uint32_t _polls;
    uint32_t _finals;
    uint32_t _errs;
    static DWM1000_Anchor* _anchor;
    enum { WAIT_POLL, WAIT_FINAL } _state;
    bool interrupt_detected ;
    uint32_t _frame_len;
public:
    DWM1000_Anchor(const char* name);
    virtual ~DWM1000_Anchor();
    void mode(uint32_t m);
    void setup();
    void resetChip();
    void initSpi();
    void onEvent(Cbor& msg);
    void enableIsr();
    static void my_dwt_isr();
    bool isInterruptDetected();
    bool clearInterrupt();
    void sendReply();
};

#endif /* DWM1000_Anchor_Tag_H_ */
