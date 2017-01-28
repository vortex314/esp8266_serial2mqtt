#include "UdpServer.h"

UdpServer::~UdpServer(){
    
}

UdpServer::UdpServer(const char* name) :
    Actor(name)
{
    _connected = false;
    _port = 2000;
    _packetIdx=0;
}

void UdpServer::setConfig(uint16_t port)
{
    _port = port;
}

void UdpServer::onWifiConnect(Cbor& cbor)
{
    INFO(" UDP initialized ");
    begin(_port);
}

void UdpServer::onEvent(Cbor& msg){
    
}

void UdpServer::init()
{
    _packetIdx=0;
}

void UdpServer::setup()
{
    eb.onDst(id()).subscribe(this);
    eb.onEvent(H("wifi"),H("connected")).subscribe(this,(MethodHandler)&UdpServer::onWifiConnect);
}

bool UdpServer::isConnected()
{
    return _connected;
}

void UdpServer::send(IPAddress address,uint16_t port,Bytes& data)
{
    beginPacket(address, port);
    write(data.data(),data.length());
    endPacket();
}

void UdpServer::reply(Bytes& bytes)
{
    send(_lastAddress,_lastPort,bytes);
}

void UdpServer::loop()
{
    int length;
    if (length = parsePacket()) {	// received data
        _packetIdx++;
        _connected = true;
        if (length > UDP_MAX_SIZE) {
            WARN(" UDP packet too big");
            return;
        }
        DEBUG(" UDP rxd %s:%d in packet %d", remoteIP().toString().c_str(),
             remotePort(), _packetIdx);
        _lastAddress = remoteIP();
        _lastPort = remotePort();

        DEBUG("  remote IP : %s ", remoteIP().toString().c_str());
        read(_buffer, length); 			// read the packet into the buffer
        Bytes bytes(_buffer,length);
        eb.event(id(),H("rxd")).addKeyValue(H("data"),bytes);
        eb.send();

    }
}
