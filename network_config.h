#include <ArduinoHttpClient.h>
#include "WiFiS3.h"

// SSID/password should be filled in here for the relevant network, and should not be committed with secrets;
// if you plan to update this file, be sure to run the following command to allow local modification:
// git update-index --skip-worktree network_config.h
#define NETWORK_OPEN true
#define NETWORK_SSID ""
#define NETWORK_PASS ""

// If running locally, replace this with active IP address; for macOS,
// this can be retrieved from CLI via: ipconfig getifaddr en0
IPAddress server(0, 0, 0, 0);
uint16_t port = 6813;
