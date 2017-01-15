#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <EventBus.h>
#include <WiFiUdp.h>

#define UDP_MAX_SIZE 300

class UdpServer : public Actor, public WiFiUDP
{

    uint8_t _buffer[UDP_MAX_SIZE];	//
    bool _connected;
    IPAddress _lastAddress;
    uint16_t _lastPort;
    uint16_t _port;
    uint32_t _packetIdx;
public:
    UdpServer(const char* name);
    ~UdpServer();
    void setConfig(uint16_t port);
    void onWifiConnect(Cbor& cbor) ;
    void setup();
    void init();
    void onEvent(Cbor& cbor);
    bool isConnected() ;
    void loop() ;
    void send(IPAddress addres,uint16_t port,Bytes& data);
    void reply(Bytes& bytes);

};

#endif // UDPSERVER_H
