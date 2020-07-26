#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <config.h>
#include <PubSubClient.h>

String result;
String motion;
WiFiClient espClient;
PubSubClient client(espClient);

int val = 0;
int prevVal = 0;
int arm = 0;
int relay = 0;

const int relayPin = D1;
const int inputPin = D7;
const int ledPin = LED_BUILTIN;

unsigned long lastMeasurement = 0;

int buttonState; 
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// mqtt reconnect
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "emot-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // once connected `resubscribe`
      client.subscribe("emot/control"); // arming
      client.subscribe("emot/manual_control"); // manual-controller
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

// mqtt message
void callback(char* topic, byte* payload, unsigned int length) {
  if (strncmp(topic, "emot/control", length) == 0) {
    if (strncmp((char *)payload, "1", length) == 0) {
      // arm
      arm = 1;
      Serial.println("Arming Relay");
    } else {
      // un-arm
      arm = 0;
      Serial.println("Unarming Relay");
    }    
  } else if (strncmp(topic, "emot/control", length) == 0) {
    if (strncmp((char *)payload, "1", length) == 0) {
      // arm
      relay = 1;
      Serial.println("Manual On");
    } else {
      // un-arm
      relay = 0;
      Serial.println("Unarming Off");
    }
  }
  Serial.print(topic);
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(9600);
  pinMode(inputPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  // init state
  digitalWrite(ledPin, LOW);
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
  client.setCallback(callback);
}

void loop()
{
  // wifi maintain
  if (!client.connected()) { reconnect(); }
  client.loop();

  // PIR motion sensor control relay
  val = digitalRead(inputPin);  
  // if the sensor state is changed, due to noise or sensor trigger
  if (val != lastButtonState) {
    lastDebounceTime = millis();
  }
  // whatever the reading is at, it's been there for longer than the debounce
  // delay, so take it as the actual current state:    
  // this will publish 1/0 to emot/motion_event var
  if ((millis() - lastDebounceTime) > debounceDelay) {
      if (val != buttonState) {
        buttonState = val;
        Serial.printf("%d", val);
        if (buttonState == LOW) {
          Serial.println("Motion end");
          client.publish("emot/motion_event", "0");
        } else {
          Serial.println("Motion start");
          client.publish("emot/motion_event", "1");
        }        
      }
  }

  // relay trigger code - this is manually controlled by `emot/control`
  if (relay == 1) {
      digitalWrite(ledPin, LOW);
      if (arm == 1) {
        digitalWrite(relayPin, HIGH);
      }    
  } else {
      digitalWrite(ledPin, HIGH);
      if (arm == 1) {
        digitalWrite(relayPin, LOW);
      }
  }

  lastButtonState = val;
}