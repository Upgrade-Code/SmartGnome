#include "SmartGnome.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>

SmartGnome::SmartGnome() :
ssid_ap("SmartGnome.o"),
hostname("smartgnome"),
server(new WebServer(80))
{
}

SmartGnome::~SmartGnome()
{
}

// WiFi related operations
void SmartGnome::set_ap_station_mode()
{
  WiFi.mode(WIFI_AP_STA);
}

int SmartGnome::connect_station()
{
  Serial.println("CONNECT STATION");
  Serial.print("ssid_sta = ");
  Serial.println(ssid_sta);
  Serial.print("password_sta = ");
  Serial.println(password_sta);
  
  WiFi.begin(ssid_sta, password_sta);
  Serial.print("Connecting.");

  // Timeout after 30 seconds
  int timeout = 0;
  while((WiFi.status() != WL_CONNECTED) && (timeout < 20)) {
    Serial.print(".");
    delay(500);
    timeout++;
  }
  Serial.println();

  // Not connected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Not connected: Timed out!");
    Serial.println(WiFi.status());
    return -1;
  }

  Serial.println("Connected");
  return 1;
}

int SmartGnome::disconnect_station()
{
  WiFi.disconnect();
  Serial.print("Disconnecting.");

  // Timeout after 10 seconds
  int timeout = 0;
  while((WiFi.status() == WL_CONNECTED) && (timeout < 20)) {
    Serial.print(".");
    delay(500);
    timeout++;
  }
  Serial.println();

  // Not disconnected
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Not disconnected: Timed out!");
    return -1;
  }

  Serial.println("Disconnected");
  return 1;
}

int SmartGnome::start_access_point()
{
  WiFi.softAP(ssid_ap);
  Serial.println(WiFi.softAPIP());
  
  return 1;
}

void SmartGnome::start_mdns()
{
  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started");
  }
}

char * SmartGnome::get_ssid_ap()
{
  return ssid_ap;
}

char * SmartGnome::get_ssid_sta()
{
  return ssid_sta;
}

char * SmartGnome::get_mdns_hostname()
{
  return hostname;
}

String SmartGnome::get_ipaddress()
{
  return WiFi.localIP().toString();
}

//Core behavior
void SmartGnome::init_preferences(bool readOnly)
{
  preferences.begin("smartgnome", readOnly);
}

void SmartGnome::end_preferences()
{
  preferences.end();
}

void SmartGnome::set_ssid_station(String ssid)
{
  preferences.putString("ssid", ssid);
}

void SmartGnome::set_password_station(String password)
{
  preferences.putString("password", password);
}

String SmartGnome::get_ssid_station()
{
  return preferences.getString("ssid");
}

String SmartGnome::get_password_station()
{
  return preferences.getString("password");
}

void SmartGnome::configure()
{
  init_preferences();
  
  if(server->args() == 0)
  {
    server->send(200, "text/plain", "Nothing changed");
    return;
  }
  else
  {
    if (server->hasArg("ssid"))
    {
      Serial.print("ssid arg = ");
      Serial.println(server->arg("ssid"));
      set_ssid_station(server->arg("ssid"));
    }
    if (server->hasArg("password"))
    {
      Serial.print("password arg = ");
      Serial.println(server->arg("password"));
      set_password_station(server->arg("password")); 
    } 
  }
  
  end_preferences();

  server->send(200, "text/plain", "Configured");

  // Restart the Gnome
  begin();
}

bool SmartGnome::is_configured()
{
  return (get_ssid_station() != "");
}

void SmartGnome::reset()
{
  init_preferences();
  preferences.clear();
  end_preferences();

  server->send(200, "text/plain", "Preferences reset");
}

void SmartGnome::grab_config()
{
  init_preferences(true);
  ssid_sta = const_cast<char*>(get_ssid_station().c_str());
  password_sta = const_cast<char*>(get_password_station().c_str());
  end_preferences();

  Serial.println("GRAB CONFIG");
  Serial.print("ssid_sta = ");
  Serial.println(preferences.getString("ssid"));
  Serial.println(get_ssid_station());
  Serial.println(get_ssid_station().c_str());
  Serial.println(const_cast<char*>(get_ssid_station().c_str()));
  Serial.print("password_sta = ");
  Serial.println(password_sta);
}

void SmartGnome::begin()
{ 
  grab_config();
  set_ap_station_mode();
  delay(50);
  disconnect_station();
  WiFi.softAPdisconnect(false);
  delay(50);
  connect_station();
  
  delay(50);
  start_access_point();
  start_mdns();
  
  start_webserver();
}

void SmartGnome::handle_root()
{
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>ESP32 Demo</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <h1>Hello from ESP32!</h1>\
                <p>Uptime: %02d:%02d:%02d</p>\
                <img src=\"/test.svg\" />\
              </body>\
            </html>",

           hr, min % 60, sec % 60
          );
  server->send(200, "text/html", temp);
}

void SmartGnome::handle_not_found()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";

  for (uint8_t i = 0; i < server->args(); i++) {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }

  server->send(404, "text/plain", message);
}

void SmartGnome::start_webserver()
{
  server->on("/", std::bind(&SmartGnome::handle_root, this));
  server->on("/configure", std::bind(&SmartGnome::configure, this));
  server->on("/reset", std::bind(&SmartGnome::reset, this));
  server->on("/inline", [this]() {
    this->server->send(200, "text/plain", "this works");
  });
  server->onNotFound(std::bind(&SmartGnome::handle_not_found, this));
  server->begin();
  
  Serial.println("HTTP server started");
}

void SmartGnome::handle_client()
{
  server->handleClient();
}

SmartGnome SmartG;

