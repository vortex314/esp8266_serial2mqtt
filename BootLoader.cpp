#include "BootLoader.h"


BootLoader::BootLoader(const char* name):
    Actor(name)
{
    _mode = M_SYSTEM;
    _alt_serial = false;
    _timeout = 0;
    _boot0 = true;
    _usartRxd = 0;
}

BootLoader::~BootLoader()
{
}


#define PIN_RESET 4 // GPIO4 D2
#define PIN_BOOT0 5 // GPIO5 D1
#define ACK 0x79
#define NACK 0x1F
#define DELAY 10

enum {
    BL_GET = 0,
    BL_GET_VERSION = 1,
    BL_GET_ID = 2,
    BL_READ_MEMORY = 0x11,
    BL_GO = 0x21,
    BL_WRITE_MEMORY = 0x31,
    BL_ERASE_MEMORY = 0x43,
    BL_EXTENDED_ERASE_MEMORY = 0x44,
    BL_WRITE_PROTECT = 0x63,
    BL_WRITE_UNPROTECT = 0x73,
    BL_READOUT_PROTECT = 0x82,
    BL_READOUT_UNPROTECT = 0x92
} BootLoaderCommand;
#define XOR(xxx) (xxx ^ 0xFF)



Erc BootLoader::setAltSerial(bool flag)
{
    if (_alt_serial == flag)
        return E_OK;
    _alt_serial = flag;
    Serial.swap();
    return E_OK;
}
/*
 Erc BootLoader::setBoot0(bool flag) {
 if (_boot0 == flag)
 return E_OK;
 digitalWrite(PIN_BOOT0, flag);	// 1 : bootloader mode, 0: flash run
 _boot0 = flag;
 return E_OK;
 }*/

void BootLoader::init()
{
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, 1);
    pinMode(PIN_BOOT0, OUTPUT);
    boot0Flash();
    _mode = M_SYSTEM;
//	setAltSerial(true);
}

void BootLoader::setup()
{
    eb.onDst(H("bootloader")).subscribe(this);
}

void BootLoader::onEvent(Cbor& msg)
{


    Serial.begin(_baudrate, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);
    Serial.swap();

    Cbor& reply = eb.reply();

    uint32_t startTime = millis();
    Erc erc = 0;
    if ( eb.isRequest(H("resetBootloader"))) {

        erc =  resetSystem();

    }  else if ( eb.isRequest(H("resetFlash"))) {

        erc = resetFlash();

    } else if ( eb.isRequest(H("goFlash")) ) {

        uint32_t address;
        if ( msg.getKeyValue(H("address"),address) ) {
            erc = go(address);
            reply.addKeyValue(H("address"), address);
        }

    } else if ( eb.isRequest(H("getId")) ) {

        uint16_t chipId;
        erc = getId(chipId);
        if (erc == E_OK) {
            reply.addKeyValue(H("chip_id"), chipId) ;
        }

    } else if ( eb.isRequest(H("getVersion")) ) {

        uint8_t version;
        erc = getVersion(version);
        if (erc == E_OK) {
            reply.addKeyValue(H("version"), version);
        }

    } else if ( eb.isRequest(H("get")) ) {

        Bytes data(30);
        uint8_t version;
        erc = get(version, data);
        if (erc == E_OK) {
            reply.addKeyValue(H("$cmds"),data);
            reply.addKeyValue(H("version"), version);
        }

    } else if ( eb.isRequest(H("writeMemory")) ) {

        Bytes data(256);
        uint32_t address;
        if ( msg.getKeyValue(H("$data"),data) && msg.getKeyValue(H("address"),address)  ) {
            erc = writeMemory(address, data);
            reply.addKeyValue(H("address"),address).addKeyValue(H("length"),data.length());
        } else {
            erc=EINVAL;
        }

    } else if ( eb.isRequest(H("eraseMemory")) ) {

        Bytes pages(256);
        if ( msg.getKeyValue(H("$pages"),pages)  ) {
            erc = eraseMemory(pages);
            reply.addKeyValue(H("length"),pages.length());
        }

    } else if ( eb.isRequest(H("extendedEraseMemory")) ) {

        erc = extendedEraseMemory();

    } else if ( eb.isRequest(H("eraseAll")) ) {

        erc = eraseAll();

    } else if ( eb.isRequest(H("readMemory")) ) {

        Bytes data(256);
        uint32_t address ;
        uint32_t length ;
        if ( msg.getKeyValue(H("length"),length) && msg.getKeyValue(H("address"),address)  ) {

            reply.addKeyValue(H("address"), address);
            reply.addKeyValue(H("length"),length);
            erc = readMemory(address, length, data);
            if (erc == E_OK) {
                reply.addKeyValue(H("data"),data);
            }
        } else {
            erc=EINVAL;
        }
    } else if ( eb.isRequest(H("writeProtect")) ) {

        Bytes data(256);
        if ( msg.getKeyValue(H("$data"),data))
            erc = writeProtect(data);
        else {
            erc=EINVAL;
        }
    } else if ( eb.isRequest(H("writeUnprotect")) ) {

        erc = writeUnprotect();

    } else if ( eb.isRequest(H("readoutProtect")) ) {

        erc = readoutProtect();

    } else if ( eb.isRequest(H("readoutUnprotect"))) {

        erc = readoutUnprotect();

    } else if ( eb.isRequest(H("set")) ) {
        uint32_t baudrate;
        if (msg.getKeyValue(H("baudrate"),baudrate) ) {
            _baudrate = baudrate;
            reply.addKeyValue(H("baudrate"), _baudrate);
            Config.set("uart.baudrate", _baudrate);
        } else {
            erc = EINVAL;
        }
        String config;
//        Config.load(config);
        reply.addKeyValue(H("config"), config);

    } else {
        eb.defaultHandler(this,msg);
    }
    reply.addKeyValue(H("delta"), Sys::millis() - startTime);
    reply.addKeyValue(H("error"),erc);

    Serial.begin(_baudrate, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_FULL);
    Serial.swap();
}

