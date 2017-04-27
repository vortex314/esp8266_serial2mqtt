#include "Mqtt.h"
#include <Cbor.h>
#include <EventBus.h>

#include <unistd.h>
#include <fcntl.h>
//--------------------------------------------------------------------------------------------------------
Mqtt::Mqtt(const char* name,uint32_t maxSize) : Actor(name),
    _host(40), _port(1883), _clientId(30), _user(20), _password(20), _willTopic(
        30), _willMessage(30), _willQos(0), _willRetain(false), _keepAlive(
            20), _cleanSession(1), _msgid(0),_prefix(20),_topic(TOPIC_LENGTH),_message(maxSize)
{
    _lastSrc=0;
}
//--------------------------------------------------------------------------------------------------------
Mqtt::~Mqtt()
{
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::setup()
{
    _host = HOST;
    _clientId = Sys::hostname();
    _user = "";
    _password = "";
    _prefix="limero";
    _willTopic = _prefix;
    _willTopic.append("/system/alive");
    _willMessage = "false";
    _keepAlive=20;


    _wifi_client = new WiFiClient();
    _client =  new PubSubClient(*_wifi_client);
    _client->setServer(_host.c_str(), 1883);
    _client->setCallback(callback);
    state(H("disconnected"));

    eb.onDst(H(name())).call(this);
//	eb.onEvent(H("wifi"),0).call(this);
//	eb.onRequest(H("mqtt"),H("publish")).call(this,(MethodHandler)&Mqtt::publish);
//	eb.onRequest(H("mqtt"),H("subscribe")).call(this,(MethodHandler)&Mqtt::subscribe);
// 	eb.onRequest(H("mqtt"),H("connected")).call(this,(MethodHandler)&Mqtt::isConnected);
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::onEvent(Cbor& msg)
{
    if ( eb.isRequest(H("connect"))) {
        connect(msg);
    } else if ( eb.isRequest(H("disconnect"))) {
        disconnect(msg);
    } else if ( eb.isRequest(H("publish"))) {
        publish(msg);
    } else if ( eb.isRequest(H("subscribe"))) {
        subscribe(msg);
    } else if ( eb.isRequest(H("connected)"))) {
        isConnected(msg);
    } else {
        eb.defaultHandler(this,msg);
    }
}
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
void Mqtt::loop()
{
    _client->loop();
    if ( _client->state() != _client_state) {
        _client_state = _client->state();
        if ( _client_state == MQTT_CONNECTED ) {
            state(H("connected"));
            DEBUG(" state changed : %s ",uid.label(state()));
        } else {
            state(H("disconnected"));
            DEBUG(" state changed : %s ",uid.label(state()));
        }
        eb.event(id(),state());
        eb.send();
    }

}
//--------------------------------------------------------------------------------------------------------
void Mqtt::onActorRegister(Cbor& cbor)
{
}

//--------------------------------------------------------------------------------------------------------
void Mqtt::wakeup()
{
    DEBUG(" wakeup ");
}


//--------------------------------------------------------------------------------------------------------
void Mqtt::loadConfig(Cbor& cbor)
{
    cbor.getKeyValue(H("host"), _host);
    cbor.getKeyValue(H("port"), _port);
    cbor.getKeyValue(H("client_id"), _clientId);
    cbor.getKeyValue(H("user"), _user);
    cbor.getKeyValue(H("password"), _password);
    cbor.getKeyValue(H("keep_alive"), _keepAlive);
    cbor.getKeyValue(H("clean_session"), _cleanSession);
    cbor.getKeyValue(H("will_topic"), _willTopic);
    cbor.getKeyValue(H("will_message"), _willMessage);
    cbor.getKeyValue(H("will_qos"), _willQos);
    cbor.getKeyValue(H("will_retain"), _willRetain);
    cbor.getKeyValue(H("prefix"),_prefix);
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::connect(Cbor& cbor)
{
    loadConfig(cbor);
    cbor.getKeyValue(EB_SRC,_lastSrc);
    if (state()==H("connected")) {
        eb.reply().addKeyValue(EB_ERROR,E_OK);
        eb.send();
        return;
    }
    INFO( " Connecting to (%s : %d) will (%s : %s) with clientId : %s ",_host.c_str(),_port,_willTopic.c_str(),_willMessage.c_str(),_clientId.c_str());
    if ( _client->connect (_clientId.c_str(), _user.c_str(), _password.c_str(), _willTopic.c_str(), _willQos, _willRetain, _willMessage.c_str())) {
        eb.reply().addKeyValue(EB_ERROR,E_OK);
        eb.send();
        state(H("connected"));
        /*        eb.event(id(),H("connected"));
               eb.send();
               INFO(" state changed : %s ",uid.label((uid_t)state()));*/
    } else {
//       eb.publish(id(),H("disconnected"));
        eb.reply().addKeyValue(EB_ERROR,_client->state());
        eb.send();
    }
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::callback(char* topic,byte* message,uint32_t length)
{
    INFO(" message arrived : [%s]",topic);
    Str tpc(topic);
    Str msg(message,length);
    eb.event(H("mqtt"),H("published")).addKeyValue(H("topic"), tpc).addKeyValue(H("message"), msg); // TODO make H("mqtt") dynamic on name
    eb.send();
}
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
void Mqtt::isConnected(Cbor& cbor)
{
    cbor.getKeyValue(EB_SRC,_lastSrc);
    if ( state()==H("connected)") ) {
        eb.reply().addKeyValue(H("error"),0);
        eb.send();
    } else {
        eb.reply().addKeyValue(H("error"),_client->state());
        eb.send();
    }
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::disconnect(Cbor& cbor)
{
    INFO("disconnect()");
//    eb.publish(id(),H("disconnected"));
    _client->disconnect();
    eb.reply().addKeyValue(H("error"), (uint32_t) E_OK);
    eb.send();
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------
void Mqtt::publish(Cbor& cbor)
{
    if (state()!=H("connected")) {
        eb.reply().addKeyValue(H("error_detail"),"not connected").addKeyValue(EB_ERROR, (uint32_t) ENOTCONN);
        eb.send();
        WARN(" not connected ");
        return;
    }
    if ( cbor.getKeyValue(H("topic"), _topic) && cbor.getKeyValue(H("message"), _message)) {
        bool retain=false;
        cbor.getKeyValue(H("retain"), retain);
        _client->loop();    // to avoid timeouts  https://github.com/knolleary/pubsubclient/issues/151
        if ( _client->publish(_topic.c_str(),_message.data(),_message.length(),retain)) {
            _client->loop();    // to avoid timeouts  https://github.com/knolleary/pubsubclient/issues/151
            eb.reply().addKeyValue(EB_ERROR, (uint32_t) E_OK);
            eb.send();
        } else {
            eb.reply().addKeyValue(EB_ERROR, _client->state());
            eb.send();
            /*           Cbor msg(0);
                       disconnect(msg);
                       eb.event(id(),H("disconnected"));
                       eb.send(); */
        }
    } else {
        eb.reply().addKeyValue(H("error"), _client->state());
        eb.send();
    }
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------
void Mqtt::subscribe(Cbor& cbor)
{
    cbor.getKeyValue(EB_SRC,_lastSrc);
    if (state() != H("connected")) {
        eb.reply().addKeyValue(H("error"), ENOTCONN);
        eb.send();
        return;
    }
    Str topic(TOPIC_LENGTH);
    int qos = 0;
    if ( cbor.getKeyValue(H("topic"), topic)  ) {
        cbor.getKeyValue(H("qos"), qos);
        DEBUG(" topic : %s qos : %d ",topic.c_str(),qos);
        if ( _client->subscribe(topic.c_str(),qos) ) {
            eb.reply().addKeyValue(H("error"), E_OK);
            eb.send();
        } else {

            eb.reply().addKeyValue(H("error"), EFAULT);
            eb.send();
            WARN("NOK OK : EFAULT");
        }
    } else {
        eb.reply().addKeyValue(H("error"), EINVAL);
        eb.send();
        WARN("NOK OK : EINVAL");
    }
    _client->loop(); // https://github.com/knolleary/pubsubclient/issues/98
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------
