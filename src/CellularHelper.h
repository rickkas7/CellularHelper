#ifndef __CELLULARHELPER_H
#define __CELLULARHELPER_H

#include "Particle.h"

#define Wiring_Cellular 1 // TEMPORARY
#if Wiring_Cellular


// Class for quering information directly from the u-blox SARA modem

/**
 * @brief Common base class for response objects
 * 
 * All response objects inherit from this, so the parse() method can be called
 * in the subclass, and also the resp and enableDebug members are always available.
 */
class CellularHelperCommonResponse {
public:
	/**
	 * @brief Response code from Cellular.command
	 * 
	 * The typical return values are:
	 * - RESP_OK = -2
	 * - RESP_ERROR = -3
	 * - RESP_ABORTED = -5
	 */
	int resp = RESP_ERROR;

	/**
	 * @brief Enables debug mode (default: false)
	 * 
	 * If you set this to true in your response object class, additional debugging logs
	 * (Log.info, etc.) will be generated to help troubleshoot problems.
	 */
	bool enableDebug = false;

	/**
	 * @brief Method to parse the output from the modem
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.

	 * This is called from responseCallback, which is the callback to Cellular.command.
	 * 
	 * In the base class CellularHelperCommonResponse this is pure virtual and must be 
	 * subclassed in your concrete subclass of CellularHelperCommonResponse.
	 */
	virtual int parse(int type, const char *buf, int len) = 0;

	/**
	 * @brief Used when enableDebug is true to log the Cellular.command callback data using Log.info
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.
	 */
	void logCellularDebug(int type, const char *buf, int len) const;
};

/**
 * @brief Things that return a simple string, like the manufacturer string, use this
 *
 * Since it inherits from CellularHelperCommonResponse you
 * can check resp == RESP_OK to make sure the call succeeded.
 */
class CellularHelperStringResponse : public CellularHelperCommonResponse {
public:
	/**
	 * @brief Returned string is stored here
	 */
	String string;

	/**
	 * @brief Method to parse the output from the modem
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.
	 * 
	 * This is called from responseCallback, which is the callback to Cellular.command.
	 * 
	 * This class just appends all TYPE_UNKNOWN data into string.
	 */
	virtual int parse(int type, const char *buf, int len);
};

/**
 * @brief Things that return a + response and a string use this.
 *
 * Since it inherits from CellularHelperCommonResponse you
 * can check resp == RESP_OK to make sure the call succeeded.
 */
class CellularHelperPlusStringResponse : public CellularHelperCommonResponse {
public:
	/**
	 * @brief Your subclass must set this to the command requesting (not including the AT+ part)
	 * 
	 * Say you're implementing an AT+CSQ response handler. You would set command to "CSQ". This
	 * is because the modem will return +CSQ as the response so the parser needs to know what
	 * to look for. 
	 */
	String command;

	/**
	 * @brief Returned string is stored here
	 */
	String string;

	/**
	 * @brief Method to parse the output from the modem
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.
	 * 
	 * This is called from responseCallback, which is the callback to Cellular.command.
	 * 
	 * This class just appends all + responses (TYPE_PLUS) that match command into string.
	 */
	virtual int parse(int type, const char *buf, int len);

	/**
	 * @brief Gets the double quoted part of a string response
	 *
	 * @param onlyFirst (boolean, default true) Only the first double quoted string
	 * is returned if true. If false, all double quoted strings are concatenated,
	 * ignoring all of the parts outside of the double quotes.
	 * 
	 * Some commands, like AT+UDOPN return a + response that contains a double quoted
	 * string. If you only want to extract this string, you can use this method to 
	 * extract it.
	 *  
	 */
	String getDoubleQuotedPart(bool onlyFirst = true) const;
};

/**
 * @brief This class is used to return the rssi and qual values (AT+CSQ)
 *
 * Note that for 2G, qual is not available and 99 is always returned.
 *
 * Since it inherits from CellularHelperPlusStringResponse and CellularHelperCommonResponse you
 * can check resp == RESP_OK to make sure the call succeeded.
 * 
 * This class is the result of CellularHelper.getRSSIQual(); you normally wouldn't 
 * instantiate one of these directly.
 */
class CellularHelperRSSIQualResponse : public CellularHelperPlusStringResponse {
public:
	/**
	 * @brief RSSI Received Signal Strength Indication value
	 * 
	 * Values:
	 * - 0: -113 dBm or less
	 * - 1: -111 dBm
	 * - 2..30: from -109 to -53 dBm with 2 dBm steps
	 * - 31: -51 dBm or greater
	 * - 99: not known or not detectable or currently not available
	 */
	int rssi = 99;

