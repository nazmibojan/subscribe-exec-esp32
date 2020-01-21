/*
 * @brief: request relay status to arduino 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define RXD2    16
#define TXD2    17
#define PERIOD  5000

const char* ssid      = "CONNEXT-AXIATA";
const char* password  = "4xiatadigitallabs18";
const char* ntp_server = "pool.ntp.org";
const char* mqtt_server = "hairdresser.cloudmqtt.com";
const char* mqtt_user = "wcsuepgk";
const char* mqtt_pwd = "UnNCUdSddsQb";
const int mqtt_port = 18541;
String device_id = "esp8266";
String pub_topic = String(device_id + "/relay_status");
String sub_topic = String(device_id + "/relay_control");
String device_serial = "2286179853734245";

const long utcOffsetInSeconds = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
unsigned long epoch_time = 0;
double relay_status = 0;
unsigned long last_request = 0;
String request = "relay on\r\n";
String relay_string = "";
String inputString = "";
bool stringComplete = false;

WiFiClient ESPClient;
PubSubClient client(ESPClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntp_server, utcOffsetInSeconds);

void setup()
{
  Serial.begin(115200);

  setup_wifi();

  // Connect to MQTT Server
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  timeClient.begin();
}

void loop()
{
  if(WiFi.status() != WL_CONNECTED){
    setup_wifi();  
  }

  if (WiFi.status()== WL_CONNECTED && !client.connected()) {
    reconnect();
  }

  if (millis() - last_request > PERIOD) {
    last_request = millis();

//    Serial.write("get_status");
//    Serial.write('\r');
//    Serial.write('\n');
  }
  
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    relay_string += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  
  if (stringComplete) {
    Serial.print("relayState: ");
    Serial.println(relay_string);
    if (relay_string == request) {
      relay_status = 1;
    } else {
      relay_status = 0;
    }

    printLocalTime();
    send_event();

    relay_string = "";
    stringComplete = false;
  }

  client.loop();
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void printLocalTime()
{
  timeClient.update();

  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());

  epoch_time = timeClient.getEpochTime();
}

//receiving a message
void callback(char* topic, byte* payload, unsigned int length) {
  char msg[300] = {0};
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  do_actions(msg);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(device_id.c_str(), mqtt_user, mqtt_pwd)) {
      Serial.println("connected");
      //subscribe to the topic
      client.subscribe(sub_topic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publish_message(const char* message){
  Serial.println(message);
  client.publish(pub_topic.c_str(), message);
  Serial.println("Event published...");  
}

//====================================================================================================================================================================  
void send_event(){

  char msgtosend[1024] = {0};
  char relay[2];
  char epoch_mil[32];
  char deviceSerial[32];
  double relay_status = 1;

  String sepoch = String(epoch_time);
  sepoch.toCharArray(epoch_mil, sizeof(epoch_mil));
  device_serial.toCharArray(deviceSerial, sizeof(deviceSerial));
  dtostrf(relay_status, 1, 0, relay);

  strcat(msgtosend, "{\"eventName\":\"relayStatus\",\"status\":\"none\"");
  strcat(msgtosend, ",\"relay\":");
  strcat(msgtosend, relay);
  strcat(msgtosend, ",\"time\":");
  strcat(msgtosend, epoch_mil);
  strcat(msgtosend, ",\"mac\":\"");
  strcat(msgtosend, deviceSerial);
  strcat(msgtosend, "\"}");
  publish_message(msgtosend);  //send the event to backend
  memset(msgtosend, 0, sizeof msgtosend);
}
//====================================================================================================================================================================
//==================================================================================================================================================================== 
void do_actions(const char* message){
  //Create this function according to your actions. you will receive a message something like this
  /* Eg : {
        "action":"actionOne",
        "param":{
          "ac1Value1":"1110" ,
          "parentMac":"6655591876092787",
          "ac1Value4":"port:03",
          "ac1Value5":"on",
          "mac":"6655591876092787",
          "ac1Value2":"2220",
          "ac1Value3":"567776"
        }
      } */
  Serial.println(message);
}
