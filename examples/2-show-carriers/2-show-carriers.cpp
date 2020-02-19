#include "Particle.h"
#include "CellularHelper.h"

// STARTUP(cellular_credentials_set("epc.tmobile.com", "", "", NULL));

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

const unsigned long STARTUP_WAIT_TIME_MS = 4000;
const unsigned long MODEM_ON_WAIT_TIME_MS = 4000;

// Forward declarations
void cellularScan();
void buttonHandler(system_event_t event, int data);

enum State {
	STARTUP_WAIT_STATE,
	MODEM_ON_STATE,
	MODEM_ON_WAIT_STATE,
	RUN_TEST_STATE,
	COPS_STATE,
	DONE_STATE,
	IDLE_WAIT_STATE
};
State state = STARTUP_WAIT_STATE;
unsigned long stateTime = 0;
bool buttonClicked = false;
CellularHelperEnvironmentResponseStatic<32> envResp;

void setup() {
	Serial.begin(9600);
	System.on(button_click, buttonHandler);
}

void loop() {
	switch(state) {
	case STARTUP_WAIT_STATE:
		// This delay is to give you time to connect to the serial port
		if (millis() - stateTime >= STARTUP_WAIT_TIME_MS) {
			stateTime = millis();
			state = MODEM_ON_STATE;
		}
		break;

	case MODEM_ON_STATE:
		buttonClicked = false;
		Serial.println("turning on modem...");
		Cellular.on();
		state = MODEM_ON_WAIT_STATE;
		stateTime = millis();
		break;

	case MODEM_ON_WAIT_STATE:
		// In system threaded mode you need to wait a little while after turning on the
		// cellular mode before it will respond. That's what MODEM_ON_WAIT_TIME_MS is for.
		if (millis() - stateTime >= MODEM_ON_WAIT_TIME_MS) {
			state = RUN_TEST_STATE;
			stateTime = millis();
			break;
		}
		break;

	case RUN_TEST_STATE:
		cellularScan();
		state = DONE_STATE;
		break;

	case COPS_STATE:
		break;

	case DONE_STATE:
		Serial.println("tests complete!");
		Serial.println("press the MODE button to repeat test");
		buttonClicked = false;
		state = IDLE_WAIT_STATE;
		break;

	case IDLE_WAIT_STATE:
		if (buttonClicked) {
			buttonClicked = false;
			state = RUN_TEST_STATE;
		}
		break;
	}
}

class OperatorName {
public:
	int mcc;
	int mnc;
	String name;
};

class CellularHelperCOPNResponse : public CellularHelperCommonResponse {
public:
	CellularHelperCOPNResponse();

	void requestOperator(CellularHelperEnvironmentCellData *data);
	void requestOperator(int mcc, int mnc);
	void checkOperator(char *buf);
	const char *getOperatorName(int mcc, int mnc) const;

	virtual int parse(int type, const char *buf, int len);

	// Maximum number of operator names that can be looked up
	static const size_t MAX_OPERATORS = 16;

private:
	size_t numOperators;
	OperatorName operators[MAX_OPERATORS];
};
CellularHelperCOPNResponse copnResp;


CellularHelperCOPNResponse::CellularHelperCOPNResponse() : numOperators(0) {

}

void CellularHelperCOPNResponse::requestOperator(CellularHelperEnvironmentCellData *data) {
	if (data && data->isValid(true)) {
		requestOperator(data->mcc, data->mnc);
	}
}

void CellularHelperCOPNResponse::requestOperator(int mcc, int mnc) {
	for(size_t ii = 0; ii < numOperators; ii++) {
		if (operators[ii].mcc == mcc && operators[ii].mnc == mnc) {
			// Already requested
			return;
		}
	}
	if (numOperators < MAX_OPERATORS) {
		// There is room to request another
		operators[numOperators].mcc = mcc;
		operators[numOperators].mnc = mnc;
		numOperators++;
	}
}

const char *CellularHelperCOPNResponse::getOperatorName(int mcc, int mnc) const {
	for(size_t ii = 0; ii < numOperators; ii++) {
		if (operators[ii].mcc == mcc && operators[ii].mnc == mnc) {
			return operators[ii].name.c_str();
		}
	}
	return "unknown";
}


