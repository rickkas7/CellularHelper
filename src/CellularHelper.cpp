#include "Particle.h"

#include "CellularHelper.h"

// This check is here so it can be a library dependency for a library that's compiled for both
// cellular and Wi-Fi.
#if Wiring_Cellular


CellularHelperClass CellularHelper;

void CellularHelperCommonResponse::logCellularDebug(int type, const char *buf, int len) const {
	String typeStr;
	switch(type) {
	case TYPE_UNKNOWN:
		typeStr = "TYPE_UNKNOWN";
		break;

	case TYPE_OK:
		typeStr = "TYPE_OK";
		break;

	case TYPE_ERROR:
		typeStr = "TYPE_ERROR";
		break;

	case TYPE_RING:
		typeStr = "TYPE_ERROR";
		break;

	case TYPE_CONNECT:
		typeStr = "TYPE_CONNECT";
		break;

	case TYPE_NOCARRIER:
		typeStr = "TYPE_NOCARRIER";
		break;

	case TYPE_NODIALTONE:
		typeStr = "TYPE_NODIALTONE";
		break;

	case TYPE_BUSY:
		typeStr = "TYPE_BUSY";
		break;

	case TYPE_NOANSWER:
		typeStr = "TYPE_NOANSWER";
		break;

	case TYPE_PROMPT:
		typeStr = "TYPE_PROMPT";
		break;

	case TYPE_PLUS:
		typeStr = "TYPE_PLUS";
		break;

	case TYPE_TEXT:
		typeStr = "TYPE_PLUS";
		break;

	case TYPE_ABORTED:
		typeStr = "TYPE_ABORTED";
		break;

	default:
		typeStr = String::format("type=0x%x", type);
		break;
	}

	Log.info("cellular response type=%s len=%d", typeStr.c_str(), len);
	String out;
	for(int ii = 0; ii < len; ii++) {
		if (buf[ii] == '\n') {
			out += "\\n";
			Log.info(out);
			out = "";
		}
		else
		if (buf[ii] == '\r') {
			out += "\\r";
		}
		else
		if (buf[ii] < ' ' || buf[ii] >= 127) {
			char hex[10];
			snprintf(hex, sizeof(hex), "0x%02x", buf[ii]);
			out.concat(hex);
		}
		else {
			out.concat(buf[ii]);
		}
	}
	if (out.length() > 0) {
		Log.info(out);
	}
}




int CellularHelperStringResponse::parse(int type, const char *buf, int len) {
	if (enableDebug) {
		logCellularDebug(type, buf, len);
	}
	if (type == TYPE_UNKNOWN) {
		CellularHelperClass::appendBufferToString(string, buf, len, true);
	}
	return WAIT;
}


int CellularHelperPlusStringResponse::parse(int type, const char *buf, int len) {
	if (enableDebug) {
		logCellularDebug(type, buf, len);
	}
	if (type == TYPE_PLUS) {
		// Copy to temporary string to make processing easier
		char *copy = (char *) malloc(len + 1);
		if (copy) {
			strncpy(copy, buf, len);
			copy[len] = 0;

			// We return the parts of the + response corresponding to the command we requested
			char searchFor[32];
			snprintf(searchFor, sizeof(searchFor), "\n+%s: ", command.c_str());

			//Log.info("searching for: +%s:", command.c_str());

			char *start = strstr(copy, searchFor);
			if (start) {
				start += strlen(searchFor);

				char *end = strchr(start, '\r');
				CellularHelperClass::appendBufferToString(string, start, end - start);
				//Log.info("found %s", string.c_str());
			}
			else {
				//Log.info("not found");
			}

			free(copy);
		}
	}
	return WAIT;
}

String CellularHelperPlusStringResponse::getDoubleQuotedPart(bool onlyFirst) const {
	String result;
	bool inQuoted = false;

	result.reserve(string.length());

	for(size_t ii = 0; ii < string.length(); ii++) {
		char ch = string.charAt(ii);
		if (ch == '"') {
			inQuoted = !inQuoted;
			if (!inQuoted && onlyFirst) {
				break;
			}
		}
		else {
			if (inQuoted) {
				result.concat(ch);
			}
		}
	}

	return result;
}