	/**
	 * @brief Signal quality value
	 * 
	 * Values:
	 * - 0..7: Signal quality, 0 = good, 7 = bad
	 * - 99: not known or not detectable or currently not available
	 */
	int qual = 99;

	/**
	 * @brief Parses string and stores the results in rssi and qual
	 */
	void postProcess();

	/**
	 * @brief Converts this object into a string
	 * 
	 * The string will be of the format `rssi=-50 qual=3`. If either value is unknown, it will
	 * show as 99. 
	 */
	String toString() const;
};

/**
 * @brief Response class for the AT+CESQ command
 * 
 * This class is the result of CellularHelper.getExtendedQual(); you normally wouldn't 
 * instantiate one of these directly.
 */
class CellularHelperExtendedQualResponse : public CellularHelperPlusStringResponse {
public:
	/**
	 * @brief Received Signal Strength Indication (RSSI)
	 *
	 * Values: 
	 *   - 0: less than -110 dBm
	 *   - 1..62: from -110 to -49 dBm with 1 dBm steps
	 *   - 63: -48 dBm or greater
	 *   - 99: not known or not detectable 	
	 */
	uint8_t rxlev = 99;
	
	/**
	 * @brief Bit Error Rate (BER)
	 *
	 * Values: 
	 *   - 0..7: as the RXQUAL values described in GSM TS 05.08 [28]
	 *   - 99: not known or not detectable 
	 */
	uint8_t ber   = 99;
	
	/**
	 * @brief Received Signal Code Power (RSCP) 
	 *
	 * Values: 
	 *   - 0: -121 dBm or less
	 *   - 1..95: from -120 dBm to -24 dBm with 1 dBm steps
	 *   - 96: -25 dBm or greater
	 *   - 255: not known or not detectable
	 */
	uint8_t rscp  = 99;
	
	/**
	 * @brief Ratio of received energy per PN chip to the total received power spectral density 
	 *
	 * Values: 
	 *   - 0: -24.5 dB or less
	 *   - 1..48: from -24 dB to -0.5 dBm with 0.5 dB steps
	 *   - 49: 0 dB or greater
	 *   - 255: not known or not detectable 
	 */
	uint8_t ecn0  = 255;
	
	/**
	 * @brief Reference Signal Received Quality (RSRQ)
	 *
	 * Values: 
	 *   - 0: -19 dB or less
	 *   - 1..33: from -19.5 dB to -3.5 dB with 0.5 dB steps
	 *   - 34: -3 dB or greater
	 *   - 255: not known or not detectable Number 
	 */
	uint8_t rsrq  = 255;
	
	/**
	 * @brief Reference Signal Received Power (RSRP)
	 *
	 * Values: 
	 *   - 0: -141 dBm or less
	 *   - 1..96: from -140 dBm to -45 dBm with 1 dBm steps
	 *   - 97: -44 dBm or greater
	 *   - 255: not known or not detectable
	 */
	uint8_t rsrp  = 255;

	/**
	 * @brief Converts the data in string into the broken out fields like rxlev, ber, rsrq, etc.
	 */
	void postProcess();

	/**
	 * @brief Converts this object into a string
	 * 
	 * The string will be of the format `rxlev=99 ber=99 rscp=255 ecn0=255 rsrq=0 rsrp=37`. 
	 * Unknown values for rxlev, ber, and rscp are 99.
	 * Unknown values for ecn0, rsrq, rsrp are 255.
	 */
	String toString() const;
};


/**
 * @brief Used to hold the results for one cell (service or neighbor) from the AT+CGED command
 * 
 * You will normally use CellularHelperEnvironmentResponseStatic<> or CellularHelperEnvironmentResponse 
 * which includes this as a member.
 */
class CellularHelperEnvironmentCellData { // 44 bytes
public:
	/**
	 * @brief Mobile Country Code
	 * 
	 * - Range 0 - 999 (3 digits). Other values are to be considered invalid / not available.
	 */
	int mcc = 65535;

	/**
	 * @brief Mobile Network Code
	 * 
	 * - Range 0 - 999 (1 to 3 digits). Other values are to be considered invalid / not available.
	 */
	int mnc = 255; 

	/**
	 * @brief Location Area Code
	 * 
	 * - Range 0h-FFFFh (2 octets).
	 */
	int lac;

	/**
	 * @brief Cell Identity
	 * 
	 * - 2G cell: range 0h-FFFFh (2 octets)
	 * - 3G cell: range 0h-FFFFFFFh (28 bits)
	 */
	int ci; 

