#ifndef _LEDCONTROL_h
#define _LEDCONTROL_h

#include <vector>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int daySize = 4;
const int weekSize = 7;
const int weekCount = 5;
const int dayCount = (weekCount * weekSize);
const int overallOffset = 3;

const int stripLength = (dayCount * daySize + overallOffset);

const int LED_PIN = 4;	  // 2 pin
const int BUTTON_PIN = 5; // 1 pin
const int FORWARD_PIN = 13;
const int BACK_PIN = 15;
const int NUM_LEDS = 141; // число светодиодов
const int chunkSize = weekSize * daySize;
const int modeCount = 4;

typedef struct LedData {
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	LedData() {
		r = g = b = 255;
	}
};

typedef enum { Power = 0, CarbonDioxide = 1, Temperature = 2, displayModeCount = 3 } displayMode_t;
typedef std::vector<std::vector<std::vector<int>>> rawDisplayData_t;

class LedControl {
  private:
  public:
	const int loadBarLength = 20;
	const int canvasLength = 20;
	const int canvasHeight = 7;
	const int stepCount = 2 * canvasHeight + 2 * canvasLength - 4;
	LedControl();
	void ledInit(void);
	bool stripWrite(int n, uint8_t r, uint8_t g, uint8_t b, bool update);
	void loadLedData(std::vector<LedData> ledData);
	bool clearStrip(bool all);
	void clearModes(bool update);
	bool setDayStatue(int day, int part, uint8_t val, uint8_t inBrightness, uint8_t hour, bool tripleLedMode);
	bool fillDayStatue(int day, int part, int val);
	void initRawDataArr(void);
	void randomRawDataArr(void);
	bool writeRawDataToStrip(void);
	void nextMode(void);
	void showMode(void);
	bool draw(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool update);
	void loadscreen(void);

	Adafruit_NeoPixel strip;
	displayMode_t displayMode;
	rawDisplayData_t rawDisplayData;

	std::vector<LedData> ledsData;
};

extern LedControl leds;

#endif