void CellularHelperRSSIQualResponse::postProcess() {
	if (sscanf(string.c_str(), "%d,%d", &rssi, &qual) == 2) {

		// The range is the following:
		// 0: -113 dBm or less
		// 1: -111 dBm
		// 2..30: from -109 to -53 dBm with 2 dBm steps
		// 31: -51 dBm or greater
		// 99: not known or not detectable or currently not available
		if (rssi < 99) {
			rssi = -113 + (rssi * 2);
		}
		else {
			rssi = 0;
		}

		resp = RESP_OK;
	}
	else {
		// Failed to parse result
		resp = RESP_ERROR;
	}
}

String CellularHelperRSSIQualResponse::toString() const {
	
	return String::format("rssi=%d qual=%d", rssi, qual);
}


void CellularHelperExtendedQualResponse::postProcess() {
	int values[6];

	if (sscanf(string.c_str(), "%d,%d,%d,%d,%d,%d", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {

		rxlev = (uint8_t) values[0];
		ber = (uint8_t) values[1];
		rscp = (uint8_t) values[2];
		ecn0 = (uint8_t) values[3];
		rsrq = (uint8_t) values[4];
		rsrp = (uint8_t) values[5];
		

		resp = RESP_OK;
	}
	else {
		// Failed to parse result
		resp = RESP_ERROR;
	}
}

String CellularHelperExtendedQualResponse::toString() const {
	
	return String::format("rxlev=%d ber=%d rscp=%d ecn0=%d rsrq=%d rsrp=%d", (int)rxlev, (int)ber, (int)rscp, (int)ecn0, (int)rsrq, (int)rsrp);
}


CellularHelperEnvironmentResponse::CellularHelperEnvironmentResponse() :neighbors(0), numNeighbors(0) {
}

CellularHelperEnvironmentResponse::CellularHelperEnvironmentResponse(CellularHelperEnvironmentCellData *neighbors, size_t numNeighbors) :
	neighbors(neighbors), numNeighbors(numNeighbors) {


}

int CellularHelperEnvironmentResponse::parse(int type, const char *buf, int len) {
	if (enableDebug) {
		logCellularDebug(type, buf, len);
	}

	if (type == TYPE_UNKNOWN || type == TYPE_PLUS) {
		// We get this for AT+CGED=5
		// Copy to temporary string to make processing easier
		char *copy = (char *) malloc(len + 1);
		if (copy) {
			strncpy(copy, buf, len);
			copy[len] = 0;

			// This is used for skipping over the +CGED: part of the response
			char searchFor[32];
			size_t searchForLen = snprintf(searchFor, sizeof(searchFor), "+%s: ", command.c_str());

			char *endStr;

			char *line = strtok_r(copy, "\r\n", &endStr);
			while(line) {
				if (line[0]) {
					// Not an empty line

					if (type == TYPE_PLUS && strncmp(line, searchFor, searchForLen) == 0) {
						line += searchForLen;
					}

					if (strncmp(line, "MCC:", 4) == 0) {
						// Line begins with MCC:
						// This happens for 2G and 3G
						if (curDataIndex < 0) {
							service.parse(line);
							curDataIndex++;
						}
						else
						if (neighbors && (size_t)curDataIndex < numNeighbors) {
							neighbors[curDataIndex++].parse(line);
						}
					}
					else
					if (strncmp(line, "RAT:", 4) == 0) {
						// Line begins with RAT:
						// This happens for 3G in the + response so you know whether
						// the response is for a 2G or 3G tower
						service.parse(line);
					}
				}
				line = strtok_r(NULL, "\r\n", &endStr);
			}

			free(copy);
		}
	}
	return WAIT;
}

void CellularHelperEnvironmentCellData::parse(const char *str) {
	char *mutableCopy = strdup(str);

	char *endStr;

	char *pair = strtok_r(mutableCopy, ",", &endStr);
	while(pair) {
		// Remove leading spaces caused by ", " combination
		while(*pair == ' ') {
			pair++;
		}

		char *colon = strchr(pair, ':');
		if (colon != NULL) {
			*colon = 0;
			const char *key = pair;
			const char *value = ++colon;

			addKeyValue(key, value);
		}

		pair = strtok_r(NULL, ",", &endStr);
	}


	free(mutableCopy);
}

bool CellularHelperEnvironmentCellData::isValid(bool ignoreCI) const {

	if (mcc > 999) {
		return false;
	}

	if (!ignoreCI) {
		if (isUMTS) {
			if (ci >= 0xfffffff) {
				return false;
			}
		}
		else {
			if (ci >= 0xffff) {
				return false;
			}
		}
	}
	return true;
}


void CellularHelperEnvironmentCellData::addKeyValue(const char *key, const char *value) {
	char ucCopy[16];
	if (strlen(key) > (sizeof(ucCopy) - 1)) {
		Log.info("key too long key=%s value=%s", key, value);
		return;
	}
	size_t ii = 0;
	for(; key[ii]; ii++) {
		ucCopy[ii] = toupper(key[ii]);
	}
	ucCopy[ii] = 0;

	if (strcmp(ucCopy, "RAT") == 0) {
		isUMTS = (strstr(value, "UMTS") != NULL);
	}
	else
	if (strcmp(ucCopy, "MCC") == 0) {
		mcc = atoi(value);
	}
	else
	if (strcmp(ucCopy, "MNC") == 0) {
		mnc = atoi(value);
	}
	else
	if (strcmp(ucCopy, "LAC") == 0) {
		lac = (int) strtol(value, NULL, 16); // hex
	}
	else
	if (strcmp(ucCopy, "CI") == 0) {
		ci = (int) strtol(value, NULL, 16); // hex
	}
	else
	if (strcmp(ucCopy, "BSIC") == 0) {
		bsic = (int) strtol(value, NULL, 16); // hex
	}
	else
	if (strcmp(ucCopy, "ARFCN") == 0) { // Usually "Arfcn"
		// Documentation says this is hex, but this does not appear to be the case!
		// arfcn = (int) strtol(value, NULL, 16); // hex
		arfcn = atoi(value);
	}
	else
	if (strcmp(ucCopy, "ARFCN_DED") == 0 || strcmp(ucCopy, "RXLEVSUB") == 0 || strcmp(ucCopy, "T_ADV") == 0) {
		// Ignored 2G fields: Arfcn_ded, RxLevSub, t_adv
	}
	else
	if (strcmp(ucCopy, "RXLEV") == 0) { // Sometimes RxLev
		rxlev = (int) strtol(value, NULL, 16); // hex
	}
	else
	if (strcmp(ucCopy, "DLF") == 0) {
		dlf = atoi(value);
	}
	else
	if (strcmp(ucCopy, "ULF") == 0) {
		ulf = atoi(value);

		// For AT+COPS=5, we don't get a RAT, but if ULF is present it's 3G
		isUMTS = true;
	}
	else
	if (strcmp(ucCopy, "RSCP LEV") == 0) {
		rscpLev = atoi(value);
	}
	else
	if (strcmp(ucCopy, "RAC") == 0 || strcmp(ucCopy, "SC") == 0 || strcmp(ucCopy, "ECN0 LEV") == 0) {
		// We get these with AT+COPS=5, but we don't need the values
	}
	else {
		Log.info("unknown key=%s value=%s", key, value);
	}

}

int CellularHelperEnvironmentCellData::getBand() const {
	int freq = 0;

	if (isUMTS) {
		// 3G radio

		// There are a bunch of special cases:
		switch(ulf) {
		case 12:
		case 37:
		case 62:
		case 87:
		case 112:
		case 137:
		case 162:
		case 187:
		case 212:
		case 237:
		case 262:
		case 287:
			freq = 1900; // PCS A-F
			break;

		case 1662:
		case 1687:
		case 1712:
		case 1737:
		case 1762:
		case 1787:
		case 1812:
		case 1837:
		case 1862:
			freq = 1700; // AWS A-F
			break;

		case 782:
		case 787:
		case 807:
		case 812:
		case 837:
		case 862:
			freq = 850; // CLR
			break;

		case 2362:
		case 2387:
		case 2412:
		case 2437:
		case 2462:
		case 2487:
		case 2512:
		case 2537:
		case 2562:
		case 2587:
		case 2612:
		case 2637:
		case 2662:
		case 2687:
			freq = 2600; // IMT-E
			break;

		case 3187:
		case 3212:
		case 3237:
		case 3262:
		case 3287:
		case 3312:
		case 3337:
		case 3362:
		case 3387:
		case 3412:
		case 3437:
		case 3462:
			freq = 1700; // EAWS A-G
			break;

		case 3707:
		case 3732:
		case 3737:
		case 3762:
		case 3767:
			freq = 700; // LSMH A/B/C
			break;

		case 3842:
		case 3867:
			freq = 700; // USMH C
			break;

		case 3942:
		case 3967:
			freq = 700; // USMH D

		case 387:
		case 412:
		case 437:
			freq = 800;
			break;

		case 6067:
		case 6092:
		case 6117:
		case 6142:
		case 6167:
		case 6192:
		case 6217:
		case 6242:
		case 6267:
		case 6292:
		case 6317:
		case 6342:
		case 6367:
			freq = 1900; // EPCS A-G
			break;

		case 5712:
		case 5737:
		case 5762:
		case 5767:
		case 5787:
		case 5792:
		case 5812:
		case 5817:
		case 5837:
		case 5842:
		case 5862:
			freq = 850; // ECLR
			break;

		default:
			if (ulf >= 0 && ulf <= 124) {
				freq = 900;
			}
			else
			if (ulf >= 128 && ulf <= 251) {
				freq = 850;
			}
			else
			if (ulf >= 512 && ulf <= 885) {
				freq = 1800;
			}
			else
			if (ulf >= 975 && ulf <= 1023) {
				freq = 900;
			}
			else
			if (ulf >= 1312 && ulf <= 1513) {
				freq = 1700;
			}
			else
			if (ulf >= 2712 && ulf <= 2863) {
				freq = 900;
			}
			else
			if (ulf >= 4132 && ulf <= 4233) {
				freq = 850;
			}
			else
			if ((ulf >= 4162 && ulf <= 4188) || (ulf >= 20312 && ulf <= 20363)) {
				freq = 800;
			}
			else
			if (ulf >= 9262 && ulf <= 9538) {
				freq = 1900;
			}
			else
			if (ulf >= 9612 && ulf <= 9888) {
				freq = 2100;
			}
			break;
		}


	}
	else {
		// 2G, use arfcn
		if (arfcn >= 0 && arfcn <= 124) {
			freq = 900;
		}
		else
		if (arfcn >= 128 && arfcn <= 251) {
			freq = 850;
		}
		else
		if (arfcn >= 512 && arfcn <= 885) {
			freq = 1800;
		}
		else
		if (arfcn >= 975 && arfcn <= 1023) {
			freq = 900;
		}
	}
	return freq;
}

// Calculated
String CellularHelperEnvironmentCellData::getBandString() const {
	String band;

	int freq = getBand();

	if (isUMTS) {
		// 3G radio
		if ((ulf >= 0 && ulf <= 124) ||
			(ulf >= 128 && ulf <= 251)) {
			band = "GSM " + String(freq);
		}
		else
		if (ulf >= 512 && ulf <= 885) {
			band = "DCS 1800";
		}
		else
		if (ulf >= 975 && ulf <= 1023) {
			band = "ESGM 900";
		}
		else
		if (freq != 0) {
			band = "UMTS " + String(freq);
		}
		else {
			band = "3G unknown";
		}
	}
	else {
		// 2G, use arfcn

		if (arfcn >= 512 && arfcn <= 885) {
			band = "DCS 1800 or 1900";
		}
		else
		if (arfcn >= 975 && arfcn <= 1024) {
			band = "EGSM 900";
		}
		else
		if (freq != 0) {
			band = "GSM " + String(freq);
		}
		else {
			band = "2G unknown";
		}
	}


	return band;
}

int CellularHelperEnvironmentCellData::getRSSI() const {
	int rssi = 0;

	if (isUMTS) {
		// 3G radio

		if (rscpLev <= 96) {
			rssi = -121 + rscpLev;
		}
	}
	else {
		// 2G
		if (rxlev <= 96) {
			rssi = -121 + rxlev;
		}
	}
	return rssi;
}

int CellularHelperEnvironmentCellData::getBars() const {
	return CellularHelperClass::rssiToBars(getRSSI());
}

String CellularHelperEnvironmentCellData::toString() const {
	String common = String::format("mcc=%d, mnc=%d, lac=%x ci=%x band=%s rssi=%d",
			mcc, mnc, lac, ci, getBandString().c_str(), getRSSI());

	if (isUMTS) {
		return String::format("rat=UMTS %s dlf=%d ulf=%d", common.c_str(), dlf, ulf);
	}
	else {
		return String::format("rat=GSM %s bsic=%x arfcn=%d rxlev=%d", common.c_str(), bsic, arfcn, rxlev);
	}
}

void CellularHelperEnvironmentResponse::clear() {
	curDataIndex = -1;
}


void CellularHelperEnvironmentResponse::logResponse() const {
	Log.info("service %s", service.toString().c_str());
	if (neighbors) {
		for(size_t ii = 0; ii < numNeighbors; ii++) {
			if (neighbors[ii].isValid(true /* ignoreCI */)) {
				Log.info("neighbor %d %s", ii, neighbors[ii].toString().c_str());
			}
		}
	}
}

size_t CellularHelperEnvironmentResponse::getNumNeighbors() const {
	if (curDataIndex < 0) {
		return 0;
	}
	else {
		if (neighbors) {
			for(size_t ii = 0; ii < (size_t)curDataIndex; ii++) {
				if (!neighbors[ii].isValid()) {
					return ii;
				}
			}
		}
		return curDataIndex;
	}
}



// +UULOC: <date>,<time>,<lat>,<long>,<alt>,<uncertainty>

void CellularHelperLocationResponse::postProcess() {
	char *mutableCopy = strdup(string.c_str());
	if (mutableCopy) {
		char *part, *endStr;

		part = strtok_r(mutableCopy, ",", &endStr);
		if (part) {
			// part is date
			part = strtok_r(NULL, ",", &endStr);
			if (part) {
				// part is time
				part = strtok_r(NULL, ",", &endStr);
				if (part) {
					// part is lat
					lat = atof(part);

					part = strtok_r(NULL, ",", &endStr);
					if (part) {
						// part is lon
						lon = atof(part);

						part = strtok_r(NULL, ",", &endStr);
						if (part) {
							// part is alt
							alt = atoi(part);

							part = strtok_r(NULL, ",", &endStr);
							if (part) {
								// part is uncertainty
								uncertainty = atoi(part);
								valid = true;
								resp = RESP_OK;
							}
						}
					}
				}
			}
		}

		free(mutableCopy);
	}
}

String CellularHelperLocationResponse::toString() const {
	if (valid) {
		return String::format("lat=%f lon=%f alt=%d uncertainty=%d", lat, lon, alt, uncertainty);
	}
	else {
		return "valid=false";
	}
}

void CellularHelperCREGResponse::postProcess() {
	// "\r\n+CREG: 2,1,\"FFFE\",\"C45C010\",8\r\n"
	int n;

	if (sscanf(string.c_str(), "%d,%d,\"%x\",\"%x\",%d", &n, &stat, &lac, &ci, &rat) == 5) {
		// SARA-R4 does include the n (5 parameters)
		valid = true;
	}
	else
	if (sscanf(string.c_str(), "%d,\"%x\",\"%x\",%d", &stat, &lac, &ci, &rat) == 4) {
		// SARA-U and SARA-G don't include the n (4 parameters)
		valid = true;
	}

}

String CellularHelperCREGResponse::toString() const {
	if (valid) {
		return String::format("stat=%d lac=0x%x ci=0x%x rat=%d", stat, lac, ci, rat);
	}
	else {
		return "valid=false";
	}
}


String CellularHelperClass::getManufacturer() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGMI\r\n");

	return resp.string;
}

String CellularHelperClass::getModel() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGMM\r\n");

	return resp.string;
}

