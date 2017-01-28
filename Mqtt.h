#ifndef MQTT_H
#define MQTT_H


#include <Cbor.h>
#include <EventBus.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define HOST     "limero.ddns.net"
//#define HOST     "test.mosquitto.org"

#define ADDRESS     "tcp://limero.ddns.net:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "stm32/alive"
#define PAYLOAD     "true"
#define QOS         1
#define TIMEOUT     10000L

#define TOPIC_LENGTH 40

class Mqtt : public Actor
{
private:
    PubSubClient* _client;
	WiFiClient* _wifi_client;
	int _client_state;

    Str _host;
    uint16_t _port;
    Str _clientId;
    Str _user;
    Str _password;
    Str _willTopic;
    Str _willMessage;
    int _willQos;
    bool _willRetain;
    uint32_t _keepAlive;
    int _cleanSession;
    uint16_t _msgid;
    uint16_t _lastSrc;
    int _fd[2];   // pipe fd to wakeup in select
    Str _prefix;
	Str _topic;
	Bytes _message;
	static void callback(char* topic,byte* message,uint32_t length);

public:

    Mqtt(const char* name,uint32_t maxSize);
    virtual ~Mqtt();
    void setup();
    void onEvent(Cbor& cbor);
    void onActorRegister(Cbor& cbor);

 //   static void onConnectionLost(void *context, char *cause);
//    static int onMessage(void *context, char *topicName, int topicLen, MQTTAsync_message *message);

    void disconnect(Cbor& cbor);
 //   static void onDisconnect(void* context, MQTTAsync_successData* response);
    void connect(Cbor& cbor);
 //   static void onConnectFailure(void* context, MQTTAsync_failureData* response);
//    static void onConnectSuccess(void* context, MQTTAsync_successData* response);
    void subscribe(Cbor& cbor);
//    static void onSubscribeSuccess(void* context, MQTTAsync_successData* response);
 //   static void onSubscribeFailure(void* context, MQTTAsync_failureData* response);
    void publish(Cbor& cbor);
//    static void onPublishSuccess(void* context, MQTTAsync_successData* response);
//    static void onPublishFailure(void* context, MQTTAsync_failureData* response);
    void loadConfig(Cbor& cbor);
    void isConnected(Cbor& cbor);

    int fd();
    void wakeup();
	void loop();

};


#endif // MQTT_H
