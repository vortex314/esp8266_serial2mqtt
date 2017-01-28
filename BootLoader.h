#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <EventBus.h>
#include <user_config.h>
#include <Base64.h>
#include <Bytes.h>
#include <Config.h>
class BootLoader : public Actor
{
    bool _serialSwapped;
    bool _boot0;
    uint64_t _timeout;
    uint32_t _baudrate;
    uint32_t _usartRxd;
    uint64_t _endTime;
    Bytes _in;
public:
    BootLoader(const char* name);
    ~BootLoader();
    typedef enum {
        M_SYSTEM,M_FLASH
    } Mode;

private:
    Mode _mode;
public:
    enum Op {
        X_WAIT_ACK = 0x40,
        X_SEND = 0x41,
        X_RECV = 0x42,
        X_RECV_VAR = 0x43,
        X_RECV_VAR_MIN_1 = 0x44,
        X_RESET,
        X_BOOT0
    };

    void init();
    void setup();
    void onEvent(Cbor& cbor);
    void loop();
    void flush();
    Erc begin();

    void report();
    Erc resetPulse();
    Erc getId(uint16_t& id);
    Erc getVersion(uint8_t& version);
    Erc get(uint8_t& version, Bytes& cmds);
    Erc writeMemory(uint32_t address, Bytes& data);
    Erc readMemory(uint32_t address, uint32_t length, Bytes& data);
    Erc eraseMemory(Bytes& pages);
    Erc extendedEraseMemory();
    Erc eraseAll();
    Erc writeProtect(Bytes& sectors);
    Erc writeUnprotect();
    Erc readoutProtect();
    Erc readoutUnprotect();
    Erc go(uint32_t address);
    int getMode() {
        return _mode;
    }

    Erc waitAck(Bytes& out,  uint32_t timeout);
    Erc readTillAck(Bytes& in, uint32_t timeout);
    Erc readVar(Bytes& in, uint32_t max, uint32_t timeout);
    Erc read(Bytes& in, uint32_t lenghth, uint32_t timeout);
    Erc setBoot0(bool);
    Erc serialSwap(bool);
    Erc engine(Bytes& reply, Bytes& req);
    bool endWait();
    void wait(uint32_t delta);
};


#endif // BOOTLOADER_H