String CellularHelperClass::getOrderingCode() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "ATI0\r\n");

	return resp.string;
}

String CellularHelperClass::getFirmwareVersion() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGMR\r\n");

	return resp.string;
}

String CellularHelperClass::getIMEI() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGSN\r\n");

	return resp.string;
}

String CellularHelperClass::getIMSI() const {
	CellularHelperStringResponse resp;

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGMI\r\n");

	return resp.string;
}

String CellularHelperClass::getICCID() const {
	CellularHelperPlusStringResponse resp;
	resp.command = "CCID";

	Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CCID\r\n");

	return resp.string;
}

bool CellularHelperClass::isSARA_R4() const {
	return getModel().startsWith("SARA-R4");
}


String CellularHelperClass::getOperatorName(int operatorNameType) const {
	String result;

	// The default is OPERATOR_NAME_LONG_EONS (9).
	// If the EONS name is not available, then the other things tried in order are:
	// NITZ, CPHS, ROM
	// So basically, something will be returned

	CellularHelperPlusStringResponse resp;
	resp.command = "UDOPN";

	int respCode = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+UDOPN=%d\r\n", operatorNameType);

	if (respCode == RESP_OK) {
		result = resp.getDoubleQuotedPart();
	}

	return result;
}

/**
 * Get the RSSI and qual values for the receiving cell site.
 *
 * The qual value is always 99 for me on the G350 (2G) and LTE-M1
 */
