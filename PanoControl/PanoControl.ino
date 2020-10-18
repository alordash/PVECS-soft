#include <Thread.h>
#include "Adafruit_NeoPixel.h"
#include <vector>
#include <string>
#include <string.h>
#include <ArduinoOTA.h>

#include "board.h"
#include "WebServer.h"
#include "FileStorage.h"
#include "Adafruit_CCS811.h"

#define SENSOR_SCL 14
#define SENSOR_SDA 12

char displayModeName[3][16] = {{"Power"}, {"CarbonDioxide"}, {"Temperature"}};

const long utcOffsetInHours = 3600 * 3;

const int maxDataFileSize = sizeof(TLogData) * daySize * 365; // 365 days
bool isFileOverFlown = false;

const int debugLedsOffset = 0;

TLogData maxLogDataVal;
TLogData minLogDataVal;
TLogData checkLogData;
int sensorCheckCount = 0;
int webServerCheckCount = 0;
int checkNumber = 0;

Thread CheckThread = Thread();

bool useLogData = false;
bool bigMonth = false;

bool modeSwitchButtonPressed = false;
bool forwardButtonPressed = false;
bool backButtonPressed = false;

bool returnMonthPressed = false;

bool formatPressed = false;

uint64_t accumulatingPower = 0;
uint64_t accumulatingCarbonDioxide = 0;
uint64_t accumulatingTemperature = 0;
uint16_t checksForAccumulation = 0;

int displayMonth = 0;
int displayYear = 1970;
uint32_t initDisplayTimeStamp = 0;
uint32_t displayMonthStartTimeStamp = 0;
uint32_t displayMonthEndTimeStamp = 0;

uint32_t startDay = 0;
uint32_t startMonth = 0;
uint32_t startWeekDay = 0;
uint32_t nextSaveTS = 0;
uint16_t cyclesToCheck = 0;
uint8_t lastDay = 0;
uint8_t lastPart = 0;

uint8_t ledsBrightness = 50;
bool isIdle = true;
bool startIdleCountdown = false;
uint32_t idleTime = 0;
uint16_t activeTime = checkPeriod;

uint16_t ObtainedDataPage = 0;
uint16_t sendWiFiListCounter = 1000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInHours);

Adafruit_CCS811 ccs;

