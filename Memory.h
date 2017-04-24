#ifndef MEMORY_H
#define MEMORY_H

#include <EventBus.h>

class Memory : public Actor
{
    uint32_t _address;
    uint32_t _value;
public:
    Memory(const char* name);
    ~Memory();
   
    void setup() ;
    void onEvent(Cbor& msg) ;
};

#endif // MEMORY_H