CellularHelperRSSIQualResponse CellularHelperClass::getRSSIQual() const {
	CellularHelperRSSIQualResponse resp;
	resp.command = "CSQ";

	resp.resp = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CSQ\r\n");

	if (resp.resp == RESP_OK) {
		resp.postProcess();
	}

	return resp;
}

CellularHelperExtendedQualResponse CellularHelperClass::getExtendedQual() const {
	CellularHelperExtendedQualResponse resp;
	resp.command = "CESQ";

	resp.resp = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CESQ\r\n");

	if (resp.resp == RESP_OK) {
		resp.postProcess();
	}

	return resp;
}


bool CellularHelperClass::selectOperator(const char *mccMnc) const {
	CellularHelperStringResponse resp;

	int respCode;

	if (mccMnc == NULL) {
		// Reset back to automatic mode
		respCode = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+COPS=0\r\n");
		return (respCode == RESP_OK);
	}

	String curMccMnc = CellularHelper.getOperatorName(0); // 0 = MCC/MNC
	if (strcmp(mccMnc, curMccMnc.c_str()) == 0) {
		// Operator already selected; nothing to do
		Log.info("operator already %s", mccMnc);
		return true;
	}

	if (curMccMnc.length() != 0) {
		// Disconnect from the current operator if there is an operator set.
		// On cold boot there won't be a name set and the string will be empty
		respCode = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+COPS=2\r\n");
	}

	// Connect
	respCode = Cellular.command(responseCallback, (void *)&resp, 60000, "AT+COPS=4,2,\"%s\"\r\n", mccMnc);

	return (respCode == RESP_OK);
}