void OTA_init() {
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname("PVECS");
	ArduinoOTA.setPassword("alordash!");
	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		} else {
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
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

uint32_t checkFileData(String path) {
	File f = SPIFFS.open(path, "r");
	int fSize = f.size();
	if (f) {
		if (fSize < sizeof(TLogData)) {
			Serial.println("Low logData file size");
		}
		f.close();
		return fSize;
	} else {
		Serial.println("Error openning file");
		f.close();
		return 0;
	}
}

bool storeLogData() {
	uint32_t TS = (timeClient.getEpochTime() / partTime) * partTime;
	Serial.printf("power: %d, CO2: %d, temperatue: %d, sensor Check count: %d\r\n", accumulatingPower, accumulatingCarbonDioxide, accumulatingTemperature,
				  sensorCheckCount);
	TLogData writingLogData;
	writingLogData.TimeStamp = TS;
	double divider = max(sensorCheckCount / (checksPerAccumulation + 1), 1);

	writingLogData.Power = (uint32_t)(accumulatingPower / max(1, webServerCheckCount));
	writingLogData.CarbonDioxide = (uint32_t)(accumulatingCarbonDioxide / divider);
	writingLogData.Temperature = (uint32_t)(accumulatingTemperature / divider);
	accumulatingPower = accumulatingCarbonDioxide = accumulatingTemperature = 0;

	Serial.printf("WRITE p = %d T = %d co2 = %d  TS = %d\r\n", writingLogData.Power, writingLogData.Temperature, writingLogData.CarbonDioxide,
				  writingLogData.TimeStamp);
	uint32_t fSize = checkFileData(FILE_NAME_LOG_DATA);
	bool res = true;
	if (fSize > 0) {
		uint32_t readTimeStamp = 0;
		TLogData readLogData;
		uint32_t length = fSize / sizeof(TLogData);
		File f = SPIFFS.open(FILE_NAME_LOG_DATA, FILE_READ);
		bool found = false;
		int i = 0;
		while (i < length && !found) {
			if (f.read((uint8_t *)&readTimeStamp, sizeof(uint32_t)) != sizeof(uint32_t)) {
				Serial.printf("error reading \"sensorRead\"\r\n");
			}
			f.seek(f.position() + sizeof(TLogData) - sizeof(uint32_t));
			if (readTimeStamp == TS) {
				Serial.printf("found at pos = %d\r\n", f.position());
				found = true;
			} else {
				i++;
			}
		}
		f.close();
		if (found) {
			f = SPIFFS.open(FILE_NAME_LOG_DATA, "r+");
			f.seek(i * sizeof(TLogData));
			if (f.write((uint8_t *)&writingLogData, sizeof(TLogData)) != sizeof(TLogData)) {
				Serial.printf("error writing \"sensorRead\"\r\n");
				res = false;
			}
			f.close();
		} else {
			if (fSize >= maxDataFileSize) {
				isFileOverFlown = true;
			} else {
				isFileOverFlown = false;
			}
			if (isFileOverFlown) {
				f = SPIFFS.open(FILE_NAME_LOG_DATA, "r+");
				f.seek((checkNumber % (maxDataFileSize / sizeof(TLogData))) * sizeof(TLogData));
			} else {
				f = SPIFFS.open(FILE_NAME_LOG_DATA, FILE_APPEND);
			}
			if (f.write((uint8_t *)&writingLogData, sizeof(TLogData)) != sizeof(TLogData)) {
				Serial.printf("error appending \"sensorRead\"\r\n");
				res = false;
			}
			f.close();
			if (!saveCheckCount()) {
				Serial.printf("Error saving CHECK COUNT\r\n");
			}
		}
	} else {
		File f = SPIFFS.open(FILE_NAME_LOG_DATA, FILE_WRITE);
		if (f.write((uint8_t *)&writingLogData, sizeof(TLogData)) != sizeof(TLogData)) {
			res = false;
			Serial.println("ERROR writing css data");
		}
		f.close();
	}
	checkNumber++;
	return res;
}

void getMonthTimestamp(int month, int year, uint32_t *startTimeStamp, uint32_t *endTimeStamp) {
	tm unixStart = {0};
	unixStart.tm_hour = 0;
	unixStart.tm_min = 0;
	unixStart.tm_sec = 0;
	unixStart.tm_year = 70;
	unixStart.tm_mon = 0;
	unixStart.tm_mday = 1;
	tm monthBegin = {0};
	monthBegin.tm_hour = 0;
	monthBegin.tm_min = 0;
	monthBegin.tm_sec = 0;
	monthBegin.tm_year = year - 1900;
	monthBegin.tm_mon = month;
	monthBegin.tm_mday = 1;
	tm monthEnd = {0};
	monthEnd.tm_hour = 0;
	monthEnd.tm_min = 0;
	monthEnd.tm_sec = 0;
	monthEnd.tm_year = year - 1900;
	monthEnd.tm_mon = month + 1;
	monthEnd.tm_mday = 1;
	time_t unixTime = mktime(&unixStart);
	*startTimeStamp = difftime(mktime(&monthBegin), unixTime);
	*endTimeStamp = difftime(mktime(&monthEnd), unixTime);
}

void loadCheckingLogData() {
	getMonthTimestamp(displayMonth, displayYear, &displayMonthStartTimeStamp, &displayMonthEndTimeStamp);
	uint32_t TS = checkLogData.TimeStamp;
	if (TS >= displayMonthStartTimeStamp && TS < displayMonthEndTimeStamp) {
		int weekDay = timeClient.getWeekDay(displayMonthStartTimeStamp) - 1;
		if (weekDay < 0) {
			weekDay = weekSize - 1;
		}

		if (forwardButtonPressed && bigMonth) {
			weekDay -= weekSize;
		}
		int sum = weekDay + monthDays[displayMonth];
		if (sum >= weekSize * weekCount) {
			Serial.println(("Big month, sum = " + String(sum)).c_str());
		}
		uint32_t readTimeStamp = TS - displayMonthStartTimeStamp;
		uint32_t day = (readTimeStamp / partTime) /* - startDay*/;
		uint8_t part = day % daySize;
		day = (day / daySize) + weekDay - debugLedsOffset;
		uint8_t hour = timeClient.getHours(TS);
		uint8_t val = getLogDataBrightness(checkLogData, minLogDataVal, maxLogDataVal, leds.displayMode);
		Serial.printf("Loading CHECKING LOG DATA, day: %d, part: %d, val: %d checkNumber: %d\r\n", day, part, val, checkNumber);
		
		if (!leds.setDayStatue(day, part, val, ledsBrightness, hour, leds.displayMode == Temperature)) {
			Serial.printf("Error loading check log data at day %d part %d\r\n", day, part);
		}
		leds.strip.show();
	}
}

bool loadLogData(bool clear) {
	bool res = true;
	leds.showMode();
	getMonthTimestamp(displayMonth, displayYear, &displayMonthStartTimeStamp, &displayMonthEndTimeStamp);
	Serial.println("DMTS = " + String(displayMonthStartTimeStamp));
	int weekDay = timeClient.getWeekDay(displayMonthStartTimeStamp) - 1;

	if (weekDay < 0) {
		weekDay = weekSize - 1;
	}

	if (forwardButtonPressed && bigMonth) {
		weekDay -= weekSize;
		bigMonth = false;
	}
	if (clear) {
		leds.clearStrip(false);
	}

	int beginDay = max(0, weekDay);
	int endDay = min((displayMonthEndTimeStamp - displayMonthStartTimeStamp) / (partTime * daySize) + weekDay, (uint32_t)(weekSize * weekCount));
	Serial.println("Filling from " + String(beginDay) + " to " + String(endDay));
	for (size_t i = beginDay; i < endDay; i++) {
		for (size_t j = 0; j < daySize; j++) {
			if (!leds.fillDayStatue(i, j, 100)) {
				Serial.println("ERROR fillDayStatue at (" + String(i) + ", " + String(j) + ")");
			}
		}
	}
	if (checkFileData(FILE_NAME_LOG_DATA)) {
		uint8_t curHour = (uint8_t)timeClient.getHours();
		TLogData readLogData;
		TLogData *arrLogData;
		readLogData.TimeStamp = 0;

		File f = SPIFFS.open(FILE_NAME_LOG_DATA, "r");
		int fSize = f.size();
		Serial.printf("maxDataFileSize: %d, f.size(): %d\r\n", maxDataFileSize, fSize);
		int length = fSize / sizeof(TLogData);
		arrLogData = new TLogData[length];
		Serial.printf("length = %d\r\n", length);
		int readCount = 0;

		for (int i = 0; i < length; i++) {
			if (f.read((uint8_t *)&readLogData, sizeof(TLogData)) != sizeof(TLogData)) {
				Serial.printf("loadLogData read logData ERROR\r\n");
				res = false;
			} else {
				if (readLogData.TimeStamp >= displayMonthStartTimeStamp && readLogData.TimeStamp < displayMonthEndTimeStamp) {
					arrLogData[i] = readLogData;
					readCount++;
				}
			}
		}

		if (weekDay + (readCount / daySize) - 1 >= weekSize * weekCount) {
			bigMonth = true;
			Serial.println(("Big month, readCount = " + String(readCount)).c_str());
		} else {
			bigMonth = false;
		}

		for (size_t i = 0; i < length; i++) {
			uint32_t TS = arrLogData[i].TimeStamp;
			if (TS >= displayMonthStartTimeStamp && TS < displayMonthEndTimeStamp) {
				uint32_t readTimeStamp = TS - displayMonthStartTimeStamp;
				uint32_t day = (readTimeStamp / partTime) - 1;
				uint8_t part = day % daySize;
				day = (day / daySize) + weekDay - debugLedsOffset;
				if (!leds.setDayStatue(day, part, getLogDataBrightness(arrLogData[i], minLogDataVal, maxLogDataVal, leds.displayMode), ledsBrightness, curHour,
									   leds.displayMode == Temperature)) {
					Serial.println("ERROR WRITELOGDATA at this readDay: " + String(day) + " readPart: " + String(part));
					res = false;
				}
				lastDay = day;
				lastPart = part;
			}
		}
		delete[] arrLogData;
		f.close();
		leds.strip.show();
		return res;
	} else {
		return false;
	}
}

void ledStripInit() {
	leds.ledInit();
	leds.strip.begin();
	leds.strip.setBrightness(50);
	leds.strip.clear();

	leds.initRawDataArr();
	leds.randomRawDataArr();

	leds.clearStrip(true);
}

void sensorRead(uint32_t TS) {
	if (ccs.available() && !ccs.readData()) {
		checkLogData.TimeStamp = TS;
		if (sensorCheckCount == 0) {
			checkLogData.CarbonDioxide = 0;
			checkLogData.Temperature = 0;
		}
		checkLogData.CarbonDioxide = ccs.getTVOC();
		checkLogData.Temperature = (uint16_t)(ccs.calculateTemperature() * 10);
		if (checksForAccumulation >= checksPerAccumulation) {
			accumulatingCarbonDioxide += checkLogData.CarbonDioxide;
			accumulatingTemperature += checkLogData.Temperature;
			checksForAccumulation = 0;
		} else {
			checksForAccumulation++;
		}
		sensorCheckCount++;
		
		Serial.println(("co2 = " + String(checkLogData.CarbonDioxide) + " temp = " + String(checkLogData.Temperature)).c_str());
		if (!useLogData) {
			useLogData = true;
		}
	}
}

void PeriodicCheck() {
	Serial.println("PeriodicCheck");
	timeClient.update();
	uint32_t curTS = timeClient.getEpochTime();
	sensorRead(curTS);
	Serial.printf("isIdle = %s, curTS = %d, idleTime = %d\r\n", isIdle ? "true" : "false", curTS, idleTime);
	if (!isIdle) {
		if (curTS > idleTime) {
			isIdle = true;
			uint8_t curHour = (uint8_t)timeClient.getHours(curTS);
			if (!(9 < curHour && curHour < 23)) {
				ledsBrightness = 25;
			} else {
				ledsBrightness = 50;
			}
			
			leds.clearStrip(false);
			idleTime = 0;
			Serial.println("Back to idle");
			displayMonth = timeClient.getMonth(curTS) - 1;
			displayYear = timeClient.getYear(curTS);
			if (useLogData) {
				if (loadLogData(false)) {
					Serial.println("Success loading log data");
				} else {
					Serial.println("Error loading log data");
				}
			}
		}
	}
	if (curTS > nextSaveTS) {
		nextSaveTS += partTime;

		bool res = storeLogData();
		if (res && !useLogData) {
			useLogData = true;
			leds.clearStrip(false);
		}
		if (!res) {
			Serial.println("Error saving log data in loop");
		}
		sensorCheckCount = 0;
		webServerCheckCount = 0;
		loadLogData(false);
	}
	loadCheckingLogData();
}

void spiffsInit() {
	if (fileStorage.Start()) {
		Serial.println("SPIFFS Initialize....ok");
	} else {
		Serial.println("SPIFFS Initialization...failed");
	}
}

bool boundsInit() {
	bool res = true;
	File f = SPIFFS.open(FILE_NAME_BOUNDS, FILE_READ);
	if (f.size() == sizeof(TLogData) * 2) {
		if (f.read((uint8_t *)&maxLogDataVal, sizeof(TLogData)) != sizeof(TLogData)) {
			res = false;
		}
		if (f.read((uint8_t *)&minLogDataVal, sizeof(TLogData)) != sizeof(TLogData)) {
			res = false;
		}
	} else {
		maxLogDataVal.Power = 2500;
		maxLogDataVal.Temperature = 600;
		maxLogDataVal.CarbonDioxide = 800;
	}
	f.close();
	return res;
}

bool loadCheckCount() {
	File f = SPIFFS.open(FILE_CHECK_COUNT, FILE_READ);
	if (f.size() > 0) {
		if (f.read((uint8_t *)&checkNumber, sizeof(checkNumber)) == sizeof(checkNumber)) {
			f.close();
			return true;
		}
	} else {
		checkNumber = 0;
	}
	f.close();
	return false;
}

bool saveCheckCount() {
	File f = SPIFFS.open(FILE_CHECK_COUNT, FILE_WRITE);
	if (f.write((uint8_t *)&checkNumber, sizeof(checkNumber)) == sizeof(checkNumber)) {
		f.close();
		return true;
	}
	f.close();
	return false;
}

uint16_t GetLogDataVal(TLogData t, displayMode_t displayMode) {
	switch (displayMode) {
		case Power:
			return (t.Power);
			break;
		case CarbonDioxide:
			return (t.CarbonDioxide);
			break;
		case Temperature:
			return (t.Temperature);
			break;
		default:
			return 0;
			break;
	}
}

String TimestampToString(uint32_t TS) {
	uint8_t date = timeClient.getDate(TS);
	uint8_t month = timeClient.getMonth(TS);
	uint8_t year = timeClient.getYear(TS);
	uint8_t curYear = timeClient.getYear();
	uint8_t hour = timeClient.getHours(TS);
	uint8_t minute = timeClient.getMinutes(TS);
	String s = String(date);
	if (date < 10) {
		s = "0" + s;
	}
	if (month < 10) {
		s += ".0" + String(month);
	} else {
		s += "." + String(month);
	}
	if (year != curYear) {
		s += "." + String(year);
		s += DataObtainedTimeShort;
	} else {
		s += DataObtainedTimeLong;
	}
	if (hour < 10) {
		s += "0" + String(hour);
	} else {
		s += String(hour);
	}
	s += ":";
	if (minute < 10) {
		s += "0" + String(minute);
	} else {
		s += String(minute);
	}
	return s;
}

void GenerateObtainedDataPage(int pageOffset) {
	char *cWP = new char[DataObtainedPageSize];
	char cD[100];
	char *cCat = new char[DataObtainedPageSize];
	strcpy_P(cWP, DataObtainedPageBegin);

	Serial.printf("Current page: %d, pagedOffset: %d, DataObtainedPageSize = %d\r\n", ObtainedDataPage, pageOffset, DataObtainedPageSize);
	ObtainedDataPage += pageOffset;
	if (ObtainedDataPage < 0) {
		ObtainedDataPage = 0;
	}

	if (checkFileData(FILE_NAME_LOG_DATA)) {
		TLogData readLogData;
		readLogData.TimeStamp = 0;

		File f = SPIFFS.open(FILE_NAME_LOG_DATA, "r");
		int fSize = f.size();
		int length = fSize / sizeof(TLogData);

		int offset = ObtainedDataPage * DataObtainedMaxSamples;
		Serial.printf("maxDataFileSize: %d, fSize: %d, length: %d, offset: %d\r\n", maxDataFileSize, fSize, length, offset);
		while (length <= offset) {
			ObtainedDataPage--;
			offset = ObtainedDataPage * DataObtainedMaxSamples;
		}
		int count = min(length - offset, DataObtainedMaxSamples);
		Serial.printf("count: %d\r\n", count);

		sprintf(cD, "%d", ObtainedDataPage);
		strcat(cWP, cD);
		strcpy_P(cCat, DataObtainedPageStyles);
		strcat(cWP, cCat);
		f.seek(offset * sizeof(TLogData));
		for (int i = 0; i < count; i++) {
			if (f.read((uint8_t *)&readLogData, sizeof(TLogData)) != sizeof(TLogData)) {
				Serial.printf("loadLogData read logData ERROR\r\n");
			} else {
				strcpy_P(cCat, DataObtainedTime);
				strcat(cWP, cCat);
				strcpy_P(cCat, TimestampToString(readLogData.TimeStamp).c_str());
				strcat(cWP, cCat);
				strcpy_P(cCat, DataObtainedPower);
				strcat(cWP, cCat);
				sprintf(cD, "%d", readLogData.Power);
				strcat(cWP, cD);
				strcpy_P(cCat, DataObtainedCO2);
				strcat(cWP, cCat);
				sprintf(cD, "%d", readLogData.CarbonDioxide);
				strcat(cWP, cD);
				strcpy_P(cCat, DataObtainedTemperature);
				strcat(cWP, cCat);
				sprintf(cD, "%d", readLogData.Temperature);
				strcat(cWP, cD);
				strcpy_P(cCat, DataObtainedRowEnd);
				strcat(cWP, cCat);
			}
		}
		f.close();
	} else {
		strcpy_P(cCat, DataObtainedPageStyles);
		strcat(cWP, cCat);
	}

	strcpy_P(cCat, DataObtainedPageEnd);
	strcat(cWP, cCat);
	webServer.server.send(200, "text/html", cWP);
	delete[] cCat;
	delete[] cWP;
	Serial.println("Data table sent");
}

void GenerateSettingsPage(int modeCount) {
	char *cWP = new char[settingsPageSize];
	char cD[100];
	char *cCat = new char[settingsPageSize];
	strcpy_P(cWP, SettingsPageBegin);

	for (int i = 0; i < modeCount; i++) {
		strcpy_P(cCat, SettingsModeBegin);
		strcat(cWP, cCat);
		sprintf(cD, "%d", 11 + 28 * (i + 1));
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsModeMiddle);
		strcat(cWP, cCat);
		sprintf(cD, "%s %d", displayModeName[i], GetLogDataVal(checkLogData, static_cast<displayMode_t>(i)));
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsModeEnd);
		strcat(cWP, cCat);
	}

	for (int i = 0; i < modeCount; i++) {
		strcpy_P(cCat, SettingsLeftBoundBegin);
		strcat(cWP, cCat);
		sprintf(cD, "%d", -13 + 45 * (i + 1));
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsLeftBoundEnd);
		strcat(cWP, cCat);

		strcpy_P(cCat, SettingsRightBoundBegin);
		strcat(cWP, cCat);
		sprintf(cD, "%d", -13 + 45 * (i + 1));
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsRightBoundEnd);
		strcat(cWP, cCat);
	}

	for (int i = 0; i < modeCount; i++) {
		strcpy_P(cCat, SettingsLeftBorderBegin);
		strcat(cWP, cCat);
		sprintf(cD, "%d", i);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsLeftBorderMiddle);
		strcat(cWP, cCat);
		sprintf(cD, "%d", 25 + 45 * i);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsLeftBorderEnd);
		strcat(cWP, cCat);
		uint16_t t = GetLogDataVal(minLogDataVal, static_cast<displayMode_t>(i));
		Serial.print("tMin = " + String(t, DEC) + " ");
		sprintf(cD, "%d", t);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsLeftBordeClose);
		strcat(cWP, cCat);

		strcpy_P(cCat, SettingsRightBorderBegin);
		strcat(cWP, cCat);
		sprintf(cD, "%d", t = i);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsRightBorderMiddle);
		strcat(cWP, cCat);
		sprintf(cD, "%d", 25 + 45 * i);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsRightBorderEnd);
		strcat(cWP, cCat);
		t = GetLogDataVal(maxLogDataVal, static_cast<displayMode_t>(i));
		Serial.println("tMax = " + String(t, DEC));
		sprintf(cD, "%d", t);
		strcat(cWP, cD);
		strcpy_P(cCat, SettingsRightBordeClose);
		strcat(cWP, cCat);
	}

	strcpy_P(cCat, SettingsPageEnd);
	strcat(cWP, cCat);
	webServer.server.send(200, "text/html", cWP);
	delete[] cCat;
	delete[] cWP;
	Serial.println("Settings sent");
}

