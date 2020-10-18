#include <Thread.h>
#include <vector>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <ModbusRtu.h>
#include <ArduinoOTA.h>
#include <mDNSResolver.h>
#include <functional>
#include <Arduino.h>
#include "FS.h"
#include "board.h"

#include "WebServer.h"

#define FILE_NAME_WIFI_NET "/WiFiNetworks"

#define POST_STRIP_DRAW "/stripDraw"
#define POST_STRIP_CLEAR "/stripClear"
#define POST_TEST_DATA "/rawData"
#define POST_CHANGE_MODE "/changeMode"
#define POST_PZEM_DATA "/pzemData"

#define PZEM_RX_PIN 4 // D2
#define PZEM_TX_PIN 5 // D1

#define TARGET_HOSTNAME "ect.local"

#define RESTART_TIMEOUT 1800000 // (30 minutes)

const int checkPeriod = 4; // IN SECONDS

typedef struct TLogData {
	uint32_t TimeStamp = 0;
	uint16_t Power = 0;
	uint16_t CarbonDioxide = 0;
	uint16_t Temperature = 0;
};

const char *OTAPASS = "abirvalg";

Thread CheckThread = Thread();
bool clientConnected = false;

WiFiUDP udp;
mDNSResolver::Resolver resolver(udp);

int testDataDays = 10;
std::vector<std::vector<TLogData>> rawData;

bool firstWiFi = false;

uint16_t au16data[10];
uint8_t u8state;
SoftwareSerial pzemSerial(PZEM_RX_PIN, PZEM_TX_PIN, false); // D2 rx, D1 tx
Modbus master(0, pzemSerial);
modbus_t telegram;
unsigned long u32wait;
bool dataExists = false;

int checkCount = 0;
uint32_t pzemPower = 0;
uint16_t pzemVoltage = 0;

void pzemInit() {
	pzemSerial.begin(9600); // baud-rate at 19200
	master.start();
	master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over
	u32wait = millis() + 1000;
	u8state = 0;
}

void OTA_init() {
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname("EnergyConsumptionTracker");
	ArduinoOTA.setPassword(OTAPASS);
	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		} else {
			type = "filesystem";
		}

		Serial.println("Start updating " + type);
	});
	ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();
}

void webServerInit() {
	if (!webServer.WiFi_init(FILE_NAME_WIFI_NET, "ect.local", resolver)) {
		webServer.target_ip = webServer.baseIP;
		Serial.printf("Failed to connect to target, using ip: %s\r\n", webServer.target_ip.toString().c_str());
	}
	webServer.server.on("/", HTTP_GET, []() {
		String sendText = "EnergyConsumptionTracker is OK, target_ip = " + webServer.target_ip.toString() + ", connected = ";
		if (clientConnected) {
			sendText += "true";
		} else {
			sendText += "false";
		}
		webServer.server.send(200, "text/html", sendText);
	});
	webServer.server.on("/restart", HTTP_GET, []() {
		webServer.server.send(200, "text/html", "Restarting ECT");
		delay(5000);
		ESP.restart();
	});
	webServer.server.begin();
}

void setup() {
	Serial.begin(115200);
	SPIFFS.begin();

	pzemInit();
	webServerInit();
	OTA_init();
	Serial.println("Starting loop");
}
bool hasData = false;
void loop() {
	ArduinoOTA.handle();
	webServer.loop();
	if (webServer.client.connect(webServer.target_ip, 80)) {
		clientConnected = true;
		if (hasData) {
			sendPzemData();
			hasData = false;
		}
	} else {
		clientConnected = false;
	}
	pzemReadData();
	if (millis() > RESTART_TIMEOUT) {
		Serial.printf("Time to restart, check count: %d", checkCount);
		delay(5000);
		ESP.restart();
	}
}

void pzemReadData() {
	switch (u8state) {
		case 0:
			if (millis() > u32wait) {
				u8state++; // wait state
			}
			break;
		case 1:
			telegram.u8id = 1;			 // slave address
			telegram.u8fct = 4;			 // function code (this one is registers read)
			telegram.u16RegAdd = 0;		 // start address in slave
			telegram.u16CoilsNo = 9;	 // number of elements (coils or registers) to read
			telegram.au16reg = au16data; // pointer to a memory array in the Arduino

			master.query(telegram); // send query (only once)
			u8state++;
			break;
		case 2:
			dataExists = master.poll() > 0;
			if (master.getState() == COM_IDLE) {
				if (dataExists) {
					Serial.printf("state: %d, val: %d %d %d %d %d %d %d %d %d\r\n", u8state, au16data[0], au16data[1], au16data[2], au16data[3], au16data[4],
								  au16data[5], au16data[6], au16data[7], au16data[8], au16data[9]);
					checkCount++;
					pzemVoltage = au16data[0];
					pzemPower = (au16data[4] << 16) + au16data[3];
					Serial.println("pzemPower = " + String(pzemPower));
					hasData = true;
				}
				u8state = 0;
				u32wait = millis() + checkPeriod * 1000;
			}
			break;
	}
}

void sendPzemData() {
	String sendData = "power=" + String(pzemPower);
	webServer.client.printf("POST %s HTTP/1.1\r\n", POST_PZEM_DATA);
	webServer.client.print("Host: ");
	webServer.client.print(host);
	webServer.client.print("\r\n");

	webServer.client.print("Content-Type: application/x-www-form-urlencoded\r\n");
	webServer.client.print("Content-Length: ");
	webServer.client.print(strlen(sendData.c_str()));
	webServer.client.print("\r\n\r\n");
	webServer.client.print(sendData);
	Serial.printf("send Pzem Data: %d\r\n", pzemPower);
	if (webServer.client.available()) {
		Serial.println("GOT WIFI LIST FROM SERVER");

		webServer.client.readStringUntil('^');
		char separator = '&';
		String countStr = "count=";
		String response = webServer.client.readStringUntil(separator);
		Serial.println("response = \"" + response + "\"");
		int count = atoi(response.substring(countStr.length()).c_str());
		Serial.println("count = \"" + String(count) + "\"");
		bool res = SPIFFS.remove(FILE_NAME_WIFI_NET);
		File f = SPIFFS.open(FILE_NAME_WIFI_NET, "w");
		for (size_t i = 0; i < count; i++) {
			String ssid = webServer.client.readStringUntil(separator);
			String pass = webServer.client.readStringUntil(separator);
			Serial.println("ssid = \"" + ssid + "\"");
			Serial.println("pass = \"" + pass + "\"");
			TWiFiNet writeWiFiNet;
			strcpy(writeWiFiNet.ssid, ssid.c_str());
			strcpy(writeWiFiNet.password, pass.c_str());
			if (f.write((uint8_t *)&writeWiFiNet, sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
				Serial.println("Error saving wifi net");
			} else {
				Serial.println("Stored WiFi network with SSID: " + ssid);
			}
		}
		f.close();
	}
}