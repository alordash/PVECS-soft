#include "WebServer.h"
#include <ArduinoOTA.h>

WebServer webServer;

WebServer::WebServer() {
}

void WebServer::loop(void) {
	server.handleClient();
}

bool WebServer::WiFi_init(const char *path, const char *target_hostname, mDNSResolver::Resolver resolver) {
	baseIP.fromString(baseIPstr);
	if (WiFi.getAutoConnect() != true) {
		WiFi.setAutoConnect(true);
	}
	File networks = SPIFFS.open(path, "r+");
	int fSize = networks.size();
	bool fileExist = fSize > 0;
	bool WiFiConnected = false;
	bool TargetConnected = false;
	int count;
	TWiFiNet *WiFiNetArr;
	Serial.println(String(path) + " size = " + String(fSize));
	if (fileExist) {
		count = fSize / sizeof(TWiFiNet);
		WiFiNetArr = new TWiFiNet[count];
		for (size_t i = 0; i < count; i++) {
			if (networks.read((uint8_t *)&WiFiNetArr[i], sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
				Serial.println("Error reading Wi-Fi networks");
			} else {
				Serial.println("Read network with SSID: " + String(WiFiNetArr[i].ssid));
			}
		}
	} else {
		count = 0;
	}
	networks.close();
	int n = 0;

	while (!WiFiConnected) {
		WiFiConnected = false;
		int attempts = 0;
		String ssid;
		String pass;
		if (fileExist) {
			ssid = WiFiNetArr[n].ssid;
			pass = WiFiNetArr[n].password;
		}
		Serial.println("Waiting to disconnect");
		while (WiFi.status() == WL_CONNECTED) {
			WiFi.disconnect(true);
		}
		Serial.println("Disconnected");
		if (n < count && fileExist) {
			if (ssid == "") {
				attempts = accessAttempts;
				Serial.println("Found empty wifi ssid with pass: \"" + pass + "\"");
			} else {
				Serial.print("Attempt to connect to \"" + ssid + "\"");
				WiFi.begin(ssid, pass);
			}
			n++;
		} else {
			Serial.print("Attempt to connect to " + String(baseSSID));
			WiFi.begin(baseSSID, basePASSWORD);
			n = 0;
		}
		while (!WiFiConnected && (attempts < accessAttempts)) {
			if (WiFi.status() == WL_CONNECTED && attempts > 0) {
				Serial.println("\r\nWiFi connected SSID: " + WiFi.SSID());
				WiFiConnected = true;
			} else {
				attempts++;
				Serial.print(".");
				delay(50);
			}
		}
		if (!WiFiConnected) {
			WiFi.disconnect(true);
		}
		Serial.println();
	}
	if (fileExist) {
		delete[] WiFiNetArr;
	}
	return false;
}