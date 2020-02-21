# Cellular Helper

**A library to access useful things from the Electron, E Series, Boron, and B Series cellular modem**

**Note: The official version of this library can now be found at: [https://github.com/particle-iot/CellularHelper](https://github.com/particle-iot/CellularHelper)**

The code here will be archived.

The full API documentation can be [found here](https://rickkas7.github.io/CellularHelper/).

## Simple functions

The simple information calls query information from the modem and return a String.

```
Log.info("manufacturer=%s", CellularHelper.getManufacturer().c_str());

Log.info("model=%s", CellularHelper.getModel().c_str());

Log.info("firmware version=%s", CellularHelper.getFirmwareVersion().c_str());

Log.info("ordering code=%s", CellularHelper.getOrderingCode().c_str());

Log.info("IMEI=%s", CellularHelper.getIMEI().c_str());

Log.info("IMSI=%s", CellularHelper.getIMSI().c_str());

Log.info("ICCID=%s", CellularHelper.getICCID().c_str());
```

These are all just various bits of data from the modem or the SIM card. 

```
0000008020 [app] INFO: manufacturer=u-blox
0000008040 [app] INFO: model=SARA-U260
0000008060 [app] INFO: firmware version=23.20
0000008090 [app] INFO: ordering code=SARA-U260-00S-00
```

You might find the ICCID (SIM Number) to be useful as well.

`Cellular.on()` needs to have been called, which happens automatically in automatic mode (the default).

Note that if you are using `SYSTEM_THREAD(ENABLED)` or `SYSTEM_MODE(SEMI_AUTOMATIC)` or `SYSTEM_MODE(MANUAL)` you must also wait after turning the modem on. 4 seconds should be sufficient, because `Cellular.on()` is asynchronous and there's no call to determine if has completed yet. Instead of a delay, you could also use `cellular_on(NULL)` which blocks until the modem is turned on.


## Cellular connection functions

For full information about these commands, consult the API documentation which describes these functions in much more detail.

These functions can only be used after connecting to the cellular network.

### getOperatorName (2G/3G only)

Returns a string containing the operator name, for example AT&T or T-Mobile in the United States.

```
Log.info("operator name=%s", CellularHelper.getOperatorName().c_str());
```

Example output:

```
0000008574 [app] INFO: operator name=AT&T
```

The operator name is not available on LTE Cat M1 devices (SARA-R410M-02-B).

### getRSSIQual

Returns the RSSI (signal strength) value and a quality value.

```
CellularHelperRSSIQualResponse rssiQual = CellularHelper.getRSSIQual();
int bars = CellularHelperClass::rssiToBars(rssiQual.rssi);

Log.info("rssi=%d, qual=%d, bars=%d", rssiQual.rssi, rssiQual.qual, bars);
```

```
0000008595 [app] INFO: rssi=-75, qual=2, bars=3
```

The RSSI is in dBm, the standard measure of signal strength. It's a negative value, and values closer to 0 are higher signal strength. There is more information in the [API documentation](https://rickkas7.github.io/CellularHelper/class_cellular_helper_class.html#af9012f92f58f5151cc7006aceadd311b).

The quality value is 0 (highest quality) to 7 (lowest quality) or 99 if the value unknown. It's typically 99 for 2G connections. The qual value is described in the u-blox documentation, and it's returned by the call, but you probably won't need to use it.

The `rssiToBars()` method converts the RSSI to a 0 to 5 bars, where 5 is the strongest signal.

The `CellularHelper.getExtendedQualResponse()` is available on LTE Cat M1 devices and returns LTE specific parameters like the RSRP. You can find more information [here](https://rickkas7.github.io/CellularHelper/class_cellular_helper_class.html#a2cf4ec66557254498d5d7600ea66bd74).

### getEnvironment (2G/3G only)

The method getEnvironment returns cell tower information. The is the u-blox AT+CGED command.

```
CellularHelperEnvironmentResponseStatic<8> envResp;

CellularHelper.getEnvironment(CellularHelper.ENVIRONMENT_SERVING_CELL_AND_NEIGHBORS, envResp);
if (envResp.resp != RESP_OK) {
	// We couldn't get neighboring cells, so try just the receiving cell
	CellularHelper.getEnvironment(CellularHelper.ENVIRONMENT_SERVING_CELL, envResp);
}
envResp.logResponse();

```

The first line declares a variable to hold up to 8 neighbor towers.

The first getEnvironment call tries to get the serving cell (the one you're connected to) and the neighbor cells. This only works for me on the 2G (G350) Electron.

If that fails, it will try again only using the serving cell information.

This sample just prints the information to serial debug:

```
0000008645 [app] INFO: service rat=UMTS mcc=310, mnc=410, lac=2cf7 ci=8a5a782 band=UMTS 850 rssi=0 dlf=4384 ulf=4159
```

Note that the rssi will always be 0 for 3G towers. This information is only returned by the AT+CGED command for 2G towers. You can use getRSSIQual() to get the RSSI for the connected tower; that works for 3G.

### getLocation (2G/3G only)

This function returns the location of the Electron, using cell tower location. This call may take 10 seconds to complete!

```
CellularHelperLocationResponse locResp = CellularHelper.getLocation();
Log.info(locResp.toString());
```

The locResp contains the member variables:

- lat - The latitude (in degrees, -90 to +90)
- lon - The longitude (in degrees, -180 to +180)
- alt - The altitude (in meters)
- uncertainty - The radius of the circle of uncertainty (in meters)
	
This only works on 2G/3G devices, and a better alternative in most cases is to use the google-maps-device-locator to do the location query on the cloud-side instead of on-device. 

## Examples

### 1-Simple Demo

The simple demo tests all of the basic functions in the library, displaying the results to USB serial.

The code examples in this document were taken from this example.


### 2-Show Carriers Demo

This is a demo program that uses the cellular modem to scan for available operators, frequency band used, and signal strength. It prints a result like this to USB serial:

```
3G AT&T UMTS 850 2 bars (310410)
2G T-Mobile DCS 1800 or 1900 2 bars (310260)
```

It should work even when you can't connect to a tower and also display carriers that are not supported by your SIM. (It only displays carriers compatible with the GSM modem, however, so it won't, for example, display Verizon in the United States since that requires a PCS modem.)

This is a very time consuming operation (it can take 2 minutes or longer to run) and it's pretty rarely needed, so it builds on the CellularHelper library but the commands it uses (AT+COPS and AT+COPN) are not part of the library itself because they're so rarely needed.

To build a binary for this, you can download the repository and use the Particle CLI compiler from the top level of it:

```
particle compile electron examples/2-show-carriers/ --saveTo firmware.bin 
```

Then you can flash it to your Electron in DFU mode (blinking yellow):

```
particle flash --usb firmware.bin
```

### 3-Select Carrier Demo

The 3-select-carrier example shows how to prefer a certain carrier when multiple carriers are supported by the SIM card.

- You should use SYSTEM_MODE(SEMI_AUTOMATIC) so you can set the carrier before trying to connect.
- You need to turn the cellular modem on using `cellular_on(NULL)` or `Cellular.on()` (see comments in the code).
- Call selectOperator() with the MCC/MNC. For example:

```
	// Select the operator using the MCC/MNC string. For example:
	// "310410" = AT&T
	// "310260" = T-Mobile
	bool bResult = CellularHelper.selectOperator("310260");
```

The 2-show-carriers example prints out the MCC/MNC for carriers at your location.

- If the operator needs to be changed, it may take around 20 seconds to do so. If the operator is already selected, it returns more or less immediately.
- The setting of the operator is stored in the modem, but is reset on power down, so you should always select the operator in setup().

On a cold boot, you might see something like this in the USB serial debug log:

```
     5.297 AT send      12 "AT+UDOPN=0\r\n"
     5.307 AT read ERR  34 "\r\n+CME ERROR: no network service\r\n"
     5.307 AT send      22 "AT+COPS=4,2,\"310260\"\r\n"
    23.778 AT read OK    6 "\r\nOK\r\n"
0000023789 [app] INFO: selectOperator returned 1
```

Then, later:

```
0000042189 [system] INFO: ARM_WLAN_WD 2
0000042189 [system] INFO: CLR_WLAN_WD 1, DHCP success
    42.180 AT send      12 "AT+UDOPN=9\r\n"
    42.220 AT read  +   24 "\r\n+UDOPN: 2,\"T-Mobile\"\r\n"
    42.230 AT read OK    6 "\r\nOK\r\n"
0000042241 [app] INFO: current operator=T-Mobile
```

If you do a warm boot after setting:

```
     2.507 AT send      12 "AT+UDOPN=0\r\n"
     2.517 AT read  +   22 "\r\n+UDOPN: 0,\"310260\"\r\n"
     2.527 AT read OK    6 "\r\nOK\r\n"
0000002527 [app] INFO: operator already 310260
0000002527 [app] INFO: selectOperator returned 1
```

### 4-cell-locate

This demo uses the u-blox Cell Locate feature to find the latitude, longitude, and elevation of the device.

This only works on 2G and 3G devices, not LTE Cat M1. Another alternative is to use google-maps-device-locator 
which works on all devices, and is also generally much faster.

### 5-cellular-global-identity

Previously getting the cell tower information required a variety of techniques that depended on the cellular 
modem and generation of device. In Device OS 1.2.1 and later, it's now easy to query this information from
Device OS. This example shows how.

## Version History

#### 0.1.0 (2020-02-13)

- Code cleanup
- Minimum Device OS version is now 0.7.0 
- Removed non-working ping and DNS calls
- Added lots of API documentation

#### 0.0.7 (2018-11-27) 

- Added the getCREG call
- Last versions that supports Device OS 0.6.x

#### 0.0.6 

- Added selectOperator


