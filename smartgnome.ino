/*
  mqtt_esp32.ino - Smart-Gnome code
  Copyright (c) 2018 Nizar AYED - Upgrade-Code.org. All rights reserved.

  This code is not a free software and can not distributed without a hand-written
  permission of the author. It can be licensed only by the author
*/

#include "SmartGnome.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <driver/adc.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Default MQTT server
//const char* mqtt_server = "vps511303.ovh.net";
const char* mqtt_server = "broker.mqtt-dashboard.com";

/* define DHT pins */
#define DHTPIN 14
#define DHTTYPE DHT22
DHTesp dht;
float temperature = 0;

/* Humidity */
float humidity = 0;

/* Hygrometer */
float hygrometry = 0;

/* Luminosity */
float luminosity = 0;

/* topics */
#define TEMP_TOPIC     "Device/502-481-5954/tempAir"
#define HUMID_TOPIC    "Device/502-481-5954/humidity"
#define HYGRO_TOPIC    "Device/502-481-5954/hygrometry"
#define PHOTO_TOPIC    "Device/502-481-5954/luminosity"
#define CMD_TOPIC      "Device/502-481-5954/command" /* 1=on, 0=off */

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
boolean debug = false;
boolean is_configured = false;

/*LED GPIO pin*/
const char led = 13;

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");


  // Switch on the LED if an 1 was received as first character
  String myPayload = "";
  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    myPayload += (char)payload[i];
  }
  Serial.println();

  if(debug) {
    Serial.print("myPayload:");
    Serial.println(myPayload);
  }
  
  if (myPayload == "debug-on") {
    debug = true;
  } else if (myPayload == "debug-off") {
    debug = false;
  } else if (myPayload == "reset") {
    SmartG.reset();
  } else if (myPayload == "hygrometry") {
    hygrometry = map(random(1024), 0, 1024, 100, 0);
    Serial.print("Hygrometry : ");
    Serial.println(hygrometry);
    
    if (!isnan(hygrometry)) {
      snprintf (msg, 20, "%lf", hygrometry);
      /* publish the message */
      client.publish(HYGRO_TOPIC, msg);
    }
    
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED off 
  } else if (myPayload == "luminosity") {
    luminosity = map(random(1024), 0, 1024, 100, 0);
    Serial.print("Luminosity: ");
    Serial.println(luminosity);
    
    if (!isnan(luminosity)) {
      snprintf (msg, 20, "%lf", luminosity);
      /* publish the message */
      client.publish(PHOTO_TOPIC, msg);
    }
  } else {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED on
  }
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  SmartG.begin();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  /*start DHT sensor */
  dht.setup(DHTPIN, DHTesp::DHT22);
  Serial.println("DHT initiated");

  Serial.print("Unique MAC ident: ");
  Serial.print( WiFi.macAddress()[9]);
  Serial.print( WiFi.macAddress()[10]);
  Serial.print( WiFi.macAddress()[12]);
  Serial.print( WiFi.macAddress()[13]);
  Serial.print( WiFi.macAddress()[15]);
  Serial.print( WiFi.macAddress()[16]);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    /* client ID */
    String clientId = "502-481-5954";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      /* subscribe topic with default QoS 0*/
      client.subscribe(CMD_TOPIC);
    } else {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(10000);
    }
  }
}

int readHygrometry()
{
  adc1_config_width(ADC_WIDTH_BIT_10);   //Range 0-1023 
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);  //ADC_ATTEN_DB_11 = 0-3,6V
  return adc1_get_raw( ADC1_CHANNEL_0 ); //Read analog
}

int readLuminosity()
{
  adc1_config_width(ADC_WIDTH_BIT_10);   //Range 0-1023 
  adc1_config_channel_atten(ADC1_CHANNEL_3,ADC_ATTEN_DB_11);  //ADC_ATTEN_DB_11 = 0-3,6V
  return adc1_get_raw( ADC1_CHANNEL_3 ); //Read analog
}

void loop_mqtt()
{
  if (!client.connected())
  {
    reconnect();
  }
  /* this function will listen for incomming 
  subscribed topic-process-invoke receivedCallback */
  client.loop();

  /* we measure temperature every 3 secs
  we count until 3 secs reached to avoid blocking program if using delay()*/
  long now = millis();
  if (now - lastMsg > 180000)
  {
    lastMsg = now;
    /* read DHT11/DHT22 sensor and convert to string */
    temperature = dht.getTemperature();
    Serial.print("Temperature : ");
    Serial.println(temperature);
    
    humidity = dht.getHumidity();
    Serial.print("Humidity : ");
    Serial.println(humidity);

    hygrometry = map(readHygrometry(), 0, 1024, 100, 0);
    Serial.print("Hygrometry : ");
    Serial.println(hygrometry);
    
    luminosity = map(readLuminosity(), 0, 1024, 100, 0);
    Serial.print("Luminosity : ");
    Serial.println(luminosity);
    
    if (!isnan(temperature)) {
      snprintf (msg, 20, "%lf", temperature);
      /* publish the message */
      client.publish(TEMP_TOPIC, msg);
    }
    
    if (!isnan(humidity)) {
      snprintf (msg, 20, "%lf", humidity);
      /* publish the message */
      client.publish(HUMID_TOPIC, msg);
    }
    
    if (!isnan(hygrometry)) {
      snprintf (msg, 20, "%lf", hygrometry);
      /* publish the message */
      client.publish(HYGRO_TOPIC, msg);
    }
    
    if (!isnan(luminosity)) {
      snprintf (msg, 20, "%lf", hygrometry);
      /* publish the message */
      client.publish(PHOTO_TOPIC, msg);
    }
  }
}

void loop()
{
  if(WiFi.status() == WL_CONNECTED) 
  {
    loop_mqtt();
  }
  
  SmartG.handle_client();
}

