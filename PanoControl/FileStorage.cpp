#include "board.h"
#include "FileStorage.h"
#include "FS.h"

FileStorage fileStorage;

FileStorage::FileStorage() {}

FileStorage::~FileStorage() {}

bool FileStorage::Start(void) {
	return SPIFFS.begin();
}

void FileStorage::Stop(void) {
	SPIFFS.end();
}

void FileStorage::ListDir(const char * dirname, uint8_t levels, FileStorage::TListDirFunction fn) {
	File root = SPIFFS.open(dirname, FILE_READ);
	if(!root) {
		return;
	}
	if(!root.isDirectory()) {
		return;
	}

	File file = root.openNextFile();
	while(file) {
		if(file.isDirectory()) {
			if(levels) {
				ListDir(file.name(), levels - 1, fn);
			}
		} else {
		}
		if(fn) {
			fn(file.name(), file.size());
		}
		file = root.openNextFile();
	}
}

bool FileStorage::DeleteFile(const char * path) {
	DisableWdt();
	bool res = SPIFFS.remove(path);
	EnableWdt();
	return res;
}

bool FileStorage::Format() {
	DisableWdt();
	bool res = SPIFFS.format();
	EnableWdt();
	return res;
}

bool FileStorage::WriteFile(const char * path, const uint8_t *data, size_t len) {
	File file = SPIFFS.open(path, FILE_WRITE);
	if(!file) {
		return false;
	}
	bool res = false;
	if(len > 0) {
		if(file.write(data, len) == len) {
			res = true;
		} else {
		}
	} else {
		res = true;
	}
	file.close();
	return res;
}

bool FileStorage::AppendFile(const char * path, const uint8_t *data, size_t len) {
	File file = SPIFFS.open(path, FILE_APPEND);
	if(!file) {
		return false;
	}
	bool res = false;
	if(file.write(data, len) == len) {
		res = true;
	} else {
	}
	file.close();
	return res;
}

bool FileStorage::Exists(const String& path) {
	return SPIFFS.exists(path);
}

bool FileStorage::Exists(const char * path) {
	return SPIFFS.exists(path);
}

File FileStorage::FileOpen(const String& path, const char* mode = FILE_READ) {
	return SPIFFS.open(path, mode);
}

bool FileStorage::ReadFile(const char * path, uint8_t *data, size_t len) {
	File file = SPIFFS.open(path, FILE_READ);
	if(!file) {
		return false;
	}
	bool res = false;
	if(len == file.size()) {
		if(file.read(data, len) == len) {
			res = true;
		}
	} else {
		res = false;
	}
	file.close();
	return res;
}

bool FileStorage::write(File& file, String& s) {
	return file.write((uint8_t*)s.c_str(), s.length()) == s.length();
}

size_t FileStorage::FileSize(const char * path){
	return SPIFFS.open(path, "r").size();
}

bool FileStorage::DeleteLogData(){
	return DeleteFile(FILE_NAME_LOG_DATA) && DeleteFile(FILE_CHECK_COUNT);
}