Erc BootLoader::boot0Flash()
{
    digitalWrite(PIN_BOOT0, 0);
    return E_OK;
}

Erc BootLoader::boot0System()
{
    digitalWrite(PIN_BOOT0, 1);
    return E_OK;
}

Str hstr(1024);
void logBytes(const char* location, Bytes& bytes)
{
    hstr.clear();
    bytes.toHex(hstr);
    LOGF(" %s : %s", location, hstr.c_str());
}
Bytes in(300);

Erc BootLoader::waitAck(Bytes& out, Bytes& in, uint32_t count, uint32_t time)
{
    Serial.write(out.data(), out.length());
    timeout(time);
    while (true) {
        if (timeout()) {
            logBytes("TIMEOUT", in);
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            byte b;
            while (Serial.available()) {
                in.write(b = Serial.read());
            }
            if (b == ACK)
                break;
        }
    }
//	logBytes("ACK", in);
    return E_OK;
}

Erc BootLoader::readVar(Bytes& in, uint32_t max, uint32_t time)
{
    uint32_t count;
    timeout(time);
    while (true) {
        if (timeout()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            count = Serial.read() + 1;
//			in.write(count - 1);
            break;
        }
    }
    if (count > max)
        return EINVAL;
    while (count) {
        if (timeout()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            in.write(Serial.read());
            count--;
        }
    }
    return E_OK;
}

Erc BootLoader::read(Bytes& in, uint32_t count, uint32_t time)
{
    timeout(time);
    while (count) {
        if (timeout()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            in.write(Serial.read());
            count--;
        }
    }
    return E_OK;
}

void flush()
{
    while (Serial.available()) {
        Serial.read();
    };
    in.clear();
}

byte xorBytes(byte* data, uint32_t count)
{
    byte x = data[0];
    int i = 1;
    while (i < count) {
        x = x ^ data[i];
        i++;
    }
    return x;
}

byte slice(uint32_t word, int offset)
{
    return (byte) ((word >> (offset * 8)) & 0xFF);
}

Erc BootLoader::resetFlash()
{
    boot0Flash();
    digitalWrite(PIN_RESET, 0);
    delay(10);
    digitalWrite(PIN_RESET, 1);
    delay(10);
    _mode = M_FLASH;
    return E_OK;
}

Erc BootLoader::resetSystem()
{
    boot0System();
    digitalWrite(PIN_RESET, 0);
    delay(10);
    digitalWrite(PIN_RESET, 1);
    delay(10);
    Serial.write(0x7F);	// send sync for bootloader
    _mode = M_SYSTEM;
    return E_OK;
}

Erc BootLoader::getId(uint16_t& id)
{
    byte GET_ID[] = { BL_GET_ID, XOR(BL_GET_ID) };
    Bytes out(GET_ID, 2);
    flush();
    Erc erc = E_OK;
    if ((erc = waitAck(out, in, 1, DELAY)) == E_OK) {
        in.clear();
        if ((erc = readVar(in, 4, DELAY)) == E_OK) {
            id = in.peek(0) * 256 + in.peek(1);
        }
    }
    return erc;
}

Erc BootLoader::get(uint8_t& version, Bytes& cmds)
{
    byte GET[] = { BL_GET, XOR(BL_GET) };
    Bytes buffer(30);
    Bytes out(GET, 2);
    flush();
    Erc erc = E_OK;
    if ((erc = waitAck(out, cmds, 1, DELAY)) == E_OK) {
        cmds.clear();
        if ((erc = readVar(buffer, 30, DELAY)) == E_OK) {
            buffer.offset(0);
            version = buffer.read();
            while (buffer.hasData())
                cmds.write(buffer.read());
        }
    }
    return erc;
}

Erc BootLoader::getVersion(uint8_t& version)
{
    byte GET_VERSION[] = { BL_GET_VERSION, XOR(BL_GET_VERSION) };
    Bytes out(GET_VERSION, 2);
    Bytes in(4);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    if ((erc = waitAck(out, in, 1, DELAY)) == E_OK) {
        in.clear();
        if ((erc = waitAck(noData, in, 4, DELAY)) == E_OK) {
            version = in.peek(0);
        }
    }
    return erc;
}

