#ifndef _FILESTORAGE_h
#define _FILESTORAGE_h

#include <functional>
#include <Arduino.h>
#include "FS.h"

#define YEAR_DAY 365.24
#define DAY_HOUR 24
#define HOUR_SEC 3600
#define ONEYEAR (YEAR_DAY * DAY_HOUR * HOUR_SEC)

#define FILE_WRITE "w"
#define FILE_APPEND "a"
#define FILE_READ "r"

#define FILE_NAME_LOG_DATA "/panoLog.data"
#define FILE_CHECK_COUNT "/checkCount"
#define FILE_NAME_BOUNDS "/bounds"
#define FILE_NAME_WIFI_NET "/WiFiNetworks"

const int checkPeriod = 10;
const int checksPerAccumulation = 4;
const int partTime = 21600;

const int monthSize = 12;
const int monthDays[monthSize] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

typedef struct TLogData {
  uint32_t TimeStamp = 0;
  uint32_t Power = 0;
  uint16_t CarbonDioxide = 0;
  uint16_t Temperature = 0;
};
 
class FileStorage {
private:
	bool write(File& file, String& s);
public:
	typedef std::function<void(String, int)> TListDirFunction;
	FileStorage();
	~FileStorage();
	bool Start(void);
	void Stop(void);
	void ListDir(const char * dirname, uint8_t levels, TListDirFunction fn);
	bool DeleteFile(const char * path);
	bool Format(); 
	bool WriteFile(const char * path, const uint8_t *data = NULL, size_t len = 0);
	bool AppendFile(const char * path, const uint8_t *data, size_t len);
	bool Exists(const String& path);
	bool Exists(const char* path);
	File FileOpen(const String& path, const char* mode);
	bool ReadFile(const char * path, uint8_t *data, size_t len);
	size_t FileSize(const char * path);

	bool DeleteLogData(void);
};
extern FileStorage fileStorage;

#endif
