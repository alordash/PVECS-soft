#include "LedControl.h"

LedControl leds;

LedControl::LedControl() {
	ledInit();
}

void LedControl::ledInit() {
	strip = Adafruit_NeoPixel(NUM_LEDS + overallOffset, LED_PIN, NEO_GRB + NEO_KHZ800);
	rawDisplayData.resize(modeCount + 1);
	displayMode = Power;
	Serial.println("STRIP.LENGTH = " + String(stripLength));
	LedData t;
	ledsData.reserve(stripLength);
	if (ledsData.size() == 0) {
		for (size_t i = 0; i < stripLength; i++) {
			ledsData.push_back(t);
		}
	}
	Serial.println("ledsData.size = " + String(ledsData.size()) + " ledsData[0].r = " + String(ledsData[0].r));
	clearStrip(true);
}

bool LedControl::stripWrite(int n, uint8_t r, uint8_t g, uint8_t b, bool update) {
	if (n < 0 || n > stripLength) {
		return false;
	} else {
		LedData t;
		t.r = r;
		t.g = g;
		t.b = b;
		ledsData[n] = t;
		strip.setPixelColor(n, r, g, b);
		if (update) {
			strip.show();
		}
		return true;
	}
}

void LedControl::loadLedData(std::vector<LedData> ledData) {
	int i = 0;

	for (auto &&Led : ledData) {
		stripWrite(i, Led.r, Led.g, Led.b, false);
		i++;
	}
	strip.show();
}

bool LedControl::clearStrip(bool all) {
	int n = (all ? 0 : overallOffset);
	bool res = true;
	for (int i = n; i < stripLength; i++) {
		if (!stripWrite(i, 0, 0, 0, false)) {
			res = false;
		}
	}
	strip.show();
	return res;
}
void LedControl::clearModes(bool update) {
	for (size_t i = 0; i < overallOffset; i++) {
		stripWrite(i, 0, 0, 0, update);
	}
}

bool LedControl::setDayStatue(int day, int part, uint8_t val, uint8_t inBrightness, uint8_t hour, bool tripleLedMode) {
	uint8_t debugDay = day;
	uint8_t debugPart = part;
	if ((day * part > NUM_LEDS) || (part > daySize) || (day * part < 0))
		return false;
	int weekOffset = day / weekSize;
	day = day % weekSize + 1;
	int n = weekOffset * weekSize * daySize;
	switch (part % 2) {
		case 0:
			n += (part + 1) * weekSize - day;
			break;
		case 1:
			n += part * weekSize + day - 1;
			break;
		default:
			return false;
			break;
	}

	n += overallOffset;

	float divider = 1.5;
	float brightness = inBrightness;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	if (hour < 24) {
		if (9 >= hour || hour >= 23) {
			divider = 1.75;
			brightness /= 2;
		}
	}
	
	brightness /= 100.0;
	if (tripleLedMode) {
		r = (uint8_t)(max(2 * val - 255, 0) / divider);
		g = (uint8_t)((255 - abs(val - 128)) / divider);
		b = (uint8_t)(max(255 - 2 * val, 0) / divider);
	} else {
		r = (uint8_t)(255 - abs(val - 255) / divider);
		g = (uint8_t)((255 - val) / divider);
		b = (uint8_t)max(r, g) / (8 * divider);
	}
	
	r = (uint8_t)(r * brightness);
	g = (uint8_t)(g * brightness);
	b = (uint8_t)(b * brightness);

	stripWrite(n, r, g, b, false);
	return true;
}

bool LedControl::fillDayStatue(int day, int part, int val) {
	if ((day * part > NUM_LEDS) || (part > daySize) || (day * part < 0))
		return false;
	int weekOffset = day / weekSize;
	day = day % weekSize + 1;

	int n = weekOffset * weekSize * daySize;
	switch (part % 2) {
		case 0:
			n += (part + 1) * weekSize - day;
			break;
		case 1:
			n += part * weekSize + day - 1;
			break;

		default:
			return false;
			break;
	}

	n += overallOffset;
	uint8_t value = (uint8_t)(val * 0.085);
	uint8_t blue = (uint8_t)(1.45 * val * 0.07);
	
	stripWrite(n, value, value, blue, false);
	return true;
}

void LedControl::initRawDataArr(void) {
	for (size_t i = 0; i < modeCount + 1; i++) {
		rawDisplayData[i].resize(dayCount);
		for (size_t j = 0; j < dayCount; j++) {
			rawDisplayData[i][j].resize(daySize);
			for (size_t w = 0; w < daySize; w++) {
				rawDisplayData[i][j][w] = 0;
			}
		}
	}
}

void LedControl::randomRawDataArr(void) {
	for (size_t i = 0; i < modeCount + 1; i++) {
		for (size_t j = 0; j < dayCount; j++) {
			for (size_t w = 0; w < daySize; w++) {
				rawDisplayData[i][j][w] = random(0, 255);
			}
		}
	}
}

bool LedControl::writeRawDataToStrip(void) {
	clearModes(false);

	stripWrite(displayMode, 155, 155, 155, false);
	strip.show();
	bool result = true;
	for (size_t i = 0; i < dayCount; i++) {
		for (size_t j = 0; j < daySize; j++) {
			if (!setDayStatue(i, j, rawDisplayData[displayMode + 1][i][j], 100, 100, false)) {
				result = false;
			}
		}
	}
	strip.show();
	return result;
}

void LedControl::nextMode() {
	switch (displayMode) {
		case Power:
			stripWrite(0, 155, 155, 155, false);
			displayMode = CarbonDioxide;
			break;
		case CarbonDioxide:
			displayMode = Temperature;
			stripWrite(1, 155, 155, 155, false);
			break;
		case Temperature:
			stripWrite(2, 155, 155, 155, false);
			displayMode = Power;
			break;
		default:
			break;
	}
	strip.show();
}

void LedControl::showMode() {
	clearModes(false);
	if (displayMode < 3) {
		stripWrite(displayMode, 155, 155, 155, false);
	}
	strip.show();
}

bool LedControl::draw(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool update) {
	int n = overallOffset - 1;
	if (x % 2 == 0) {
		n += (weekSize - y);
	} else {
		n += y + 1;
	}
	n += (weekSize * x);
	return stripWrite(n, r, g, b, update);
}