	/**
	 * @brief Base Station Identify Code
	 * 
	 * - Range 0h-3Fh (6 bits) [2G] 
	 */
	int bsic;

	/**
	 * @brief Absolute Radio Frequency Channel Number
	 * 
	 * - Range 0 - 1023 [2G only]
	 * - The parameter value also decodes the band indicator bit (DCS or PCS) by means of the
	 *   most significant byte (8 means 1900 band) (i.e. if the parameter reports the value 33485, 
	 *   it corresponds to 0x82CD, in the most significant byte there is the band indicator bit, 
	 *   so the `arfcn` is 0x2CD (717) and belongs to 1900 band).
	 */
	int arfcn; 

	/**
	 * @brief Received signal level on the cell
	 * 
	 * - Range 0 - 63; see the 3GPP TS 05.08 [2G]
	 */
	int rxlev;

	/**
	 * @brief RAT is GSM (false) or UMTS (true)
	 */ 
	bool isUMTS = false;

	/**
	 * @brief Downlink frequency. 
	 * 
	 * - Range 0 - 16383 [3G only]
	 */
	int dlf;

	/**
	 * @brief Uplink frequency. Range 0 - 16383 [3G only]
	 */
	int ulf;

	/**
	 * @brief Received signal level [3G]
	 * 
	 * Received Signal Code Power expressed in dBm levels. Range 0 - 91.
	 * 
	 * | Value | RSCP  | Note |
	 * | :---: | :---: | :--- |
	 * | 0     | RSCP < -115 dBm | Weak |
	 * | 1     | -115 <= RSCP < -114 dBm | |
	 * | 90    | -26 <= RSCP < -25 dBm | |
	 * | 91    | RSCP == -25 dBm | Strong |
	 */
	int rscpLev = 255;

	/**
	 * @brief Returns true if this object looks valid
	 *
	 * @param ignoreCI (bool, default false) Sometimes the cell identifier (CI) valid is not
	 * returned by the towers but other fields are set. Passing true in this parameter causes
	 * the CI to be ignored.
	 * 
	 * @return true if the object looks valid or false if not.
	 */
	bool isValid(bool ignoreCI = false) const;

	/**
	 * @brief Parses the output from the modem (used internally)
	 * 
	 * Some classes use postprocess() to process the + response, but the AT+CGED response
	 * is sufficiently complicated that we parse each response as it comes in using the parse()
	 * method rather than waiting until all responses have come in.
	 * 
	 * @param str The comma separated response from the modem to parse
	 */
	void parse(const char *str);

	/**
	 * @brief Add a key-value pair (used internally)
	 * 
	 * The AT+CGED response contains key-value pairs. This parses out the ones we care about and
	 * stores them in the specific fields of this structure.
	 */ 
	void addKeyValue(const char *key, const char *value);

	/**
	 * @brief Returns a readable representation of this object as a String
	 * 
	 */
	String toString() const;

	// Calculated

	/**
	 * @brief Calculated field to determine the cellular frequency band
	 * 
	 * Values are frequency in MHz, such as: 700, 800, 850, 900, 1700, 1800, 1900, 2100
	 * 
	 * Not all bands are used by existing hardware.
	 * 
	 * Note that for 2G, 1800 is returned for the 1900 MHz band. This is because they
	 * use the same arfcn values. So 1800 really means 1800 or 1900 MHz for 2G.
	 * 
	 */
	int getBand() const;

	/**
	 * @brief Returns a readable string that identifies the cellular frequency band
	 * 
	 * Example 3G: `rat=UMTS mcc=310, mnc=410, lac=1af7 ci=817b57f band=UMTS 850 rssi=0 dlf=4384 ulf=4159`
	 * 
	 * Example 2G: `rat=GSM mcc=310, mnc=260, lac=ab22 ci=a78a band=DCS 1800 or 1900 rssi=-97 bsic=23 arfcn=596 rxlev=24`
	 * 
	 */
	String getBandString() const;

	/**
	 * @brief Get the RSSI (received signal strength indication)
	 * 
	 * This only available on 3G devices, and even then is not always returned. It may be 0 or 255 if not known.
	 */
	int getRSSI() const;

	/**
	 * @brief Get the RSSI as "bars" of signal strength (0-5)
	 * 
	 * | RSSI    | Bars |
	 * | :-----: | :---: |
	 * | >= -57  | 5 |
	 * | > -68   | 4 |
	 * | > -80   | 3 |
	 * | > -92   | 2 |
	 * | > -104  | 1 |
	 * | <= -104 | 0 |
	 * 
	 * This is simlar to the bar graph display on phones.
	 */
	int getBars() const;
};

