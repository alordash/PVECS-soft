#ifndef _WEBSERVER_h
#define _WEBSERVER_h

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <mDNSResolver.h>
#include "FS.h"

#define POST_PZEM_DATA "/pzemData"

typedef struct TWiFiNet {
  public:
	char ssid[32] = "";
	char password[32] = "";
};

const int maxWiFiNetworksStored = sizeof(TWiFiNet) * 10;

class WebServer {
  private:
	const int accessAttempts = 150;
  public:
	WebServer();
	bool WiFi_init(const char *path, const char *target_hostname, mDNSResolver::Resolver resolver);
	void MDNS_Init(void);
	void loop(void);

	const char *baseSSID = "abirvalg1";
	const char *basePASSWORD = "github.com";
	const char *baseIPstr = "0.0.0.0";
	IPAddress baseIP;

	ESP8266WebServer server;
	IPAddress target_ip;
	WiFiClient client;
};

extern WebServer webServer;

#endif