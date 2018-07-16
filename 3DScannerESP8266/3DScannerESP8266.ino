#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include "Adafruit_VL53L0X.h"
Adafruit_VL53L0X tof = Adafruit_VL53L0X();

//fancy include of the html page
const char *renderer = 
#include "Renderer.h"

#include <Servo.h>

ESP8266WebServer server(80);

Servo yawServo;
Servo pitchServo;
/************** configuration *************/
const int pinTrigger = 12; //D6
const int pinEcho = 13; //D7
const int pinServoYaw = 16; //D0
const int pinServoPitch = 14; //D5

const bool useTimeOfFlight = true;  //false: ultrasonic, true: tof

//angle step per measurement. 1 best resolution (slowest), >1 lower res
const int stepYaw = 1;
const int stepPitch = 1;

// limit scan range
const int yawRange[] = {0, 180};
const int pitchRange[] = {0, 181};

// limit scan distance
const float validRange[] = {0.05, 5.0};

// add offset to position calculation
const float yawOffset = 0;
const float pitchOffset = 0;

// delays for scans
static const int fixedDelay = 20;
static const int delayPerStep = 10;

/*********************************************************/

//read ultrasonic distance
float readDistanceUltrasound()
{
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);
  return pulseIn(pinEcho, HIGH, 26000) * 0.000345f * 0.5f; 
}

//read fime of flight distance
float readDistanceTimeOfFlight()
{
  VL53L0X_RangingMeasurementData_t measure; 
  tof.rangingTest(&measure, false);

  if (measure.RangeStatus != 4)
    return measure.RangeMilliMeter * 0.001f;
  else
    return 0;  
}

//perform a scan and write points to vertices.js as an array that can be used by js and webgl
void scan()
{  
  yawServo.attach(pinServoYaw);
  pitchServo.attach(pinServoPitch);
  yawServo.write(0);
  pitchServo.write(0);
  delay(1000);
  File f = SPIFFS.open("/distance.js", "w");
  if (!f) 
  {
    Serial.println("file open failed");
    return;
  }
  f.print("var vertices = new Float32Array([");
  int pitch = pitchRange[0];
  for(int yaw = yawRange[0]; yaw < yawRange[1]; yaw += stepYaw)
  {
    yawServo.write(yaw);
    delay(50);
    bool back = pitch != pitchRange[0];
    for(; (back || pitch < pitchRange[1]) && (!back || pitch >= pitchRange[0]); pitch += back ? -stepPitch : stepPitch)
    {
      pitchServo.write(pitch);
      float d;
      if(useTimeOfFlight)
      {
        delay(fixedDelay + delayPerStep  * stepPitch);
        d = readDistanceTimeOfFlight();
        if(d < validRange[0] || d > validRange[1]) d = 0;
      }
      else
      {
        delay(fixedDelay + delayPerStep * stepPitch);
        float avg = 0;
        int svgc = 0;
        for(int i = 0; i < 3; i++)
        {
          float d = readDistanceUltrasound();
          if(d >= validRange[0] && d <= validRange[1])
          {
            avg +=d;
            svgc++;
          }
        }
        d = avg / max(1, svgc);
      }
      if(d > 0)
      {
        float yawf = (yaw + yawOffset) * M_PI / 180;
        float pitchf = (pitch + pitchOffset) * M_PI / 180;
        float x = -sin(yawf) * d * cos(pitchf);
        float y = cos(yawf) * d * cos(pitchf);
        float z = d * sin(pitchf);
        String vertex = String(x, 3) + ", " + String(y, 3) + ", " + String(z, 3);
        Serial.println(vertex);
        f.print(vertex + ", ");
      }
      else
        Serial.println("far");

    }
    pitch -= back ? -stepPitch : stepPitch;
  }
  f.print("]);");
  f.close();
  yawServo.detach();
  pitchServo.detach();
}

//serve main page
void returnRenderer()
{
  server.send (200, "text/html", renderer);
}

//serve stored vertices
void returnVertices()
{
  File file = SPIFFS.open("/distance.js", "r");
  server.streamFile(file, "script/js");
  file.close();
}

//setup. executed first
void setup() 
{
  SPIFFS.begin();

//use format if you have problems with storing the file
//  SPIFFS.format();

//initialize sensors
  if(useTimeOfFlight)
  {
    tof.begin();
  }
  else
  {
    pinMode(pinTrigger, OUTPUT);
  }

  //turn off wifi to get precise measurements
  WiFi.mode(WIFI_OFF);   
  Serial.begin(115200);
  delay(3000);

  //perform a scan
  scan();

////use this if you like to let the esp to use existing wifi
//  WiFi.mode ( WIFI_STA );
//  WiFi.begin ( "ssid", "password" );
//  while ( WiFi.status() != WL_CONNECTED ) {
//    delay ( 500 );
//    Serial.print ( "." );
//  }
//  Serial.print ( "IP address: " );
//  Serial.println ( WiFi.localIP() );

//open access point instead, page is served at 192.168.4.1
  Serial.println(WiFi.softAP("Scanner") ? "Ready" : "Failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());

//let server serve the pages
  server.on("/", returnRenderer);
  server.on("/vertices.js", returnVertices);
  server.begin();  
}

void loop() 
{
  //handle the server
  server.handleClient();
}