/**
 * @brief Used to hold the results from the AT+CGED command
 * 
 * You may want to use CellularHelperEnvironmentResponseStatic<> instead of separately allocating this
 * object and the array of CellularHelperEnvironmentCellData. 
 * 
 * Using this class with the default contructor is handy if you are using ENVIRONMENT_SERVING_CELL mode
 * with CellularHelper.getLocation()
 */
class CellularHelperEnvironmentResponse : public CellularHelperPlusStringResponse {
public:
	/**
	 * @brief Constructor for AT+CGED without neighbor data
	 * 
	 * Since only the Electron 2G (SARA-G350) supports getting neighbor data, this constructor can be
	 * used with ENVIRONMENT_SERVING_CELL to get only the serving cell.
	 */
	CellularHelperEnvironmentResponse();

	/**
	 * @brief Constructor that takes an external array of CellularHelperEnvironmentCellData 
	 * 
	 * @param neighbors Pointer to array of CellularHelperEnvironmentCellData. Can be NULL.
	 * 
	 * @param numNeighbors Number of items in neighbors. Can be 0.
	 * 
	 * The templated CellularHelperEnvironmentResponseStatic<> uses this constructor but eliminates the 
	 * need to separately allocate the neighbor CellularHelperEnvironmentCellData and may be easier
	 * to use. 
	 */
	CellularHelperEnvironmentResponse(CellularHelperEnvironmentCellData *neighbors, size_t numNeighbors);

	/**
	 * @brief Information about the service cell (the one you're connected to)
	 * 
	 * Filled in in both ENVIRONMENT_SERVING_CELL and ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS modes.
	 */
	CellularHelperEnvironmentCellData service;

	/**
	 * @brief Information about the neighboring cells
	 * 
	 * Only filled in when using ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS mode, which only works on the
	 * 2G Electron (SARA-G350). Maximum number of neighboring cells is 30, however it will be further
	 * limited by the numNeighbors (the size of your array). 
	 * 
	 * The value of this member is passed into the constructor.
	 */
	CellularHelperEnvironmentCellData *neighbors;
	
	/**
	 * @brief Number of entries in the neighbors array
	 * 
	 * The value of this member is passed into the constructor.
	 */
	size_t numNeighbors;

	/**
	 * @brief Current index we're writing to
	 * 
	 * - -1 = service
	 * - 0 = first element of neighbors
	 * - 1 = second element of neighbors
	 * - ...
	 */
	int curDataIndex = -1;

	/**
	 * @brief Method to parse the output from the modem
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.
	 * 
	 * This is called from responseCallback, which is the callback to Cellular.command.
	 * 
	 */
	virtual int parse(int type, const char *buf, int len);

	/**
	 * @brief Clear the data so the object can be reused
	 */
	void clear();

	/**
	 * @brief Log the decoded environment data to the debug log using Log.info 
	 */
	void logResponse() const;

	/**
	 * @brief Gets the number of neighboring cells that were returned
	 * 
	 * - 0 = no neighboring cells
	 * - 1 = one neighboring cell
	 * - ... 
	 */
	size_t getNumNeighbors() const;
};

/**
 * @brief Response class for getting cell tower information with statically defined array of neighbor cells
 * 
 * @param MAX_NEIGHBOR_CELLS templated parameter for number of neighbors to allocate. Each one is 44 bytes. Can be 0.
 * 
 * The maximum the modem supports is 30, however you can set it smaller. Beware of using values over 10 or so
 * when storing this object on the stack as you could get a stack overflow.
 */
template <size_t MAX_NEIGHBOR_CELLS>
class CellularHelperEnvironmentResponseStatic : public CellularHelperEnvironmentResponse {
public:
	explicit CellularHelperEnvironmentResponseStatic() : CellularHelperEnvironmentResponse(staticNeighbors, MAX_NEIGHBOR_CELLS) {
	}

protected:
	/**
	 * @brief Array of neighbor cell data
	 */
	CellularHelperEnvironmentCellData staticNeighbors[MAX_NEIGHBOR_CELLS];
};

/**
 * @brief Reponse class for the AT+ULOC command
 * 
 * This class is returned from CellularHelper.getLocation(). You normally won't instantiate one
 * of these directly.
 */
class CellularHelperLocationResponse : public CellularHelperPlusStringResponse {
public:
	/**
	 * @brief Set to true if the values have been set
	 */
	bool valid = false;

	/**
	 * @brief Estimated latitude, in degrees (-90 to +90)
	 */
	float lat = 0.0;

	/**
	 * @brief Estimated longitude, in degrees (-180 to +180)
	 */
	float lon = 0.0;

