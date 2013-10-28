#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup()
{
    mosquitto.setup("test.mosquitto.org", 1883);
    mosquitto.connect();
    ofAddListener(mosquitto.onConnect, this, &testApp::onMosquittoConnect);
}

//--------------------------------------------------------------
void testApp::update()
{

}

//--------------------------------------------------------------
void testApp::draw()
{
    ofBackground(0, 0, 0);
    ofSetColor(255);
    ofDrawBitmapString(messageStr, 10, 20);
}

//--------------------------------------------------------------
void testApp::exit()
{
    mosquitto.disconnect();
}

//--------------------------------------------------------------
void testApp::onMosquittoConnect(int &rc)
{
    if (MOSQ_ERR_SUCCESS == rc)
    {
        mosquitto.subscribe(NULL, "#");
        ofAddListener(mosquitto.onMessage, this, &testApp::onMosquittoMessageReceived);
    }
}

//--------------------------------------------------------------
void testApp::onMosquittoMessageReceived(ofxMosquittoMessage &msg)
{
    if (msg.payloadlen)
    {
        messageStr = msg.payloadAsString();
    }
}