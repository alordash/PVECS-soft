#ifndef _WEBSERVER_h
#define _WEBSERVER_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include "webPages.h"
#include "FS.h"
#include "LedControl.h"

#define POST_PZEM_DATA "/pzemData"

typedef struct TWiFiNet{
	public:
	char ssid[32] = "";
	char password[32] = "";
};

const int maxWiFiNetworksStored = sizeof(TWiFiNet) * 10;

class WebServer {
  private:
	const int accessAttempts = 100;

  public:
	WebServer();
	void WiFi_init(const char* path);
	void MDNS_Init(void);
	void loop(void);

	const char *baseSSID = "abirvalg";
	const char *basePASSWORD = "github.com";
	const char *mDNSaddress = "ect";

	ESP8266WebServer server;
	WiFiClient client;
};

extern WebServer webServer;

#endif
