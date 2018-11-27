#include "Particle.h"
#include "CellularHelper.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

bool testRun = false;

void setup() {
	Serial.begin(9600);
}

void loop() {
	if (!testRun) {
		testRun = true;

		delay(5000);

		Log.info("isLTE=%d", CellularHelper.isLTE());

		CellularHelperCREGResponse resp;
		CellularHelper.getCREG(resp);
		Log.info(resp.toString().c_str());
	}

}


