#include "Particle.h"

#include "CellularHelper.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);


void setup() {
	Serial.begin();

	// Wait until the USB serial is connected or for 4 seconds. This is only for
	// debugging purposes so you can monitor the log messages.
	waitFor(Serial.isConnected, 4000);

	// The cellular modem must be turned on before using selectOperator. This version
	// is preferable to Cellular.on() if you are using SYSTEM_THREAD(ENABLED) because
	// the Cellular.on() method does not block and there's no way to know when it
	// has been turned on.
	cellular_on(NULL);

	// Select the operator using the MCC/MNC string. For example:
	// "310410" = AT&T
	// "310260" = T-Mobile
	bool bResult = CellularHelper.selectOperator("310260");

	Log.info("selectOperator returned %d", bResult);

	// We only do a Cellular.connect() here because doing a Particle.connect() generates
	// a lot of log messages obscuring the rest of the content. You may prefer to use
	// Particle.connect() instead.
	Cellular.connect();
	waitUntil(Cellular.ready);

	String longName = CellularHelper.getOperatorName();

	Log.info("current operator=%s", longName.c_str());
}

void loop() {

}