bool GenerateWiFiNetPage(bool tooBig) {
	File f = SPIFFS.open(FILE_NAME_WIFI_NET, "r+");
	int fSize = f.size();
	char *cWP = new char[WiFiNetPageSize + fSize];
	bool res = true;
	char *cCat = new char[WiFiNetPageSize];
	strcpy_P(cWP, WiFiNetPageBegin);
	if (tooBig) {
		strcpy_P(cCat, WiFiNetPageStoreError);
		strcat(cWP, cCat);
	}
	strcpy_P(cCat, WiFiNetPageBegin2);
	strcat(cWP, cCat);

	if (fSize > 0) {
		int count = fSize / sizeof(TWiFiNet);
		TWiFiNet WiFiNetRead;
		for (size_t i = 0; i < count; i++) {
			if (f.read((uint8_t *)&WiFiNetRead, sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
				res = false;
				Serial.println("READ WIFI error");
			}
			strcpy_P(cCat, WiFiNetListBegin);
			strcat(cWP, cCat);
			sprintf(cCat, "%s", WiFiNetRead.ssid);
			strcat(cWP, cCat);
			strcpy_P(cCat, WiFiNetListMiddle);
			strcat(cWP, cCat);
			strcat(cWP, WiFiNetRead.ssid);
			strcpy_P(cCat, WiFiNetListEnd);
			strcat(cWP, cCat);
		}
	} else {
		strcpy_P(cCat, WiFiNetEmptyList);
		strcat(cWP, cCat);
	}
	f.close();
	strcpy_P(cCat, WiFiNetPageEnd);
	strcat(cWP, cCat);
	strcat(cWP, WiFi.SSID().c_str());
	strcpy_P(cCat, WiFiNetPageClose);
	strcat(cWP, cCat);
	webServer.server.send(200, "text/html", cWP);
	Serial.println("Sent wiFi page");
	delete[] cCat;
	delete[] cWP;

	return res;
}

void webServerInit() {
	webServer.WiFi_init(FILE_NAME_WIFI_NET);
	webServer.MDNS_Init();
	webServer.server.on("/", HTTP_GET, []() {
		char *cWP = new char[IndexPageSize];
		strcpy_P(cWP, IndexPage);
		webServer.server.send(200, "text/html", cWP);
		delete[] cWP;
		Serial.println("enter '/' page");
	});
	webServer.server.on("/index", HTTP_GET, []() {
		char *cWP = new char[IndexPageSize];
		strcpy_P(cWP, IndexPage);
		webServer.server.send(200, "text/html", cWP);
		delete[] cWP;
		Serial.println("INDEX page");
	});
	webServer.server.on("/WiFiSettings", []() {
		Serial.println("WiFi settings page");
		if (!GenerateWiFiNetPage(false)) {
			Serial.println("Error loading WiFi web page");
		}
	});
	webServer.server.on("/saveWiFiNet", []() {
		TWiFiNet writeWiFiNet;
		strcpy(writeWiFiNet.ssid, webServer.server.arg("SSID").c_str());
		strcpy(writeWiFiNet.password, webServer.server.arg("pass").c_str());
		Serial.printf("Saving WiFi net with SSID = %s\r\n", writeWiFiNet.ssid);
		File f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_APPEND);
		int fSize = f.size();
		bool tooBig = false;
		if (fSize >= maxWiFiNetworksStored) {
			Serial.println("Stored too many wi-fi networks");
			tooBig = true;
			f.close();
		} else {
			bool found = false;
			int index = 0;
			if (fSize <= 0) {
				f.close();
				f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_WRITE);
			} else {
				f.close();
				f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_READ);
				int count = fSize / sizeof(TWiFiNet);
				TWiFiNet readWiFiNet;
				while (index < count && !found) {
					if (f.read((uint8_t *)&readWiFiNet, sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
						Serial.println("Error reading Wi-Fi networks");
					}
					if (readWiFiNet.ssid == writeWiFiNet.ssid) {
						found = true;
					} else {
						index++;
					}
				}
				f.close();
				if (found) {
					f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_WRITE);
				} else {
					f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_APPEND);
				}
			}
			if (found) {
				f.seek(index * sizeof(TWiFiNet));
				Serial.println("Rewriting WiFi " + String(writeWiFiNet.ssid) + " found at " + String(index));
			}
			if (f.write((uint8_t *)&writeWiFiNet, sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
				Serial.println("Error saving wifi net");
			}
			f.close();
		}
		GenerateWiFiNetPage(tooBig);
	});
	webServer.server.on("/delWiFiNet", []() {
		File f = SPIFFS.open(FILE_NAME_WIFI_NET, "r+");
		int fSize = f.size();
		int count = fSize / sizeof(TWiFiNet);
		if (count == 1) {
			f.close();
			fileStorage.DeleteFile(FILE_NAME_WIFI_NET);
		} else {
			String delWiFiName = webServer.server.arg("name");
			TWiFiNet *WiFiNetArr = new TWiFiNet[count - 1];
			bool t = false;
			TWiFiNet wiFiRead;
			for (size_t i = 0; i < count; i++) {
				f.read((uint8_t *)&wiFiRead, sizeof(TWiFiNet));
				if (strcmp(wiFiRead.ssid, delWiFiName.c_str()) == 0) {
					t = true;
				} else {
					if (t) {
						WiFiNetArr[i - 1] = wiFiRead;
					} else {
						WiFiNetArr[i] = wiFiRead;
					}
				}
			}
			f.close();
			fileStorage.DeleteFile(FILE_NAME_WIFI_NET);
			f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_WRITE);
			f.write((uint8_t *)&WiFiNetArr[0], sizeof(TWiFiNet));
			f.close();
			f = SPIFFS.open(FILE_NAME_WIFI_NET, FILE_APPEND);
			for (size_t i = 1; i < count - 1; i++) {
				f.write((uint8_t *)&WiFiNetArr[i], sizeof(TWiFiNet));
			}
			f.close();
			delete[] WiFiNetArr;
		}
		GenerateWiFiNetPage(false);
	});
	webServer.server.on("/LEDControl", []() {
		GenerateSettingsPage(3);
		Serial.println("LED Control page");
	});
	webServer.server.on("/getParam", []() {
		maxLogDataVal.Power = atoi(webServer.server.arg("UB0").c_str());
		minLogDataVal.Power = atoi(webServer.server.arg("LW0").c_str());
		maxLogDataVal.CarbonDioxide = atoi(webServer.server.arg("UB1").c_str());
		minLogDataVal.CarbonDioxide = atoi(webServer.server.arg("LW1").c_str());
		maxLogDataVal.Temperature = atoi(webServer.server.arg("UB2").c_str());
		minLogDataVal.Temperature = atoi(webServer.server.arg("LW2").c_str());

		Serial.printf("Changed min vals to: %d %d %d and max to: %d %d %d\r\n", minLogDataVal.Power, minLogDataVal.CarbonDioxide, minLogDataVal.Temperature,
					  maxLogDataVal.Power, maxLogDataVal.CarbonDioxide, maxLogDataVal.Temperature);

		fileStorage.DeleteFile(FILE_NAME_BOUNDS);
		File f = SPIFFS.open(FILE_NAME_BOUNDS, FILE_WRITE);
		f.write((uint8_t *)&maxLogDataVal, sizeof(TLogData));
		f.close();
		f = SPIFFS.open(FILE_NAME_BOUNDS, FILE_APPEND);
		f.write((uint8_t *)&minLogDataVal, sizeof(TLogData));
		f.close();

		GenerateSettingsPage(3);

		loadLogData(true);
		loadCheckingLogData();
	});
	webServer.server.on(POST_PZEM_DATA, HTTPMethod::HTTP_POST, []() {
		uint32_t P = atoi(webServer.server.arg("power").c_str());
		Serial.printf("received P: %d\r\n", P);
		if (webServerCheckCount == 0) {
			accumulatingPower = 0;
		}
		checkLogData.Power = P / 10;
		accumulatingPower += checkLogData.Power;
		webServerCheckCount++;
		uint32_t TS = timeClient.getEpochTime();
		checkLogData.TimeStamp = TS;
		if (sendWiFiListCounter > 1000) {
			String response = "^count=";
			int count;
			TWiFiNet *WiFiNetArr;
			File networks = SPIFFS.open(FILE_NAME_WIFI_NET, "r+");
			int fSize = networks.size();
			bool fileExist = fSize > 0;
			if (fileExist) {
				count = fSize / sizeof(TWiFiNet);
				WiFiNetArr = new TWiFiNet[count];
				for (size_t i = 0; i < count; i++) {
					if (networks.read((uint8_t *)&WiFiNetArr[i], sizeof(TWiFiNet)) != sizeof(TWiFiNet)) {
						Serial.println("Error reading Wi-Fi networks");
					}
				}
				response += String(count) + "&";
				for (size_t i = 0; i < count; i++) {
					response += String(WiFiNetArr[i].ssid) + "&";
					response += String(WiFiNetArr[i].password) + "&";
				}
				response += "\n";
				webServer.server.send(200, "text/plain", response);
			}
			sendWiFiListCounter = 0;
		} else {
			sendWiFiListCounter++;
		}
	});
	webServer.server.on("/ObtainedData", []() {
		Serial.println("/ObtainedData");
		GenerateObtainedDataPage(0);
	});
	webServer.server.on("/ObtainedDataBack", []() {
		Serial.println("/ObtainedDataBack");
		GenerateObtainedDataPage(-1);
	});
	webServer.server.on("/ObtainedDataForward", []() {
		Serial.println("/ObtainedDataForward");
		GenerateObtainedDataPage(1);
	});
	webServer.server.begin();
}