void CellularHelperCOPNResponse::checkOperator(char *buf) {
	if (buf[0] == '"') {
		char *numStart = &buf[1];
		char *numEnd = strchr(numStart, '"');
		if (numEnd && ((numEnd - numStart) == 6)) {
			char temp[4];
			temp[3] = 0;
			strncpy(temp, numStart, 3);
			int mcc = atoi(temp);

			strncpy(temp, numStart + 3, 3);
			int mnc = atoi(temp);

			*numEnd = 0;
			char *nameStart = strchr(numEnd + 1, '"');
			if (nameStart) {
				nameStart++;
				char *nameEnd = strchr(nameStart, '"');
				if (nameEnd) {
					*nameEnd = 0;

					// Log.info("mcc=%d mnc=%d name=%s", mcc, mnc, nameStart);

					for(size_t ii = 0; ii < numOperators; ii++) {
						if (operators[ii].mcc == mcc && operators[ii].mnc == mnc) {
							operators[ii].name = String(nameStart);
						}
					}
				}
			}
		}
	}
}


int CellularHelperCOPNResponse::parse(int type, const char *buf, int len) {
	if (enableDebug) {
		logCellularDebug(type, buf, len);
	}
	if (type == TYPE_PLUS) {
		// Copy to temporary string to make processing easier
		char *copy = (char *) malloc(len + 1);
		if (copy) {
			strncpy(copy, buf, len);
			copy[len] = 0;

			/*
			 	0000018684 [app] INFO: +COPN: "901012","MCP Maritime Com"\r\n
				0000018694 [app] INFO: cellular response type=TYPE_PLUS len=28
				0000018694 [app] INFO: \r\n
				0000018695 [app] INFO: +COPN: "901021","Seanet"\r\n
				0000018705 [app] INFO: cellular response type=TYPE_ERROR len=39
				0000018705 [app] INFO: \r\n
			 *
			 */

			char *start = strstr(copy, "\n+COPN: ");
			if (start) {
				start += 8; // length of COPN string

				char *end = strchr(start, '\r');
				if (end) {
					*end = 0;

					checkOperator(start);
				}
			}

			free(copy);
		}
	}
	return WAIT;
}

void printCellData(CellularHelperEnvironmentCellData *data) {
	const char *whichG = data->isUMTS ? "3G" : "2G";

	// Log.info("mcc=%d mnc=%d", data->mcc, data->mnc);

	const char *operatorName = copnResp.getOperatorName(data->mcc, data->mnc);

	Serial.printlnf("%s %s %s %d bars (%03d%03d)", whichG, operatorName, data->getBandString().c_str(), data->getBars(), data->mcc, data->mnc);
}

void cellularScan() {
	Log.info("starting cellular scan...");

	// envResp.enableDebug = true;
	envResp.clear();

	// Command may take up to 3 minutes to execute!
	envResp.resp = Cellular.command(CellularHelperClass::responseCallback, (void *)&envResp, 360000, "AT+COPS=5\r\n");
	if (envResp.resp == RESP_OK) {
		envResp.logResponse();

		copnResp.requestOperator(&envResp.service);
		if (envResp.neighbors) {
			for(size_t ii = 0; ii < envResp.numNeighbors; ii++) {
				copnResp.requestOperator(&envResp.neighbors[ii]);
			}
		}
	}


	Log.info("looking up operator names...");

	copnResp.enableDebug = false;
	copnResp.resp = Cellular.command(CellularHelperClass::responseCallback, (void *)&copnResp, 120000, "AT+COPN\r\n");

	Log.info("results...");

	printCellData(&envResp.service);
	if (envResp.neighbors) {
		for(size_t ii = 0; ii < envResp.numNeighbors; ii++) {
			if (envResp.neighbors[ii].isValid(true /* ignoreCI */)) {
				printCellData(&envResp.neighbors[ii]);
			}
		}
	}

}


void buttonHandler(system_event_t event, int param) {
	// int clicks = system_button_clicks(param);
	buttonClicked = true;
}