	/**
	 * @brief Estimated altitude, in meters. Not always returned.
	 */
	int alt = 0;
	
	/**
	 * @brief Maximum possible error, in meters (0 - 20000000)
	 */
	int uncertainty = 0;

	/**
	 * @brief Returns true if a valid location was found
	 */
	bool isValid() const { return valid; };

	/**
	 * @brief Converts the data in string into the broken out fields like lat, lon, alt, uncertainty.
	 */
	void postProcess();

	/**
	 * @brief Converts this object into a readable string
	 * 
	 * The string will be of the format `lat=12.345 lon=567.89 alt=0 uncertainty=2000`. 
	 */
	String toString() const;
};




/**
 * @brief Reponse class for the AT+CREG? command. This is deprecated and will be removed in the future.
 * 
 * This class is returned from CelluarHelper.getCREG(). You normally won't instantiate one of these
 * directly.
 */
class CellularHelperCREGResponse :  public CellularHelperPlusStringResponse {
public:
	/**
	 * @brief Set to true if the values have been set
	 */
	bool valid = false;

	/**
	 * @brief Network connection status
	 * 
	 * - 0: not registered, the MT is not currently searching a new operator to register to
	 * - 1: registered, home network
	 * - 2: not registered, but the MT is currently searching a new operator to register to
	 * - 3: registration denied
	 * - 4: unknown (e.g. out of GERAN/UTRAN/E-UTRAN coverage)
	 * - 5: registered, roaming
	 * - 6: registered for "SMSonly", home network(applicable only when AcTStatus indicates E-UTRAN)
	 * - 7: registered for "SMSonly",roaming (applicable only when AcTStatus indicates E-UTRAN)
	 * - 9: registered for "CSFBnotpreferred",home network(applicable only when AcTStatus indicates E-UTRAN)
	 * - 10 :registered for" CSFBnotpreferred",roaming (applicable only when AcTStatus indicates E-UTRAN)
	 */
	int stat = 0;

	/**
	 * @brief Two bytes location area code or tracking area code 
	 */
	int lac = 0xFFFF;

	/**
	 * @brief Cell Identifier (CI)
	 */
	int ci = 0xFFFFFFFF;

	/**
	 * @brief Radio access technology (RAT), the AcTStatus value in the response
	 * 
	 * - 0: GSM
	 * - 1: GSM COMPACT
	 * - 2: UTRAN
	 * - 3: GSM with EDGE availability
	 * - 4: UTRAN with HSDPA availability
	 * - 5: UTRAN with HSUPA availability
	 * - 6: UTRAN with HSDPA and HSUPA availability
	 * - 7: E-UTRAN
	 * - 255: the current AcTStatus value is invalid
	 */
	int rat = 0;
	
	/**
	 * @brief Returns true if the results were returned from the modem
	 */ 
	bool isValid() const { return valid; };

	/**
	 * @brief Converts the data in string into the broken out fields like stat, lac, ci, rat.
	 */
	void postProcess();

	/**
	 * @brief Converts this object into a readable string
	 */
	String toString() const;
};

/**
 * @brief Class for calling the u-blox SARA modem directly. 
 * 
 * Most of the methods you will need are in this class, and can be referenced using the
 * global `CellularHelper` object. For example: 
 * 
 * ```
 * String mfg = CellularHelper.getManufacturer();
 * ```
 */
class CellularHelperClass {
public:
	/**
	 * @brief Returns a string, typically "u-blox"
	 */
	String getManufacturer() const;

	/**
	 * @brief Returns a string like "SARA-G350", "SARA-U260" or "SARA-U270"
	 */
	String getModel() const;

	/**
	 * @brief Returns a string like "SARA-U260-00S-00".
	 */
	String getOrderingCode() const;

	/**
	 * @brief Returns a string like "23.20"
	 */
	String getFirmwareVersion() const;

	/**
	 * @brief Returns the IMEI for the modem
	 * 
	 * Both IMEI and ICCID are commonly used identifiers. The IMEI is assigned to the modem itself.
	 * The ICCID is assigned to the SIM card.
	 * 
	 * For countries that require the cellular devices to be registered, the IMEI is typically
	 * the number that is required.
	 */
	String getIMEI() const;

	/**
	 * @brief Returns the IMSI for the modem
	 */
	String getIMSI() const;

	/**
	 * @brief Returns the ICCID for the SIM card
	 * 
	 * Both IMEI and ICCID are commonly used identifiers. The IMEI is assigned to the modem itself.
	 * The ICCID is assigned to the SIM card.
	 */
	String getICCID() const;

