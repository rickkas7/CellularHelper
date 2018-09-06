#include "Particle.h"
#include "CellularHelper.h"

// STARTUP(cellular_credentials_set("epc.tmobile.com", "", "", NULL));

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

const unsigned long STARTUP_WAIT_TIME_MS = 4000;
const unsigned long MODEM_ON_WAIT_TIME_MS = 4000;
const unsigned long CLOUD_PRE_WAIT_TIME_MS = 5000;
const unsigned long DISCONNECT_WAIT_TIME_MS = 10000;

// Forward declarations
void runModemTests();
void runCellularTests();
void buttonHandler(system_event_t event, int data);

enum State {
	STARTUP_WAIT_STATE,
	MODEM_ON_STATE,
	MODEM_ON_WAIT_STATE,
	COPS_STATE,
	CONNECT_STATE,
	CONNECT_WAIT_STATE,
	RUN_CELLULAR_TESTS,
	CLOUD_BUTTON_WAIT_STATE,
	CLOUD_CONNECT_STATE,
	CLOUD_CONNECT_WAIT_STATE,
	CLOUD_PRE_WAIT_STATE,
	RUN_CLOUD_TESTS,
	DISCONNECT_WAIT_STATE,
	REPEAT_TESTS_STATE,
	DONE_STATE,
	IDLE_WAIT_STATE
};
State state = STARTUP_WAIT_STATE;
unsigned long stateTime = 0;
bool buttonClicked = false;

void setup() {
	Serial.begin(9600);
	System.on(button_click, buttonHandler);
}

void loop() {
	switch(state) {
	case STARTUP_WAIT_STATE:
		if (millis() - stateTime >= STARTUP_WAIT_TIME_MS) {
			stateTime = millis();
			state = MODEM_ON_STATE;
		}
		break;

	case MODEM_ON_STATE:
		buttonClicked = false;
		Log.info("turning on modem...");
		Cellular.on();
		state = MODEM_ON_WAIT_STATE;
		stateTime = millis();
		break;

	case MODEM_ON_WAIT_STATE:
		// In system threaded mode you need to wait a little while after turning on the
		// cellular mode before it will respond. That's what MODEM_ON_WAIT_TIME_MS is for.
		if (millis() - stateTime < MODEM_ON_WAIT_TIME_MS) {
			// Not time yet; with system thread enabled you need to wait a few
			// seconds for the modem to actually start up before it will respond.
			break;
		}

		// Modem ready. Print out the basic stuff like ICCID.
		runModemTests();

		state = CONNECT_STATE;
		stateTime = millis();
		break;

	case COPS_STATE:
		break;

	case CONNECT_STATE:

		Log.info("attempting to connect to the cellular network...");
		Cellular.connect();

		state = CONNECT_WAIT_STATE;
		stateTime = millis();
		break;

	case CONNECT_WAIT_STATE:
		if (Cellular.ready()) {
			unsigned long elapsed = millis() - stateTime;

			Log.info("connected to the cellular network in %lu milliseconds", elapsed);
			state = RUN_CELLULAR_TESTS;
			stateTime = millis();
		}
		else
		if (Cellular.listening()) {
			Log.info("entered listening mode (blinking dark blue) - probably no SIM installed");
			state = DONE_STATE;
		}
		break;

	case RUN_CELLULAR_TESTS:
		Log.info("running cellular tests");

		runCellularTests();

		Log.info("cellular tests complete");
		Log.info("press the MODE button to connect to the cloud");
		state = CLOUD_BUTTON_WAIT_STATE;
		break;

	case CLOUD_BUTTON_WAIT_STATE:
		if (buttonClicked) {
			buttonClicked = false;
			state = CLOUD_CONNECT_STATE;
		}
		break;

	case CLOUD_CONNECT_STATE:
		Log.info("attempting to connect to the Particle cloud...");
		Particle.connect();
		state = CLOUD_CONNECT_WAIT_STATE;
		stateTime = millis();
		break;

	case CLOUD_CONNECT_WAIT_STATE:
		if (Particle.connected()) {
			unsigned long elapsed = millis() - stateTime;

			Log.info("connected to the cloud in %lu milliseconds", elapsed);


			state = CLOUD_PRE_WAIT_STATE;
			stateTime = millis();
		}
		break;

	case CLOUD_PRE_WAIT_STATE:
		if (millis() - stateTime >= CLOUD_PRE_WAIT_TIME_MS) {
			state = RUN_CLOUD_TESTS;
			stateTime = millis();
		}
		break;

	case RUN_CLOUD_TESTS:
		// Log.info("running cloud tests");
		state = DONE_STATE;
		break;

	case DISCONNECT_WAIT_STATE:
		if (millis() - stateTime >= DISCONNECT_WAIT_TIME_MS) {
			state = REPEAT_TESTS_STATE;
		}
		break;

	case REPEAT_TESTS_STATE:
		if (Cellular.ready()) {
			state = RUN_CELLULAR_TESTS;
		}
		else {
			state = CONNECT_STATE;
		}
		break;

	case DONE_STATE:
		Log.info("tests complete!");
		Log.info("press the MODE button to disconnect from the cloud and repeat tests");
		buttonClicked = false;
		state = IDLE_WAIT_STATE;
		break;


	case IDLE_WAIT_STATE:
		if (buttonClicked) {
			buttonClicked = false;
			if (Particle.connected()) {
				Log.info("disconnecting from the cloud");
				Particle.disconnect();
				state = DISCONNECT_WAIT_STATE;
				stateTime = millis();
			}
			else {
				state = REPEAT_TESTS_STATE;
			}
		}
		break;
	}
}

void runModemTests() {
	Log.info("manufacturer=%s", CellularHelper.getManufacturer().c_str());

	Log.info("model=%s", CellularHelper.getModel().c_str());

	Log.info("firmware version=%s", CellularHelper.getFirmwareVersion().c_str());

	Log.info("ordering code=%s", CellularHelper.getOrderingCode().c_str());

	Log.info("IMEI=%s", CellularHelper.getIMEI().c_str());

	Log.info("IMSI=%s", CellularHelper.getIMSI().c_str());

	Log.info("ICCID=%s", CellularHelper.getICCID().c_str());

}

// Various tests to find out information about the cellular network we connected to
void runCellularTests() {

	Log.info("operator name=%s", CellularHelper.getOperatorName().c_str());

	CellularHelperRSSIQualResponse rssiQual = CellularHelper.getRSSIQual();
	int bars = CellularHelperClass::rssiToBars(rssiQual.rssi);

	Log.info("rssi=%d, qual=%d, bars=%d", rssiQual.rssi, rssiQual.qual, bars);

	// First try to get info on neighboring cells. This doesn't work for me using the U260.
	// Get a maximum of 8 responses
	CellularHelperEnvironmentResponseStatic<8> envResp;

	CellularHelper.getEnvironment(CellularHelper.ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS, envResp);
	if (envResp.resp != RESP_OK) {
		// We couldn't get neighboring cells, so try just the receiving cell
		CellularHelper.getEnvironment(CellularHelper.ENVIRONMENT_SERVING_CELL, envResp);
	}
	envResp.logResponse();


	CellularHelperLocationResponse locResp = CellularHelper.getLocation();
	Log.info(locResp.toString());

	Log.info("ping 8.8.8.8=%d", CellularHelper.ping("8.8.8.8"));

	Log.info("dns device.spark.io=%s", CellularHelper.dnsLookup("device.spark.io").toString().c_str());
}

void buttonHandler(system_event_t event, int param) {
	// int clicks = system_button_clicks(param);
	buttonClicked = true;
}


