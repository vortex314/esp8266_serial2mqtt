#include "BootLoader.h"

Bytes noData(0);
Str _msg(100);
void setLog(const char* s)
{
    _msg=s;
}

BootLoader::BootLoader(const char* name):
    Actor(name),_in(300),_cmds(50)
{
    _mode = M_SYSTEM;
    _serialSwapped = false;
    _timeout = 0;
    _boot0 = true;
    _usartRxd = 0;
    _baudrate=115200;
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



Erc BootLoader::serialSwap(bool flag)
{
    if (_serialSwapped == flag)
        return E_OK;
    _serialSwapped = flag;
    if ( _serialSwapped ) {
        Serial.begin(115200*4, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL); // 8E1 for STM32
        _baudrate=115200*4;
        Serial.swap();
    } else {
        Serial.begin(115200, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL); // 8E1 for STM32
        _baudrate=115200;
    }

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
    digitalWrite(PIN_BOOT0, 1);
    setPublic(true);
    _mode = M_FLASH;
}

Bytes rxd(50);
Bytes txd(50);

void BootLoader::loop() // send all serial received data outside a command as log
{
    static uint64_t _lastData=0;
    if ( Serial.available()) {
        if ( rxd.hasSpace(1) ) {
            rxd.write(Serial.read());
            _lastData = Sys::millis();
        };
    }
    if ( rxd.length()>5  || ((Sys::millis() > _lastData+10) || !rxd.hasSpace(10))  ) {
        eb.event(id(),H("log")).addKeyValue(H("$data"),rxd);
        eb.send();
        rxd.clear();
    }
}



void BootLoader::setup()
{
    eb.onDst(H("bootloader")).subscribe(this);
    setPublic(true);
    init();
    timeout(5000);
}



void BootLoader::report()
{
    timeout(5000);
    eb.event(id(),H("state"))
    .addKeyValue(H("reset"),digitalRead(PIN_RESET))
    .addKeyValue(H("boot0"),digitalRead(PIN_BOOT0))
    .addKeyValue(H("mode"),_mode)
    .addKeyValue(H("baudrate"),_baudrate)
    .addKeyValue(H("serialSwapped"),_serialSwapped)
    .addKeyValue(H("$txd"),txd)
    .addKeyValue(H("$rxd"),rxd)
    .addKeyValue(H("log"),_msg);
    eb.send();
}

//-------------------------------------------------------------------------------------------------
//


void BootLoader::onEvent(Cbor& msg)
{
    if ( timeout() )  {
        report();
        return;
    }
    rxd.clear();
    txd.clear();
//    Serial.begin(_baudrate, SerialConfig::SERIAL_8E1, SerialMode::SERIAL_FULL);

    Cbor& reply = eb.reply();
    uint32_t startTime = millis();
    Erc erc = 0;
    serialSwap(true);
    flush();

    if ( eb.isRequest(H("reset"))) {
        uint32_t boot0;
        if ( msg.getKeyValue(H("boot0"),boot0) ) {
            _mode= boot0 ? M_SYSTEM : M_FLASH;
            digitalWrite(PIN_BOOT0, boot0 ? 1 : 0);
            resetPulse();
            Serial.write(0x7F); // sync char for baudrate
            reply.addKeyValue(H("boot0"),boot0 ? 1 : 0);
        };

    }  else if ( eb.isRequest(H("goFlash")) ) {

        uint32_t address;
        if ( msg.getKeyValue(H("address"),address) ) {
            erc = go(address);
            reply.addKeyValue(H("address"), address);
        }

    }  else if ( eb.isRequest(H("get")) ) {

        Bytes data(30);
        uint8_t version;
        uint16_t chipId;
        erc = get(version, data);
        if (erc == E_OK) {
            _cmds=data;
            reply.addKeyValue(H("$cmds"),data);
            reply.addKeyValue(H("version"), version);
            flush(); // drop last ACK
            erc = getId(chipId);
            if ( erc == E_OK ) {
                reply.addKeyValue(H("chipId"), chipId);
                flush(); // drop last ACK
                erc = getVersion(version);
                if (erc != E_OK) {
                    reply.addKeyValue(H("bootloaderVersion"), version);
                }
            }
        }

    } else if ( eb.isRequest(H("writeMemory")) ) {
        setLog("writeMemory");

        Bytes data(256);
        Str ss(10);
        uint32_t address;
        if ( msg.getKeyValue(H("$data"),data) && msg.getKeyValue(H("address"),address)  ) {
            ss.append("len=").append(data.length());
            setLog(ss.c_str());
            erc = writeMemory(address, data);
            reply.addKeyValue(H("address"),address)
            .addKeyValue(H("$data"),data)
            .addKeyValue(H("length"),data.length());
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
        if ( _cmds.seek(BL_ERASE_MEMORY))
            erc = eraseAll();
        else
            erc = extendedEraseMemory();

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
        eb.clear();
        eb.defaultHandler(this,msg);
        return;
    }
    reply.addKeyValue(H("delta"), Sys::millis() - startTime);
    reply.addKeyValue(H("error"),erc);
    reply.addKeyValue(H("$rxd"),rxd);
    reply.addKeyValue(H("$txd"),txd);
    rxd.clear();
    txd.clear();
    eb.send();

//   Serial.begin(_baudrate, SerialConfig::SERIAL_8N1, SerialMode::SERIAL_FULL);
}
//-------------------------------------------------------------------------------------------------

/*
Str hstr(1024);
void logBytes(const char* location, Bytes& bytes)
{
    hstr.clear();
    bytes.toHex(hstr);
    LOGF(" %s : %s", location, hstr.c_str());
}*/
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
Erc BootLoader::read(Bytes& in, uint32_t count, uint32_t time)
{
    wait(time);
    while (count) {
        if (endWait()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            uint8_t b = Serial.read();
            in.write(b);
            rxd.write(b);
            count--;
        }
    }
    return E_OK;
}
//-------------------------------------------------------------------------------------------------
void BootLoader::flush()
{
    while (Serial.available()) {
        rxd.write(Serial.read());
    };
    _in.clear();
}
//-------------------------------------------------------------------------------------------------
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
//-------------------------------------------------------------------------------------------------
byte slice(uint32_t word, int offset)
{
    return (byte) ((word >> (offset * 8)) & 0xFF);
}
//-------------------------------------------------------------------------------------------------

Erc BootLoader::resetPulse()
{
    digitalWrite(PIN_RESET, 0);
    delay(10);
    digitalWrite(PIN_RESET, 1);
    delay(10);
    return E_OK;
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

Erc BootLoader::getId(uint16_t& id)
{
    byte GET_ID[] = { BL_GET_ID, XOR(BL_GET_ID) };
    Bytes out(GET_ID, 2);
    Erc erc = E_OK;
    _in.clear();
    if ((erc = waitAck(out,  DELAY)) == E_OK) {
        _in.clear();
        if ((erc = readVar(_in, 4, DELAY)) == E_OK) {
            id = _in.peek(0) * 256 + _in.peek(1);
        }
    }
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::get(uint8_t& version, Bytes& cmds)
{
    byte GET[] = { BL_GET, XOR(BL_GET) };
    Bytes buffer(30);
    Bytes out(GET, 2);
    Erc erc = E_OK;
    if ((erc = waitAck(out,  DELAY)) == E_OK) {
        _in.clear();
        if ((erc = readVar(buffer, 30, DELAY)) == E_OK) {
            buffer.offset(0);
            version = buffer.read();
            while (buffer.hasData())
                cmds.write(buffer.read());
        }
    }
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::getVersion(uint8_t& version)
{
    byte GET_VERSION[] = { BL_GET_VERSION, XOR(BL_GET_VERSION) };
    Bytes out(GET_VERSION, 2);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    if ((erc = waitAck(out,  DELAY)) == E_OK) {
        if ((erc = readTillAck(_in,  DELAY)) == E_OK) { //TODO readVar fixed
            version = _in.peek(0);
        }
    }
    return erc;
}
//-------------------------------------------------------------------------------------------------
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
    if ((erc = waitAck(out.map(GET, 2),  DELAY)) == E_OK) {
        if ((erc = waitAck(out.map(ADDRESS, 5),  DELAY)) == E_OK) {
            uint8_t len=data.length() - 1;
            Serial.write(len);
            Serial.write(data.data(), data.length());
            Serial.write((len)^ xorBytes(data.data(), data.length()));
            txd.write(len);
            txd.write(data.data(),0, data.length());
            txd.write((len)^ xorBytes(data.data(), data.length()));
            if ((erc = readTillAck( _in, 200)) == E_OK) {

            }
        }
    }
    return erc;
}
//-------------------------------------------------------------------------------------------------
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
    erc = waitAck(out.map(READ_MEMORY, 2),  DELAY);
    if (erc)
        return erc;
    ADDRESS[4] = xorBytes(ADDRESS, 4);
    erc = waitAck(out.map(ADDRESS, 5),  DELAY);
    if (erc)
        return erc;
    Serial.write(length - 1);
    Serial.write(XOR(length - 1));
    erc = waitAck(noData,  DELAY);
    if (erc)
        return erc;
    if ((erc = read(data, length, 200)) == E_OK) {
    }
    return erc;
}
//-------------------------------------------------------------------------------------------------
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
    erc = waitAck(out.map(GO, 2), DELAY);
    if (erc)
        return erc;
    ADDRESS[4] = xorBytes(ADDRESS, 4);
    erc = waitAck(out.map(ADDRESS, 5),  DELAY);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::eraseMemory(Bytes& pages)
{
    byte ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };

    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(ERASE_MEMORY, 2),  DELAY);
    if (erc)
        return erc;
    Serial.write(pages.length() - 1);
    Serial.write(pages.data(), pages.length());
    Serial.write(
        ((byte) (pages.length() - 1))
        ^ xorBytes(pages.data(), pages.length()));
    erc = readTillAck( _in,  200);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::eraseAll()
{
    byte ERASE_MEMORY[] = { BL_ERASE_MEMORY, XOR(BL_ERASE_MEMORY) };
    byte ALL_PAGES[] = { 0xFF, 0x00 };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(ERASE_MEMORY, 2), DELAY);
    if (erc)
        return erc;
    erc = waitAck(out.map(ALL_PAGES, 2),  200);
    return erc;
}
//-------------------------------------------------------------------------------------------------
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
    erc = waitAck(out.map(EXTENDED_ERASE_MEMORY, 2),  DELAY);
    if (erc)
        return erc;
    erc = waitAck(out.map(ALL_PAGES, 3),  200);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::writeProtect(Bytes& sectors)
{
    byte WP[] = { BL_WRITE_PROTECT, XOR(BL_WRITE_PROTECT) };

    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(WP, 2),  DELAY);
    if (erc)
        return erc;
    Serial.write(sectors.length() - 1);
    Serial.write(sectors.data(), sectors.length());
    Serial.write(
        ((byte) (sectors.length() - 1))
        ^ xorBytes(sectors.data(), sectors.length()));
    erc = readTillAck( _in,  200);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::writeUnprotect()
{
    byte WUP[] = { BL_WRITE_UNPROTECT, XOR(BL_WRITE_UNPROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(WUP, 2),  DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData,  DELAY);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::readoutProtect()
{
    byte RDP[] = { BL_READOUT_PROTECT, XOR(BL_READOUT_PROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(RDP, 2),  DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData,  DELAY);
    return erc;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::readoutUnprotect()
{
    byte RDUP[] = { BL_READOUT_UNPROTECT, XOR(BL_READOUT_UNPROTECT) };
    Bytes out(0);
    Bytes noData(0);
    flush();
    Erc erc = E_OK;
    erc = waitAck(out.map(RDUP, 2),  DELAY);
    if (erc)
        return erc;
    erc = waitAck(noData,  DELAY);
    return erc;
}
//-------------------------------------------------------------------------------------------------
bool BootLoader::endWait()
{
    return _timeout < millis();
}
//-------------------------------------------------------------------------------------------------
void BootLoader::wait(uint32_t delta)
{
    _timeout = millis() + delta;
}
//-------------------------------------------------------------------------------------------------


Erc BootLoader::waitAck(Bytes& out,  uint32_t time)
{
    byte b;
    Serial.write(out.data(), out.length());
    txd.append(out);
    wait(time);
    while (true) {
        if (endWait()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            b = Serial.read();
            rxd.write(b);
        }
        if (b == ACK)
            break;
    }
    return E_OK;
}

Erc BootLoader::readTillAck(Bytes& in,  uint32_t time)
{
    byte b;
    wait(time);
    while (true) {
        if (endWait()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            b = Serial.read();
            in.write(b);
            rxd.write(b);
        }
        if (b == ACK)
            break;
    }
    return E_OK;
}
//-------------------------------------------------------------------------------------------------
Erc BootLoader::readVar(Bytes& in, uint32_t max, uint32_t time)
{
    uint32_t count;
    wait(time);
    while (true) {
        if (endWait()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            uint8_t b =Serial.read()  ;
            count = b+ 1;
            rxd.write(b);
//			in.write(count - 1);
            break;
        }
    }
    if (count > max)
        return EINVAL;
    while (count) {
        if (endWait()) {
            return ETIMEDOUT;
        };
        if (Serial.available()) {
            uint8_t b = Serial.read();
            in.write(b);
            rxd.write(b);
            count--;
        }
    }
    Bytes out(0);
    waitAck(out,  10);
    return E_OK;
}
