#include "Particle.h"

SerialLogHandler logHandler;

// This example requires Device OS 1.2.1 or later!

SYSTEM_THREAD(ENABLED);

const unsigned long CHECK_PERIOD_MS = 10000;
unsigned long lastCheck = 0;

void setup() {

}

void loop() {
    if (millis() - lastCheck >= CHECK_PERIOD_MS) {
        lastCheck = millis();

        // Using CellularGlobalIdentity works on Device OS 1.2.1 and later on all devices and modems.
        // The information is cached by Device OS so you can call it relatively frequently without
        // much of a performance penalty. The data will update as you move between cells automatically.
        // It only works once cloud connected, however. It will return error -1200 if you are only 
        // connected to cellular and not the cloud.

        CellularGlobalIdentity cgi = {0};
        cgi.size = sizeof(CellularGlobalIdentity);
        cgi.version = CGI_VERSION_LATEST;

        cellular_result_t res = cellular_global_identity(&cgi, NULL);
        if (res == SYSTEM_ERROR_NONE) {
            Log.info("cid=%d lac=%u mcc=%d mnc=%d", cgi.cell_id, cgi.location_area_code, cgi.mobile_country_code, cgi.mobile_network_code);
        }
        else {
            Log.info("cellular_global_identity failed %d", res);
        }
    }
}