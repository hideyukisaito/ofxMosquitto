#include "ofxMosquitto.h"

using namespace mosqpp;

ofxMosquitto::ofxMosquitto() : mosquittopp()
{
    lib_init();
    bConnected = false;
    bAutoReconnect = true;
    timestamp = ofGetTimestampString();
}

ofxMosquitto::ofxMosquitto(const ofxMosquitto& mom)
{
    clientID = mom.clientID;
    host = mom.host;
    port = mom.port;
    username = mom.username;
    password = mom.password;
    keepAlive = mom.keepAlive;
}

ofxMosquitto & ofxMosquitto::operator=(const ofxMosquitto &mom)
{
    clientID = mom.clientID;
    host = mom.host;
    port = mom.port;
    username = mom.username;
    password = mom.password;
    keepAlive = mom.keepAlive;
    
    return *this;
}

ofxMosquitto::ofxMosquitto(string clientID, string host, int port, bool cleanSession) : mosquittopp(clientID.c_str(), cleanSession)
{
    this->clientID = clientID;
    this->host = host;
    this->port = port;
    this->keepAlive = 60;
}

ofxMosquitto::~ofxMosquitto()
{
    if (isThreadRunning())
    {
        stopThread();
    }
    
    lib_cleanup();
}

int ofxMosquitto::reinitialise(string clientID, bool cleanSession)
{
    return mosquittopp::reinitialise(clientID.c_str(), cleanSession);
}

void ofxMosquitto::setup(string host, int port, int keepAlive)
{
    this->host = host;
    this->port = port;
    this->keepAlive = keepAlive;
}

int ofxMosquitto::connect()
{
    int ret = mosquittopp::connect(host.c_str(), port, keepAlive);
    start();
    
    return ret;
}

int ofxMosquitto::connect(string bindAddress)
{
    int ret = mosquittopp::connect(host.c_str(), port, keepAlive, bindAddress.c_str());
    start();
    
    return ret;
}

int ofxMosquitto::reconnect()
{
    return mosquittopp::reconnect();
}

int ofxMosquitto::disconnect()
{
    return mosquittopp::disconnect();
}

int ofxMosquitto::publish(string topic, string payload)
{
    return mosquittopp::publish(NULL, topic.c_str(), payload.size(), payload.c_str());
}

int ofxMosquitto::subscribe(int mid, string sub, int qos)
{
    return mosquittopp::subscribe(&mid, sub.c_str(), qos);
}

int ofxMosquitto::unsubscribe(int mid, string sub)
{
    return mosquittopp::unsubscribe(&mid, sub.c_str());
}

void ofxMosquitto::start()
{
    startThread(true, false);
}

void ofxMosquitto::stop()
{
    stopThread();
}

void ofxMosquitto::setUsernameAndPassword(string username, string password)
{
    lock();
    this->username = username;
    this->password = password;
    username_pw_set(username.c_str(), password.c_str());
    unlock();
}

void ofxMosquitto::setKeepAlive(int keepAlive)
{
    lock();
    this->keepAlive = keepAlive;
    unlock();
}

void ofxMosquitto::setAutoReconnect(bool reconnect)
{
    lock();
    this->bAutoReconnect = reconnect;
    unlock();
}

void ofxMosquitto::setUserdata(void *userdata)
{
    lock();
    this->userdata = userdata;
    unlock();
}

void ofxMosquitto::threadedFunction()
{
    while (isThreadRunning())
    {
        if (lock())
        {
            int rc = loop();
            if (0 < rc && bAutoReconnect)
            {
                reconnect();
                ofSleepMillis(20);
            }
        }
        unlock();
    }
}

void ofxMosquitto::on_connect(int rc)
{
    if (MOSQ_ERR_SUCCESS == rc)
    {
        bConnected = true;
    }
    
    ofNotifyEvent(onConnect, rc, this);
}

void ofxMosquitto::on_disconnect(int rc)
{
    if (MOSQ_ERR_SUCCESS == rc)
    {
        bConnected = false;
    }
    
    ofNotifyEvent(onDisconnect, rc, this);
}

void ofxMosquitto::on_message(const struct mosquitto_message *message)
{
    ofxMosquittoMessage msg;
    msg.mid = message->mid;
    msg.topic = message->topic;
    msg.payload = message->payload;
    msg.payloadlen = message->payloadlen;
    msg.qos = message->qos;
    msg.retain = message->retain;
    
    ofNotifyEvent(onMessage, msg, this);
}

void ofxMosquitto::on_publish(int rc)
{
    ofNotifyEvent(onPublish, rc, this);
}

void ofxMosquitto::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
//    ofLogNotice("ofxMosquitto") << "subscribed : mid = " << mid << ", qos = " << qos_count << ", granted_qos = " << granted_qos;
    ofNotifyEvent(onSubscribe, mid, this);
}

void ofxMosquitto::on_unsubscribe(int mid)
{
    ofNotifyEvent(onUnsubscribe, mid, this);
}

void ofxMosquitto::on_log(int level, const char *str)
{
    ofLogNotice("ofxMosquitto") << "on_log : level = " << level << ", str = " << ofToString(str);
}

void ofxMosquitto::on_error()
{
    ofLogNotice("ofxMosquitto") << "error";
}