void setup() {
	Serial.begin(115200);
	delay(10);
	spiffsInit();
	delay(10);
	if (!boundsInit()) {
		Serial.println("Error initializing BOUNDS");
	}
	delay(10);
	ledStripInit();
	delay(10);
	if (ccs.begin((0x5A), SENSOR_SDA, SENSOR_SCL)) {
		Serial.println("CO2 sensor Ok");
	} else {
		Serial.println("Error starting CO2 sensor");
		delay(1000);
		ESP.restart();
	}

	while (!ccs.available()) {
		Serial.println("Waiting sensor...");
		delay(1500);
	}
	delay(100);
	webServerInit();
	delay(100);
	OTA_init();
	delay(100);
	timeClient.begin();

	delay(1000);
	while (timeClient.getYear() == 1970) {
		timeClient.update();
		delay(500);
	}
	uint32_t TS = timeClient.getEpochTime();

	displayMonth = timeClient.getMonth(TS) - 1;
	displayYear = timeClient.getYear(TS);
	Serial.println(("CURRENT YEAR = " + String(timeClient.getYear())).c_str());
	getMonthTimestamp(displayMonth, displayYear, &displayMonthStartTimeStamp, &displayMonthEndTimeStamp);
	Serial.println(("StartMonthTimeStamp = " + String(displayMonthStartTimeStamp) + "\nEndMonthTimeStamp = " + String(displayMonthEndTimeStamp)).c_str());
	startWeekDay = timeClient.getWeekDay();
	Serial.println(("startMonth = " + String(displayMonth) + " startWeekDay = " + String(startWeekDay)).c_str());
	checkLogData.TimeStamp = TS;

	uint32_t readTimeStamp = TS - displayMonthStartTimeStamp;
	startDay = (readTimeStamp / partTime);

	loadCheckCount();
	initDisplayTimeStamp = displayMonthStartTimeStamp;

	nextSaveTS = ((timeClient.getEpochTime() / partTime) + 1) * partTime;
	delay(10);
	Serial.print("Start: ");
	if (checkFileData(FILE_NAME_LOG_DATA)) {
		useLogData = true;
		if (!loadLogData(true)) {
			Serial.println("Error loadLogData()");
		}
	} else {
		Serial.println("No data saved");
	}
	Serial.println(webServer.baseSSID);
	Serial.println(WiFi.localIP());
	delay(10);
	leds.showMode();

	CheckThread.onRun(PeriodicCheck);
	CheckThread.setInterval(checkPeriod * 1000);
}

