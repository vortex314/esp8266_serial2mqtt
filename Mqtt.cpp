#include "Mqtt.h"
#include <Cbor.h>
#include <EventBus.h>

#include <unistd.h>
#include <fcntl.h>
//--------------------------------------------------------------------------------------------------------
void Mqtt::onEvent(Cbor& msg)
{
	Cbor cbor(0);
	uint32_t error;
	PT_BEGIN()
	;
WIFI_CONNECTING: {
		state(H("wifi_disconn_lastected"));
		PT_YIELD_UNTIL( eb.isEvent(H("wifi"),H("connected")));
		LOGF(" wifi ok ");
	}
MQTT_CONNECTING: {
		while (true) {
			state(H("mqtt_disconnected"));
			timeout(2000);
			PT_YIELD_UNTIL(timeout());
			_willQos=2;
			if ( _client->connect (_clientId.c_str(), _user.c_str(), _password.c_str(), _willTopic.c_str(), _willQos, _willRetain, _willMessage.c_str())) {
				eb.reply().addKeyValue(EB_ERROR,E_OK);
				eb.send();
				goto CONNECTED;
			} else {
				eb.reply().addKeyValue(EB_ERROR,_client->state());
				eb.send();
			}
//			if ( _client->connect(_clientId.c_str()) )goto CONNECTED;
			timeout(2000);
			PT_YIELD_UNTIL(_client->connected() || timeout());
			if ( !timeout()) {
				goto CONNECTED;
			}
			LOGF(" not yet connected ");
		};
	};
CONNECTED: {
		LOGF(" connected ");
		eb.event(H("mqtt"),H("connected"));
		eb.send();
		state(H("connected"));
		while(true) {
			state(H("mqtt_connected"));
			timeout(500000);
			PT_YIELD_UNTIL(eb.isEvent(H("wifi"),H("disconnected") || eb.isEvent(H("mqtt"),H("disconnected"))) || timeout() );
			if ( timeout() ) continue;
			goto WIFI_CONNECTING;
		}
	}

	PT_END()
}
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
void Mqtt::loop(){
	if ( _client->state() != _client_state) {
		_client_state = _client->state();
		if ( _client_state == MQTT_CONNECTED ) {
			eb.event(id(),H("connected"));
			eb.send();
		};
	}
	_client->loop();
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::onActorRegister(Cbor& cbor)
{ 
}
//--------------------------------------------------------------------------------------------------------
Mqtt::Mqtt() : Actor("mqtt"),
	_host(40), _port(1883), _clientId(30), _user(20), _password(20), _willTopic(
	    30), _willMessage(30), _willQos(0), _willRetain(false), _keepAlive(
	        20), _cleanSession(1), _msgid(0),_prefix(20)
{
	_lastSrc=id();
}
//--------------------------------------------------------------------------------------------------------
Mqtt::~Mqtt()
{
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::wakeup()
{
	LOGF(" wakeup ");
}

//--------------------------------------------------------------------------------------------------------
void Mqtt::setup()
{
	_host = HOST;
	_clientId = CLIENTID;
	_user = "";
	_password = "";
	_prefix="limero";
	_willTopic = _prefix;
	_willTopic.append("/system/alive");
	_willMessage = "false";
	_keepAlive=120;

	_wifi_client = new WiFiClient();
	_client =  new PubSubClient(*_wifi_client);
	_client->setServer(_host.c_str(), 1883);
	_client->setCallback(callback);

	timeout(2000);
	eb.onDst(H("mqtt")).subscribe(this);
	eb.onEvent(H("wifi"),0).subscribe(this);
	eb.onRequest(H("mqtt"),H("publish")).subscribe(this,(MethodHandler)&Mqtt::publish);
	eb.onRequest(H("mqtt"),H("subscribe")).subscribe(this,(MethodHandler)&Mqtt::subscribe);
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
	if ( _client->connect (_clientId.c_str(), _user.c_str(), _password.c_str(), _willTopic.c_str(), _willQos, _willRetain, _willMessage.c_str())) {
		eb.reply().addKeyValue(EB_ERROR,E_OK);
		eb.send();
	} else {
		eb.reply().addKeyValue(EB_ERROR,_client->state());
		eb.send();
	}
}
//--------------------------------------------------------------------------------------------------------
void Mqtt::callback(char* topic,byte* message,uint32_t length)
{
	LOGF(" message arrived : [%s]",topic);
	Str tpc(topic);
	Str msg(message,length);
	eb.event(H("mqtt"),H("published")).addKeyValue(H("topic"), tpc).addKeyValue(H("message"), msg);
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
	_client->disconnect();
	eb.reply().addKeyValue(H("error"), (uint32_t) E_OK);
	eb.send();
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------
void Mqtt::publish(Cbor& cbor)
{
	cbor.getKeyValue(EB_SRC,_lastSrc);
	if (state()!=H("mqtt_connected")) {
		eb.reply().addKeyValue(H("error_detail"),"not connected").addKeyValue(H("error"), (uint32_t) ENOTCONN);
		eb.send();
		LOGF(" not connected ");
		return;
	}
	Str topic(60);
	Bytes message(1024);
	bool retain=false;
	if ( cbor.getKeyValue(H("topic"), topic) && cbor.getKeyValue(H("message"), message)) {
		cbor.getKeyValue(H("retain"), retain);
		if ( _client->publish(topic.c_str(),message.data(),message.length(),retain)) {
			eb.reply().addKeyValue(H("error"), (uint32_t) E_OK);
			eb.send();
		} else {
			eb.reply().addKeyValue(H("error"), _client->state());
			eb.send();
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
	if (state() != H("mqtt_connected")) {
		eb.reply().addKeyValue(H("error"), ENOTCONN);
		return;
	}
	Str topic(60);
	int qos = 0;
	if ( cbor.getKeyValue(H("topic"), topic)  ) {
		cbor.getKeyValue(H("qos"), qos);
		LOGF(" topic : %s qos : %d ",topic.c_str(),qos);
		if ( _client->subscribe(topic.c_str(),qos) ) {
			eb.reply().addKeyValue(H("error"), E_OK);
			eb.send();
			LOGF("OK");
		} else {
			eb.reply().addKeyValue(H("error"), EFAULT);
			eb.send();
			LOGF("NOK OK : EFAULT");
		}
	} else {
		eb.reply().addKeyValue(H("error"), EINVAL);
		eb.send();
		LOGF("NOK OK : EINVAL");
	}
	INFO("Subscribing to topic %s for client %s using QoS%d",
	     topic.c_str(), CLIENTID, qos);
}
//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------