Erc BootLoader::writeMemory(uint32_t address, Bytes& data)
{
    byte GET[] = { BL_WRITE_MEMORY, XOR(BL_WRITE_MEMORY) };
    byte ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address, 1),
                       slice(address, 0), xorBytes(ADDRESS, 4)
                     };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    if ((erc = waitAck(out.map(GET, 2), in, 1, DELAY)) == E_OK) {
        if ((erc = waitAck(out.map(ADDRESS, 5), in, 1, DELAY)) == E_OK) {
            Serial.write(data.length() - 1);
            Serial.write(data.data(), data.length());
            Serial.write(
                ((byte) (data.length() - 1))
                ^ xorBytes(data.data(), data.length()));
            if ((erc = waitAck(noData, in, 10, 200)) == E_OK) {

            }
        }
    }
    return erc;
}

Erc BootLoader::readMemory(uint32_t address, uint32_t length, Bytes& data)
{
    byte READ_MEMORY[] = { BL_READ_MEMORY, XOR(BL_READ_MEMORY) };
    byte ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address, 1),
                       slice(address, 0), xorBytes(ADDRESS, 4)
                     };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(READ_MEMORY, 2), in, 1, DELAY);
    if (erc)
        return erc;
    ADDRESS[4] = xorBytes(ADDRESS, 4);
    erc = waitAck(out.map(ADDRESS, 5), in, 1, DELAY);
    if (erc)
        return erc;
    Serial.write(length - 1);
    Serial.write(XOR(length - 1));
    erc = waitAck(noData, in, 1, DELAY);
    if (erc)
        return erc;
    if ((erc = read(data, length, 200)) == E_OK) {
    }
    return erc;
}

Erc BootLoader::go(uint32_t address)
{
    byte GO[] = { BL_GO, XOR(BL_GO) };
    byte ADDRESS[] = { slice(address, 3), slice(address, 2), slice(address, 1),
                       slice(address, 0), xorBytes(ADDRESS, 4)
                     };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(GO, 2), in, 1, DELAY);
    if (erc)
        return erc;
    ADDRESS[4] = xorBytes(ADDRESS, 4);
    erc = waitAck(out.map(ADDRESS, 5), in, 1, DELAY);
    return erc;
}

Erc BootLoader::eraseMemory(Bytes& pages)
{
    byte ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };

    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(ERASE_MEMORY, 2), in, 1, DELAY);
    if (erc)
        return erc;
    Serial.write(pages.length() - 1);
    Serial.write(pages.data(), pages.length());
    Serial.write(
        ((byte) (pages.length() - 1))
        ^ xorBytes(pages.data(), pages.length()));
    erc = waitAck(noData, in, 10, 200);
    return erc;
}

Erc BootLoader::eraseAll()
{
    byte ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };
    byte ALL_PAGES[] = { 0xFF, 0x00 };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(ERASE_MEMORY, 2), in, 1, DELAY);
    if (erc)
        return erc;
    erc = waitAck(out.map(ALL_PAGES, 2), in, 1, 200);
    return erc;
}

Erc BootLoader::extendedEraseMemory()
{
    byte EXTENDED_ERASE_MEMORY[] = { BL_EXTENDED_ERASE_MEMORY, XOR(
                                         BL_EXTENDED_ERASE_MEMORY)
                                   };
    byte ALL_PAGES[] = { 0xFF, 0xFF, 0 };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(EXTENDED_ERASE_MEMORY, 2), in, 1, DELAY);
    if (erc)
        return erc;
    erc = waitAck(out.map(ALL_PAGES, 3), in, 1, 200);
    return erc;
}

Erc BootLoader::writeProtect(Bytes& sectors)
{
    byte WP[] = { BL_WRITE_PROTECT, XOR(BL_WRITE_PROTECT) };

    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(WP, 2), in, 1, DELAY);
    if (erc)
        return erc;
    Serial.write(sectors.length() - 1);
    Serial.write(sectors.data(), sectors.length());
    Serial.write(
        ((byte) (sectors.length() - 1))
        ^ xorBytes(sectors.data(), sectors.length()));
    erc = waitAck(noData, in, 10, 200);
    return erc;
}

Erc BootLoader::writeUnprotect()
{
    byte WUP[] = { BL_WRITE_UNPROTECT, XOR(BL_WRITE_UNPROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(WUP, 2), in, 1, DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData, in, 1, DELAY);
    return erc;
}

Erc BootLoader::readoutProtect()
{
    byte RDP[] = { BL_READOUT_PROTECT, XOR(BL_READOUT_PROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(RDP, 2), in, 1, DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData, in, 1, DELAY);
    return erc;
}

Erc BootLoader::readoutUnprotect()
{
    byte RDUP[] = { BL_READOUT_UNPROTECT, XOR(BL_READOUT_UNPROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(RDUP, 2), in, 1, DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData, in, 1, DELAY);
    return erc;
}

bool BootLoader::timeout()
{
    return _timeout < millis();
}

void BootLoader::timeout(uint32_t delta)
{
    _timeout = millis() + delta;
}




void BootLoader::loop()
{

}
