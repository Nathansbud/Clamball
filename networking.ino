#include <ArduinoHttpClient.h>
#include "network_config.h" 

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
  #include "WiFiS3.h"
#else
  #include <SPI.h>
  #include <WiFiNINA.h>
#endif 
  
// If running locally, replace this with active IP address; for macOS,
// this can be retrieved from CLI via: ipconfig getifaddr en0
IPAddress server(10, 37, 117, 156);
uint16_t port = 6813;

char ssid[] = NETWORK_SSID;
char pass[] = NETWORK_PASS;
int keyIndex = 0;

int status = WL_IDLE_STATUS;

WiFiClient manager;
HttpClient client = HttpClient(manager, server, port);

void setupWifi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  while (status != WL_CONNECTED) {
    if(!NETWORK_OPEN) {
      Serial.print("Attempting to connect to non-open SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
    } else {
      Serial.print("Attempting to connect to open SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid);
    }
     
    // wait 2.5 seconds for re-connect attempt:
    delay(2500);
  }
}

void setupServer() {
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:)
  
  int response = manager.connect(server, port);
  if (response == 1) {
    Serial.println("Connected to server!");
    
    client.get("/register-cabinet");
    
    int statusCode = client.responseStatusCode();
    if(statusCode == 200) {
      int cabinetResponse = client.responseBody().substring(1).toInt();
      
      Serial.print("Assigning cabinet: ");
      Serial.println(cabinetResponse);

      // Update cabinet number to the assigned one
      CABINET_NUMBER = cabinetResponse;
    }
  } else {
    Serial.print("Failed to connect to server, code: ");
    Serial.print(response);
    Serial.print(" - ");
    Serial.println(manager.connected());
  }

  delay(1000);
}

ResponseType checkShouldStart() {
  if(manager.connect(server, port) == 1) {
    client.get("/request-start");
    
    int statusCode = client.responseStatusCode();
    if(statusCode == 200 && client.responseBody() == "Y") { 
      return SUCCESS;
    }

    return FAILURE;
  }
  
  return ERROR;
}

int sendHoleUpdate(uint8_t cabinetID, uint8_t index) {
  return sendHoleUpdate(cabinetID, index % 5, index / 5);
}

int sendHoleUpdate(uint8_t cabinetID, uint8_t row, uint8_t col) {
  if(manager.connect(server, port) == 1) {
    Serial.print("Sending hole update: ");
    Serial.print(row);
    Serial.print(", ");
    Serial.println(col);
    
    // Using a 5-character buffer: msg is at MOST 2 digits, - is one, and row/col are between 0 and 5
    char msg[5];
    snprintf(msg, 5, "%d-%d%d", cabinetID, row, col);

    client.post("/hole-update", "text/plain", msg);

    return checkWinner(client.responseBody());
  }

  return -2;
}

int checkWinner(String content) {
  if(content == "N") {
    return -1;
  } else {
    return content.substring(1).toInt();
  }
}

int sendHeartbeat() {
  if(manager.connect(server, port) == 1) { 
    client.get("/heartbeat");
    
    return checkWinner(client.responseBody());
  }

  return -2;
}

// Credit to https://forum.arduino.cc/t/finding-the-mac-address-of-the-arduino-uno-r4/1308027/
// for MAC address snippets
void printMacAddress(byte mac[]) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */  
  byte mac[6];
  Serial.print("MAC Address: ");
  printMacAddress(WiFi.macAddress(mac));

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
