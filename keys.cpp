#include <EventBus.h>

const char* hash2string(uint32_t hash)
{
    switch(hash)
    {
	case EB_DST :  return "dst";
    case EB_SRC :  return "src";
    case EB_EVENT :  return "event";
    case EB_REQUEST :  return "request";
    case EB_REPLY :  return "reply";
case H("clean_session") : return "clean_session";
case H("client_id") : return "client_id";
case H("connected") : return "connected";
case H("connected)") : return "connected)";
case H("disconnected") : return "disconnected";
case H("error") : return "error";
case H("error_detail") : return "error_detail";
case H("host") : return "host";
case H("keep_alive") : return "keep_alive";
case H("message") : return "message";
case H("mqtt") : return "mqtt";
case H("mqtt_connected") : return "mqtt_connected";
case H("mqtt_disconnected") : return "mqtt_disconnected";
case H("password") : return "password";
case H("port") : return "port";
case H("prefix") : return "prefix";
case H("publish") : return "publish";
case H("published") : return "published";
case H("qos") : return "qos";
case H("retain") : return "retain";
case H("subscribe") : return "subscribe";
case H("timer") : return "timer";
case H("topic") : return "topic";
case H("user") : return "user";
case H("wifi") : return "wifi";
case H("wifi_disconnected") : return "wifi_disconnected";
case H("will_message") : return "will_message";
case H("will_qos") : return "will_qos";
case H("will_retain") : return "will_retain";
case H("will_topic") : return "will_topic";
default : {
		return 0;
        }
    }
}