void CellularHelperClass::getEnvironment(int mode, CellularHelperEnvironmentResponse &resp) const {
	resp.command = "CGED";
	// resp.enableDebug = true;

	resp.resp = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CGED=%d\r\n", mode);
	
}

CellularHelperLocationResponse CellularHelperClass::getLocation(unsigned long timeoutMs) const {
	CellularHelperLocationResponse resp;

	// Note: Command is ULOC, but the response is UULOC
	resp.command = "UULOC";
	// resp.enableDebug = true;

	// Initialize the mode
	resp.resp = Cellular.command(5000, "AT+ULOCCELL=0\r\n");
	if (resp.resp == RESP_OK) {
		unsigned long startTime = millis();

		resp.resp = Cellular.command(responseCallback, (void *)&resp, timeoutMs, "AT+ULOC=2,2,0,%d,5000\r\n", timeoutMs / 1000);

		// This command is weird because it returns an OK, and theoretically could return +UULOC response right away,
		// but usually does not.
		if (resp.resp == RESP_OK) {
			resp.postProcess();

			// In the case where we don't get an immediate response, we send empty commands to the
			// modem to pick up the late +UULOC response
			while(!resp.valid && millis() - startTime < timeoutMs) {
				// Allow for some cloud processing before checking again
				delay(10);

				// Have not received a response yet. Send an empty command so we can get responses that
				// come afte the OK due to the weird structure of this command
				Cellular.command(responseCallback, (void *)&resp, 500, "");
				resp.postProcess();
			}
		}
	}

	return resp;
}

