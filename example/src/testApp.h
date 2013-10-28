#pragma once

#include "ofMain.h"
#include "ofxMosquitto.h"

class testApp : public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();
    void exit();
    
    ofxMosquitto mosquitto;
    void onMosquittoConnect(int &rc);
    void onMosquittoMessageReceived(ofxMosquittoMessage &msg);
    string messageStr;
		
};
