#include "CellularHelper.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);


const unsigned long CHECK_PERIOD = 120000;
unsigned long lastCheck = 0 ;

void setup() {
	Serial.begin();
}

void loop() {
	if (millis() - lastCheck >= CHECK_PERIOD) {
		Log.info("about to get location using CellLocate");

		CellularHelperLocationResponse loc = CellularHelper.getLocation(120000);

		Log.info(loc.toString().c_str());
		Particle.publish("location", loc.toString().c_str(), PRIVATE);

		lastCheck = millis();
	}
}

