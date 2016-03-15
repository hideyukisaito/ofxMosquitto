#include "ofxMosquitto.h"

using namespace mosqpp;

ofxMosquitto::ofxMosquitto() : mosquittopp()
{
    lib_init();
    bConnected = false;
    bAutoReconnect = true;
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

ofxMosquitto::ofxMosquitto(const string clientID, const string host, const int port, const bool cleanSession) : mosquittopp(clientID.c_str(), cleanSession)
{
    lib_init();
    this->clientID = clientID;
    this->host = host;
    this->port = port;
    this->keepAlive = 60;
}

ofxMosquitto::~ofxMosquitto()
{
    lock();
    if (isThreadRunning())
    {
        stopThread();
    }
    
    lib_cleanup();
    unlock();
}

void ofxMosquitto::reinitialise(const string clientID, const bool cleanSession)
{
    lock();
    this->clientID = clientID;
    check_error(mosquittopp::reinitialise(clientID.c_str(), cleanSession));
    unlock();
}

void ofxMosquitto::setup(const string host, const int port, const int keepAlive)
{
    lock();
    this->host = host;
    this->port = port;
    this->keepAlive = keepAlive;
    unlock();
}

void ofxMosquitto::connect()
{
    check_error(mosquittopp::connect(host.c_str(), port, keepAlive));
    start();
}

void ofxMosquitto::connect(const string bindAddress)
{
    check_error(mosquittopp::connect(host.c_str(), port, keepAlive, bindAddress.c_str()));
    start();
}

void ofxMosquitto::reconnect()
{
    check_error(mosquittopp::reconnect());
}

void ofxMosquitto::disconnect()
{
    stop();
    check_error(mosquittopp::disconnect());
}

void ofxMosquitto::publish(int mid, const string topic, const string payload, const int qos, const bool retain)
{
    check_error(mosquittopp::publish(&mid, topic.c_str(), payload.size(), payload.c_str(), qos, retain));
}

void ofxMosquitto::subscribe(int mid, const string sub, const int qos)
{
    check_error(mosquittopp::subscribe(&mid, sub.c_str(), qos));
}

void ofxMosquitto::unsubscribe(int mid, const string sub)
{
    check_error(mosquittopp::unsubscribe(&mid, sub.c_str()));
}

void ofxMosquitto::start()
{
    lock();
    startThread(true);
    unlock();
}

void ofxMosquitto::stop()
{
    lock();
    stopThread();
    unlock();
}

void ofxMosquitto::setUsernameAndPassword(const string username, const string password)
{
    lock();
    this->username = username;
    this->password = password;
    check_error(username_pw_set(username.c_str(), password.c_str()));
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

void ofxMosquitto::setTls(const string cafile, const string capath, const string certfile, const string keyfile, const string keyfilePath)
{
    int ret;
    if (!keyfile.empty() && !keyfilePath.empty())
    {
        ofxMosquitto::keyfilePath = keyfilePath;
        check_error(tls_set(cafile.c_str(), capath.c_str(), certfile.c_str(), keyfile.c_str(), *pw_callback));
    }
    else
    {
        check_error(tls_set(cafile.c_str(), capath.c_str(), certfile.c_str()));
    }
}

void ofxMosquitto::setTlsOptions(int verifyMode, string version, string ciphers)
{
    check_error(tls_opts_set(verifyMode, version.c_str(), ciphers.c_str()));
}

void ofxMosquitto::setTlsInsecure(bool insecure)
{
    check_error(tls_insecure_set(insecure));
}

void ofxMosquitto::setPSK(string psk, string identity, string ciphers)
{
    check_error(tls_psk_set(psk.c_str(), identity.c_str(), ciphers.c_str()));
}

void ofxMosquitto::threadedFunction()
{
    while (isThreadRunning())
    {
        if (lock())
        {
            int rc = loop();
            if (MOSQ_ERR_SUCCESS != rc && bAutoReconnect)
            {
                ofLogError("ofxMosquitto") << mosqpp::strerror(rc);
                reconnect();
                ofSleepMillis(20);
            }
            unlock();
        }
    }
}

void ofxMosquitto::on_connect(int rc)
{
    if (MOSQ_ERR_SUCCESS == rc)
    {
        bConnected = true;
    } else ofLogError("ofxMosquitto") << mosqpp::strerror(rc);
    
    ofNotifyEvent(onConnect, rc, this);
}

void ofxMosquitto::on_disconnect(int rc)
{
    if (MOSQ_ERR_SUCCESS == rc)
    {
        bConnected = false;
    } else ofLogError("ofxMosquitto") << mosqpp::strerror(rc);
    
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
    ofNotifyEvent(onSubscribe, mid, this);
}

void ofxMosquitto::on_unsubscribe(int mid)
{
    ofNotifyEvent(onUnsubscribe, mid, this);
}

void ofxMosquitto::on_log(int level, const char *str)
{
    ofLogVerbose("ofxMosquitto") << "on_log : level = " << level << ", str = " << ofToString(str);
}

void ofxMosquitto::on_error()
{
    ofLogError("ofxMosquitto") << "error";
}

void ofxMosquitto::check_error(int ret)
{
    if (0 < ret) ofLogError("ofxMosquitto") << mosqpp::strerror(ret);
}