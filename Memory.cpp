#include "Memory.h"

Memory::Memory(const char* name) :Actor(name)
{
}

Memory::~Memory()
{
}

void Memory::setup()
{
    eb.onDst(id()).call(this);
    timeout(5000);
}

void Memory::onEvent(Cbor& msg)
{
    if ( timeout() ) {
        timeout(5000);
        eb.event(id(),H("interface")).addKeyValue(H("data"),"memory");
        eb.send();
    } else if ( eb.isRequest(H("set")) && msg.getKeyValue(H("address"),_address) && msg.getKeyValue(H("value"),_value)) {
        *(uint32_t*)_address = _value;
        eb.reply().addKeyValue(H("address"),_address).addKeyValue(H("value"),*(uint32_t*)_address);
        eb.send();
    } else if ( eb.isRequest(H("request")) && msg.getKeyValue(H("address"),_address) ) {
        eb.reply().addKeyValue(H("address"),_address).addKeyValue(H("value"),*(uint32_t*)_address);
        eb.send();
    }
};
