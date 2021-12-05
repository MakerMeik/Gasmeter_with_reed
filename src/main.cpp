#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <secrets.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = SECRET_SSID;  // Enter your passwords in the secrets.h file. You can use the secrets.h.template file for this purpose.
const char* password = SECRET_PASS;
const char* mqtt_server = "192.168.188.34";
const unsigned int mqtt_port = 1883;
const char* mqtt_user = "admin";
const char* mqtt_pass = SECRET_MQTT_PASS;

IPAddress ip(192, 168, 188, 103);
IPAddress gateway(192, 168, 188, 34); 
IPAddress subnet(255, 255, 255, 0);

String clientId = "iot-GasReed3";

const unsigned int reed_pin = 3; // GPIO03

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Some of my viewers have pointed out to me that even "double" variables are liable to cause errors over
// time and will lead to mismatches in the long run. It seems that it is common practice to solve such
// problems better with integers or unsigned integers. The decimal places are obtained by dividing the 
// result by 100, as in my case. By the way, I do this on the server side in Node-RED. By the way, you 
// can also find the corresponding Node-RED flow here on Github.

unsigned int metercount = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////

bool status = 0; // 0 = nothing  /  1 = pulse = 0.01 m3
int stat_counter = 0; 


void setup_wifi(){  // Start WiFi-Connection
  WiFi.config(ip, gateway, subnet); // comment out if your router assigns the IP address automatically via DHCP
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic,"gasmeter/status") == 0) {
      String metercount_str;
      for (uint i = 0; i < length; i++) {
        metercount_str += (char)payload[i];
      }
      metercount = metercount_str.toInt();
    }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      client.subscribe("gasmeter/status");

    } else {
      delay(1000);
    }
  }
}

void setup() {
  // Serial.begin(9600);
  // Serial.setTimeout(3000);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  while(!Serial) { }
  // Serial.println("I'm awake.");
  delay(200);
  pinMode(reed_pin,INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  status = digitalRead(reed_pin);   // read the input pin

  while(stat_counter < 10 && status == 1) {
      stat_counter = stat_counter + 1;
      status = 0;
      delay(100);
      status = digitalRead(reed_pin);   // read the input pin
  } 

  if(stat_counter >= 10 && status == 1) {
    while(status == 1) {
      delay(100);
      status = digitalRead(reed_pin);   // read the input pin
      stat_counter = 0;
      while(stat_counter < 10 && status == 0) {
          stat_counter = stat_counter + 1;
          status = 1;
          delay(200);
          status = digitalRead(reed_pin);   // read the input pin
      } 
    }
    metercount = metercount + 1;
    client.publish("gasmeter/pulse", String(metercount).c_str());
    client.subscribe("gasmeter/pulse");
  }

  status = 0;
  stat_counter = 0;
  delay(100);
}