void loop() {
	ArduinoOTA.handle();
	webServer.loop();

	if (digitalRead(BUTTON_PIN) == HIGH) {
		if (!modeSwitchButtonPressed) {
			modeSwitchButtonPressed = true;
			leds.nextMode();
			leds.clearStrip(false);
			Serial.print("Current mode: ");

			Serial.println(displayModeName[leds.displayMode]);
			if (useLogData) {
				if (loadLogData(true)) {
					Serial.println("Success writing server data");
				} else {
					Serial.println("Error writing server data");
				}
			} else {
				if (checkFileData(FILE_NAME_LOG_DATA) != 0) {
					useLogData = true;
					if (loadLogData(true)) {
						Serial.println("Success loading log data");
					} else {
						Serial.println("Error loading log data");
					}
				} else if (!leds.writeRawDataToStrip()) {
					Serial.println("Error");
				} else {
					Serial.println("Error");
				}
			}
			loadCheckingLogData();
		}
	} else {
		modeSwitchButtonPressed = false;
	}

	if (digitalRead(FORWARD_PIN)) {
		if (!forwardButtonPressed) {
			forwardButtonPressed = true;
			if (!bigMonth) {
				displayMonth++;
				if (displayMonth > monthSize - 1) {
					displayMonth = 0;
					displayYear++;
				}
			}
			Serial.println(("FORWARD_PIN month: " + String(displayMonth) + " year: " + String(displayYear)).c_str());
			if (useLogData) {
				if (loadLogData(true)) {
					Serial.println("Success loading log data");
				} else {
					Serial.println("Error loading log data");
				}
				loadCheckingLogData();
			}
		}
	} else {
		forwardButtonPressed = false;
	}

	if (digitalRead(BACK_PIN)) {
		if (!backButtonPressed) {
			backButtonPressed = true;
			displayMonth--;
			if (displayMonth < 0) {
				displayMonth = monthSize - 1;
				displayYear--;
			}
			Serial.println(("BACK_PIN month: " + String(displayMonth) + " year: " + String(displayYear)).c_str());
			if (useLogData) {
				if (loadLogData(true)) {
					Serial.println("Success loading log data");
				} else {
					Serial.println("Error loading log data");
				}
				loadCheckingLogData();
			}
		}
	} else {
		backButtonPressed = false;
	}

	if (modeSwitchButtonPressed && forwardButtonPressed && backButtonPressed) {
		if (!formatPressed) {
			formatPressed = true;
			Serial.println("Starting formatting LogData");
			useLogData = false;
			if (fileStorage.DeleteLogData()) {
				leds.clearStrip(false);
				checkNumber = 0;
				Serial.println("Succesfully formatted LogData");
			} else {
				Serial.println("Error formating LogData");
			}
		}
	} else {
		formatPressed = false;
	}

	if (forwardButtonPressed && backButtonPressed && !formatPressed) {
		if (!returnMonthPressed) {
			returnMonthPressed = true;
			displayMonth = timeClient.getMonth() - 1;
			displayYear = timeClient.getYear();
			Serial.println(("RETURN TO CURRENT month: " + String(displayMonth) + " year: " + String(displayYear)).c_str());
			if (useLogData) {
				if (!loadLogData(true)) {
					Serial.println("Error writing server data");
				}
				loadCheckingLogData();
			}
		}
	} else {
		returnMonthPressed = false;
	}

	if (forwardButtonPressed || backButtonPressed || modeSwitchButtonPressed) {
		if (!startIdleCountdown) {
			startIdleCountdown = true;
			isIdle = false;
			uint32_t curTS = timeClient.getEpochTime();
			idleTime = curTS + activeTime;
			uint8_t curHour = (uint8_t)timeClient.getHours(curTS);
			if (!(9 < curHour && curHour < 23))
				ledsBrightness = 50;
			else
				ledsBrightness = 75;
			//			leds.strip.setBrightness(ledsBrightness);
			leds.strip.show();
			Serial.println("Activated from idle state");
		}
	} else if (startIdleCountdown) {
		startIdleCountdown = false;
	}

	if (CheckThread.shouldRun()) {
		CheckThread.run();
	}
}

uint8_t getLogDataBrightness(TLogData logData, TLogData minVal, TLogData maxVal, displayMode_t displayMode) {
	int val = GetLogDataVal(logData, displayMode);
	int Min = GetLogDataVal(minVal, displayMode);
	int Max = GetLogDataVal(maxVal, displayMode);
	
	val = (255 * (val - Min)) / max(1, Max - Min);
	if (val > 255) {
		val = 255;
	} else if (val < 0) {
		val = 0;
	}

	return (uint8_t)val;
}
