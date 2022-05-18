#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This ain't a ESP8266 or ESP32, dumbo!"
#endif
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <CREDENTIALS.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN       D8

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 36 // Make this large enough to make the maximum number of LEDs work!
#define STATUS_LED_WIFI  3
#define STATUS_LED_PRSR  6
#define STATUS_LED_MQTT  9
#define STATUS_LED_BRI  32

#define LED_BRI_MIN      5
#define LED_BRI_MAX    255

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels
#define MSG_BUFFER_SIZE	(2048)

/*
{ "LEDS" : [
  [255,255,255],
  [255,255,255],
  [255,255,255],
  ...
  [255,255,255]
]
}
*/

void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid        = SSID;
const char* password    = PSWD;
const char* mqtt_server = MQTT_SRV;

struct ui_status_t {
  int wifi;
  int mqtt;
  int parser;
};
typedef struct ui_status_t ui_status;

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callback, espClient);
//DynamicJsonDocument doc(MSG_BUFFER_SIZE);
StaticJsonDocument<MSG_BUFFER_SIZE> doc;
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
ui_status status;

void showStatus() {
  pixels.clear(); 
  if (status.mqtt == 1) 
    pixels.setPixelColor(STATUS_LED_MQTT, pixels.Color(0, STATUS_LED_BRI, 0));
  else 
    pixels.setPixelColor(STATUS_LED_MQTT, pixels.Color(STATUS_LED_BRI, 0, 0));

  if (status.wifi == 1) 
    pixels.setPixelColor(STATUS_LED_WIFI, pixels.Color(0, STATUS_LED_BRI, 0));
  else 
    pixels.setPixelColor(STATUS_LED_WIFI, pixels.Color(STATUS_LED_BRI, 0, 0));

  if (status.parser == 1) 
    pixels.setPixelColor(STATUS_LED_PRSR, pixels.Color(0, STATUS_LED_BRI, 0));
  else 
    pixels.setPixelColor(STATUS_LED_PRSR, pixels.Color(STATUS_LED_BRI, 0, 0));
  pixels.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message Received");
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    status.parser = 0;
    showStatus();
    return;
  }
  status.parser = 1;

  serializeJson(doc, Serial);
  pixels.clear();
  for (int i = 0; i < NUMPIXELS; i++) {
    int R = doc["LEDS"][i][0];
    int G = doc["LEDS"][i][1];
    int B = doc["LEDS"][i][2];
    Serial.printf("%d : %03d: %03d: %03d\n",i,R,G,B);
    pixels.setPixelColor(i, pixels.Color(R,G,B));
  }
  pixels.show();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    char myId[16] = "004";
    //sprintf(myId, "uniGauge-%04X", random(0xffff));

    Serial.printf("Attempting MQTT connection as %s...\n",myId);
    // Attempt to connect
    if (client.connect(myId)) {

      char subBase[16] = "/unigauge/down/";
      char subTopic[32];

      strcpy(subTopic, subBase);
      strcat(subTopic, myId);
      Serial.printf("Subscribe to %s\n",subTopic);

      char pubBase[16] = "/unigauge/up/";
      char pubTopic[32];

      strcpy(pubTopic, pubBase);
      strcat(pubTopic, myId);
      Serial.printf("Publish to %s\n",pubTopic);

      client.subscribe(subTopic);

      char msg[11];
      sprintf(msg,"{\"leds\":%02d}",NUMPIXELS);
      client.publish(pubTopic, msg);
      
      Serial.println("connected");
      status.mqtt = 1;
      showStatus();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      status.mqtt = 0;
      showStatus();
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  Serial.begin(115200);
  Serial.print("Connecting");
  WiFi.begin(ssid, password);

  status.wifi   = 0;
  status.mqtt   = 0;
  status.parser = 1;
  showStatus();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  status.wifi = 1;
  showStatus();

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

/*
{
    "Apparent_output":620,
    "BackupBuffer":"0",
    "BatteryCharging":true,
    "BatteryDischarging":false,
    "Consumption_Avg":640,
    "Consumption_W":652,
    "Fac":49.9640007019043,
    "FlowConsumptionBattery":false,
    "FlowConsumptionGrid":false,
    "FlowConsumptionProduction":true,
    "FlowGridBattery":true,
    "FlowProductionBattery":true,
    "FlowProductionGrid":false,
    "GridFeedIn_W":-51,
    "IsSystemInstalled":1,
    "OperatingMode":"2",
    "Pac_total_W":-774,
    "Production_W":1379,
    "RSOC":11,
    "RemainingCapacity_W":510,
    "Sac1":204,
    "Sac2":204,
    "Sac3":212,
    "SystemStatus":"OnGrid",
    "Timestamp":"2021-03-03 10:16:31",
    "USOC":6,
    "Uac":235,
    "Ubat":52}
*/