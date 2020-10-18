#include "WebServer.h"

void loadscreen(int step) {
	for (size_t i = 0; i < leds.loadBarLength; i++) {
		int brightness = (255 * i) / (leds.loadBarLength - 1);
		int bottomSide = leds.canvasLength + leds.canvasHeight - 1;
		int leftSide = 2 * leds.canvasLength + leds.canvasHeight - 2;
		int topSide = 2 * leds.canvasLength + 2 * leds.canvasHeight - 3;
		int n = step + i;
		if (n >= topSide) {
			n = n - topSide + 1;
			leds.draw(n, 0, brightness, brightness, brightness, true);
		} else if (n >= leftSide) {
			n = n - leftSide + 1;
			leds.draw(0, leds.canvasHeight - n - 1, brightness, brightness, brightness, true);
		} else if (n >= bottomSide) {
			n = n - bottomSide + 1;
			leds.draw(leds.canvasLength - n - 1, leds.canvasHeight - 1, brightness, brightness, brightness, true);
		} else if (n >= leds.canvasLength) {
			n = n - leds.canvasLength + 1;
			leds.draw(leds.canvasLength - 1, n, brightness, brightness, brightness, true);
		} else {
			leds.draw(n, 0, brightness, brightness, brightness, true);
		}
	}
}

WebServer webServer;

WebServer::WebServer() {
}

void WebServer::loop(void) {
	MDNS.update();
	server.handleClient();
}

void WebServer::WiFi_init(const char *path) {
	if (WiFi.getAutoConnect() != true) {
		WiFi.setAutoConnect(true);
	}
	leds.clearStrip(false);
	File networks = SPIFFS.open(path, "r+");
	int fSize = networks.size();
	bool fileExist = fSize > 0;
	bool connected = false;
	int count;
	TWiFiNet *WiFiNetArr;
	if (fileExist) {
		count = fSize / sizeof(TWiFiNet);
		WiFiNetArr = new TWiFiNet[count];
		for (size_t i = 0; i < count; i++) {
			if (networks.read((uint8_t *)&WiFiNetArr[i], sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
				Serial.println("Error reading Wi-Fi networks");
			}
		}
	} else {
		count = 0;
	}
	networks.close();
	int n = 0;
	int step = 0;
	while (!connected) {
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
				Serial.print("Attempt to connect to \"" + ssid /* + "\" with pass: \"" + pass*/ + "\"");
				WiFi.begin(ssid, pass);
			}
			n++;
		} else {
			Serial.print("Attempt to connect to " + String(baseSSID));
			WiFi.begin(baseSSID, basePASSWORD);
			n = 0;
		}
		while (!connected && (attempts < accessAttempts)) {
			if (WiFi.status() == WL_CONNECTED) {
				Serial.println("\r\nWiFi connected SSID: " + WiFi.SSID());
				connected = true;
			} else {
				attempts++;
				loadscreen(step);
				Serial.print(".");
				step++;
				if (step >= leds.stepCount) {
					step = 0;
				}

				delay(50);
			}
		}
		if (!connected) {
			WiFi.disconnect(true);
		}
		Serial.println();
	}
	if (fileExist) {
		delete[] WiFiNetArr;
	}
	leds.clearStrip(false);
}

void WebServer::MDNS_Init() {
	if (MDNS.begin(mDNSaddress)) {
		Serial.println("Started mDNS");
	}
	MDNS.addService("http", "tcp", 80);
}