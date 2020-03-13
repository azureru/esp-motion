#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <config.h>
#include <PubSubClient.h>

String result;
String motion;
int val = 0;
int prevVal = 0;

const int relayPin = D1;
const int inputPin = D7;
const int outPin = LED_BUILTIN;

unsigned long lastMeasurement = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// mqtt reconnect
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(inputPin, INPUT);
  pinMode(outPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  // init state
  digitalWrite(outPin, LOW);
  digitalWrite(relayPin, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("emotion");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // mqtt
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  // mqtt
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // PIR motion sensor
  val = digitalRead(inputPin);
  if (val == 0) {
    // val 0 means no motion
    digitalWrite(outPin, LOW);
    digitalWrite(relayPin, LOW);
  } else {
    // val 1 means there's motion
    digitalWrite(outPin, HIGH);
    digitalWrite(relayPin, HIGH);
  }

  // publish motion delta 
  if (val != prevVal) {
    if (val == 1) {
      Serial.println("Motion started!");
      client.publish("emot/motion_event", "1");
    } else {
      Serial.println("Motion end");
      client.publish("emot/motion_event", "0");
    }
    prevVal = val;
  }

  // publish motion state - but throttled to be in 250ms interval
  unsigned long currentMillis = millis();  
  if (currentMillis - lastMeasurement > 250) {   
    lastMeasurement = millis(); 
    char buffer[10];
    sprintf(buffer, "%d", val);
    client.publish("emot/motion_state", buffer);
    Serial.println(buffer);
  }
}