	/**
	 * @brief Returns true if the device is LTE Cat-M1 (deprecated method)
	 * 
	 * This method is deprecated. You should use isSARA_R4() instead. Included for backward 
	 * compatibility for now.
	 */
	bool isLTE() const { return isSARA_R4(); };

	/**
	 * @brief Returns true if the device is a u-blox SARA-R4 model (LTE-Cat M1)
	 */
	bool isSARA_R4() const;

	/**
	 * @brief Returns the operator name string, something like "AT&T" or "T-Mobile" in the United States (2G/3G only)
	 * 
	 * | Modem          | Device | Compatible |
	 * | :------------: | :---:  | :---: |
	 * | SARA-G350      | Gen 2  | Yes |
	 * | SARA-U260      | Gen 2  | Yes |
	 * | SARA-U270      | Gen 2  | Yes |
	 * | SARA-U201      | All    | Yes |
	 * | SARA-R410M-02B | All    | No  |
	 */
	String getOperatorName(int operatorNameType = OPERATOR_NAME_LONG_EONS) const;

	/**
	 * @brief Get the RSSI and qual values for the receiving cell site.
	 * 	
	 * The response structure contains the following public members:
	 * 
	 * - resp - is RESP_OK if the call succeeded or RESP_ERROR on failure
	 * 
	 * - rssi - RSSI Received Signal Strength Indication value
	 *   - 0: -113 dBm or less
	 *   - 1: -111 dBm
	 *   - 2..30: from -109 to -53 dBm with 2 dBm steps
	 *   - 31: -51 dBm or greater
	 *   - 99: not known or not detectable or currently not available
	 * 
	 * - qual - Signal quality
	 *   - 0..7: Signal quality, 0 = good, 7 = bad
	 *   - 99: not known or not detectable or currently not available
	 *
	 * The qual value is not always available. 
	 * 
	 * | Modem          | Device | Qual Available |
	 * | :------------: | :---:  | :---: |
	 * | SARA-G350      | Gen 2  | No  |
	 * | SARA-U260      | Gen 2  | Usually |
	 * | SARA-U270      | Gen 2  | Usually |
	 * | SARA-U201      | All    | Usually |
	 * | SARA-R410M-02B | All    | No  |
	 */
	CellularHelperRSSIQualResponse getRSSIQual() const;


	/**
	 * @brief Gets extended quality information on LTE Cat M1 devices (SARA-R410M-02B) (AT+CESQ)
	 * 
	 * The response structure contains the following public members:
	 * 
	 * - resp - is RESP_OK if the call succeeded or RESP_ERROR on failure
	 * 
	 * - rxlev - Received Signal Strength Indication (RSSI)
	 *   - 0: less than -110 dBm
	 *   - 1..62: from -110 to -49 dBm with 1 dBm steps
	 *   - 63: -48 dBm or greater
	 *   - 99: not known or not detectable 
	 * 
	 * - ber - Bit Error Rate (BER)
	 *   - 0..7: as the RXQUAL values described in GSM TS 05.08 [28]
	 *   - 99: not known or not detectable 
	 * 
	 * - rscp - Received Signal Code Power (RSCP)
	 *   - 0: -121 dBm or less
	 *   - 1..95: from -120 dBm to -24 dBm with 1 dBm steps
	 *   - 96: -25 dBm or greater
	 *   - 255: not known or not detectable
	 * 
	 * - ecn0 - Ratio of received energy per PN chip to the total received power spectral density
	 *   - 0: -24.5 dB or less
	 *   - 1..48: from -24 dB to -0.5 dBm with 0.5 dB steps
	 *   - 49: 0 dB or greater
	 *   - 255: not known or not detectable 
	 * 
	 * - rsrq - Reference Signal Received Quality (RSRQ)
	 *   - 0: -19 dB or less
	 *   - 1..33: from -19.5 dB to -3.5 dB with 0.5 dB steps
	 *   - 34: -3 dB or greater
	 *   - 255: not known or not detectable Number 
	 * 
	 * - rsrp - Reference Signal Received Power (RSRP)
	 *   - 0: -141 dBm or less
	 *   - 1..96: from -140 dBm to -45 dBm with 1 dBm steps
	 *   - 97: -44 dBm or greater
	 *   - 255: not known or not detectable
	 * 
	 * 
	 * Compatibility:
	 * 
	 * | Modem          | Device | Available |
	 * | :------------: | :---:  | :---: |
	 * | SARA-G350      | Gen 2  | No  |
	 * | SARA-U260      | Gen 2  | No  |
	 * | SARA-U270      | Gen 2  | No  |
	 * | SARA-U201      | All    | No  |
	 * | SARA-R410M-02B | All    | Yes |
	 */
	CellularHelperExtendedQualResponse getExtendedQual() const;