void CellularHelperClass::getCREG(CellularHelperCREGResponse &resp) const {
	int tempResp;

	tempResp = Cellular.command(DEFAULT_TIMEOUT, "AT+CREG=2\r\n");
	if (tempResp == RESP_OK) {
		resp.command = "CREG";
		resp.resp = Cellular.command(responseCallback, (void *)&resp, DEFAULT_TIMEOUT, "AT+CREG?\r\n");
		if (resp.resp == RESP_OK) {
			resp.postProcess();

			// Set back to default
			tempResp = Cellular.command(DEFAULT_TIMEOUT, "AT+CREG=0\r\n");
		}
	}
}



// There isn't an overload of String that takes a buffer and length, but that's what comes back from
// the Cellular.command callback, so that's why this method exists.
// [static]
void CellularHelperClass::appendBufferToString(String &str, const char *buf, int len, bool noEOL) {
	str.reserve(str.length() + (size_t)len + 1);
	for(int ii = 0; ii < len; ii++) {
		if (!noEOL || (buf[ii] != '\r' && buf[ii] != '\n')) {
			str.concat(buf[ii]);
		}
	}
}

// [static]
int CellularHelperClass::rssiToBars(int rssi) {
	int bars = 0;

	if (rssi < 0) {
		if (rssi >= -57)      bars = 5;
		else if (rssi > -68)  bars = 4;
		else if (rssi > -80)  bars = 3;
		else if (rssi > -92)  bars = 2;
		else if (rssi > -104) bars = 1;
	}
	return bars;
}

// [static]
int CellularHelperClass::responseCallback(int type, const char* buf, int len, void *param) {
	CellularHelperCommonResponse *presp = (CellularHelperCommonResponse *)param;

	return presp->parse(type, buf, len);
}

#endif /* Wiring_Cellular */


