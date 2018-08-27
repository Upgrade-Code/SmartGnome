#ifndef _SMART_GNOME_
#define _SMART_GNOME_

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

class SmartGnome
{
private:
  char * ssid_sta;
  char * password_sta;
  char * ssid_ap;
  char * hostname;
  WebServer* server;

protected:
  Preferences preferences;
  void set_ssid_station(String);
  void set_password_station(String);
  String get_ssid_station();
  String get_password_station();
  void handle_root();
  void handle_not_found();
  void start_webserver();
  
public:
  SmartGnome();
  ~SmartGnome();
  // WiFi related operations
  void set_ap_station_mode();
  int connect_station();
  int disconnect_station();
  int start_access_point();
  void start_mdns();
  char * get_ssid_ap();
  char * get_ssid_sta();
  char * get_mdns_hostname();
  String get_ipaddress();

  // Core behavior
  void init_preferences(bool readOnly=false);
  void end_preferences();
  void configure();
  bool is_configured();
  void grab_config();
  void reset();
  void begin();
  void handle_client();
};

extern SmartGnome SmartG;

#endif /* _SMART_GNOME_ */