	/**
	 * @brief Select the mobile operator (in areas where more than 1 carrier is supported by the SIM) (AT+COPS)
	 *
	 * @param mccMnc The MCC/MNC numeric string to identify the carrier. For examples, see the table below.
	 *
	 * @return true on success or false on error
	 *
	 * | MCC-MNC | Carrier |
	 * | :-----: | :--- |
	 * | 310410  | AT&T |
	 * | 310260  | T-Mobile |
     * 
	 * You must turn cellular on before making this call, but it's most efficient if you don't Cellular.connect()
	 * or Particle.connect(). You should use SYSTEM_MODE(SEMI_AUTOMATIC) or SYSTEM_MODE(MANUAL).
	 *
	 * If the selected carrier matches, this function return true quickly.
	 *
	 * If the carrier needs to be changed it may take 15 seconds or longer for the operation to complete.
	 *
	 * Omitting the mccMnc parameter or passing NULL will reset the default automatic mode.
	 *
	 * This setting is stored in the modem but reset on power down, so you should set it from setup().
	 */
	bool selectOperator(const char *mccMnc = NULL) const;

	/**
	 * @brief Gets cell tower information (AT+CGED). Only on 2G/3G, does not work on LTE Cat M1.
	 *
	 * @param mode is whether to get the serving cell, or serving cell and neighboring cells: 
	 * 
	 * - ENVIRONMENT_SERVING_CELL - only the cell you're connected to
	 * - ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS - note: only works on Electron 2G G350, not 3G models
	 * 
	 * @param resp Filled in with the response data.
	 * 
	 * Note: With Device OS 1.2.1 and later you should use the CellularGlobalIdentity built into
	 * Device OS instead of using this method (AT+CGED). CellularGlobalIdentity is much more efficient
	 * and works on LTE Cat M1 devices. The 5-cellular-global-identity example shows how to get the
	 * CI (cell identifier), LAC (location area code), MCC (mobile country code), and MNC (mobile
	 * network code) efficiently using CellularGlobalIdentity.
	 * 
	 * | Modem          | Device | Available | Neighbors Available |
	 * | :------------: | :---:  | :-------: | :---: |
	 * | SARA-G350      | Gen 2  | Yes       | Yes   |
	 * | SARA-U260      | Gen 2  | Yes       | No    |
	 * | SARA-U270      | Gen 2  | Yes       | No    |
	 * | SARA-U201      | All    | Yes       | No    |
	 * | SARA-R410M-02B | All    | No        | No    |
	 * 
	 */
	void getEnvironment(int mode, CellularHelperEnvironmentResponse &resp) const;


	/**
	 * @brief Gets the location coordinates using the CellLocate feature of the u-blox modem (AT+ULOC)
	 * 
	 * @param timeoutMs timeout in milliseconds. Should be at least 10 seconds.
	 * 
	 * This may take several seconds to execute and only works on Gen 2 2G/3G devices. It does not
	 * work on Gen 3 and does not work on LTE M1 (SARA-R410M-02B). In general, we recommend
	 * using a cloud-based approach (google-maps-device-locator) instead of using CellLocate.
	 * 
	 * Compatibility:
	 * 
	 * | Modem          | Device | Compatible |
	 * | :------------: | :---:  | :---: |
	 * | SARA-G350      | Gen 2  | Yes |
	 * | SARA-U260      | Gen 2  | Yes |
	 * | SARA-U270      | Gen 2  | Yes |
	 * | SARA-U201      | Gen 2  | Yes |
	 * | SARA-U201      | Gen 3  | No  |
	 * | SARA-R410M-02B | All    | No  |
	 * 
	 */
	CellularHelperLocationResponse getLocation(unsigned long timeoutMs = DEFAULT_TIMEOUT) const;

	/**
	 * @brief Gets the AT+CREG (registration info including CI and LAC) as an alternative to AT+CGED 
	 * 
	 * @param resp Filled in with the response data
	 * 
	 * Note: This has limited support and using the CellularGlobalIdentity in Device OS 1.2.1 and later 
	 * is the preferred method to get the CI and LAC. This function will be deprecated in the future.
	 * 
	 * | Modem          | Device | Device OS | Compatible |
	 * | :------------: | :---:  | :-------: | :---: |
	 * | SARA-G350      | Gen 2  | < 1.2.1   | Yes   |
	 * | SARA-U260      | Gen 2  | < 1.2.1   | Yes   |
	 * | SARA-U270      | Gen 2  | < 1.2.1   | Yes   |
	 * | SARA-U201      | Gen 2  | < 1.2.1   | Yes   |
	 * | SARA-R410M-02B | Gen 2  | < 1.2.1   | Yes   |
	 * | Any            | Gen 3  | Any       | No    |
	 * | Any            | Any    | >= 1.2.1  | No    |
	 * 
	 */
	void getCREG(CellularHelperCREGResponse &resp) const;

	/**
	 * @brief Append a buffer (pointer and length) to a String object
	 * 
	 * Used internally to add data to a String object with buffer and length,
	 * which is not one of the built-in overloads for String. This format is
	 * what comes from the Cellular.command callbacks.
	 * 
	 * @param str The String object to append to
	 * 
	 * @param buf The buffer to copy from. Does not need to be null terminated. 
	 * 
	 * @param len The number of bytes to copy.
	 * 
	 * @param noEOL (default: true) If true, don't copy CR and LF characters to the output.
	 */
	static void appendBufferToString(String &str, const char *buf, int len, bool noEOL = true);

	/**
	 * @brief Default timeout in milliseconds. Passed to Cellular.command().
	 * 
	 * Several commands take an optional timeout value and the default value is this.
	 */
	static const system_tick_t DEFAULT_TIMEOUT = 10000;

	/**
	 * @brief Constant for getEnvironment() to get information about the connected cell
	 * 
	 * This only works on the 2G and 3G Electron, E Series, and Boron. It does not work on LTE Cat M1
	 * (SARA-R410M-02B) models. See getEnvironment() for alternatives.
	 */
	static const int ENVIRONMENT_SERVING_CELL = 3;

	/**
	 * @brief Constant for getEnvironment() to get information about the connected cell and neighbors
	 * 
	 * This only works on the 2G Electron (G350)!
	 */
	static const int ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS = 5;

	/**
	 * @brief Constants for getOperatorName(). Default and recommended value is OPERATOR_NAME_LONG_EONS.
	 * 
	 * In most cases, `OPERATOR_NAME_NUMERIC` is 6 BCD digits in cccnnn format where (ccc = MCC and nnn = MNC). 
	 * However, in some countries MNC is only two BCD digits, so in that case it will be cccnn (5 BCD digits).
	 */
	static const int OPERATOR_NAME_NUMERIC = 0; 				//!< Numeric format of MCC/MNC network (three BCD digit country code and two/three BCD digit network code)
	static const int OPERATOR_NAME_SHORT_ROM = 1; 				//!< Short name in ROM
	static const int OPERATOR_NAME_LONG_ROM = 2; 				//!< Long name in ROM
	static const int OPERATOR_NAME_SHORT_CPHS = 3; 				//!< Short network operator name (CPHS)
	static const int OPERATOR_NAME_LONG_CPHS = 4; 				//!< Long network operator name (CPHS)
	static const int OPERATOR_NAME_SHORT_NITZ = 5; 				//!< Short NITZ name
	static const int OPERATOR_NAME_LONG_NITZ = 6; 				//!< Full NITZ name
	static const int OPERATOR_NAME_SERVICE_PROVIDER = 7; 		//!< Service provider name
	static const int OPERATOR_NAME_SHORT_EONS = 8; 				//!< EONS short operator name
	static const int OPERATOR_NAME_LONG_EONS = 9; 				//!< EONS long operator name
	static const int OPERATOR_NAME_SHORT_NETWORK_OPERATOR = 11; //!< Short network operator name
	static const int OPERATOR_NAME_LONG_NETWORK_OPERATOR = 12; 	//!< Long network operator name

	/**
	 * @brief Used internally as the Cellular.command callback
	 * 
	 * @param type one of 13 different enumerated AT command response types.
	 * 
	 * @param buf a pointer to the character array containing the AT command response.
	 * 
	 * @param len length of the AT command response buf.
	 *
	 * @param param a pointer to the variable or structure being updated by the callback function.
	 *
	 * @return user specified callback return value
 	 */
	static int responseCallback(int type, const char* buf, int len, void *param);

	/**
	 * @brief Function to convert an RSSI value into "bars" of signal strength (0-5)
	 * 
	 * | RSSI    | Bars |
	 * | :-----: | :---: |
	 * | >= -57  | 5 |
	 * | > -68   | 4 |
	 * | > -80   | 3 |
	 * | > -92   | 2 |
	 * | > -104  | 1 |
	 * | <= -104 | 0 |
	 * 
	 * 5 is strong and 0 is weak.
	 */
	static int rssiToBars(int rssi);


};

extern CellularHelperClass CellularHelper;

#endif /* Wiring_Cellular */

#endif /* __CELLULARHELPER_H */
