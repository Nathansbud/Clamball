#include "network_config.h" 
#include "WDT.h"
  
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
    
    delay(2500);
  }
  
  Serial.println("Successfully connected!");
}

void setupServer() {
  #ifndef TESTING:
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
    Serial.print("Failed to connect to server on port ");
    Serial.print(port);
    Serial.print(",  code: ");
    Serial.println(response);
  }
  delay(1000);
  #else 
  CABINET_NUMBER = T_CABINET_NUMBER_B;
  T_CABINET_NUMBER_A = CABINET_NUMBER;
  #endif
}

void setupWdt() {
  if(WDT.begin(wdtInterval)) {
    WDT.refresh();
  } else {
    //error initializing wdt
    while(1) { 
      Serial.println("Failed to initilalize WDT"); 
    }
  }
}

ResponseType checkShouldStart() {
  #ifndef TESTING
  if(manager.connect(server, port) == 1) {
    client.get("/request-start");

    int statusCode = client.responseStatusCode();
    if(statusCode == 200 && client.responseBody() == "Y") { 
      return SUCCESS;
    }

    return FAILURE;
  } 

  return ERROR;
  #else
  return T_START == 1 ? SUCCESS : FAILURE;
  #endif
}

int sendHoleUpdate(uint8_t cabinetID, uint8_t index) {
  #ifndef TESTING
  Serial.print("Index: ");
  Serial.println(index);
  #endif
  return sendHoleUpdate(cabinetID, index / 5, index % 5);
}

int sendHoleUpdate(uint8_t cabinetID, uint8_t row, uint8_t col) {
  #ifdef TESTING
  return T_RESPONSE_HU;
  #endif
  
  if(!networked) { 
    return -1; 
  }

  if(manager.connect(server, port) == 1) {
    Serial.print("Sending hole update: ");
    Serial.print(row);
    Serial.print(", ");
    Serial.println(col);
    
    // Using a 6-character buffer: msg is at MOST 2 digits, - is one, and row/col are between 0 and 5, and null terminator is the final byte;
    // was previously using 5, but think this isn't accounting for the null terminator
    char msg[6];
    snprintf(msg, 6, "%d-%d%d", cabinetID, row, col);

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
  // Main mode of operation of this method
  #ifndef TESTING
    if(manager.connect(server, port) == 1) { 
      client.get("/heartbeat");
      return checkWinner(client.responseBody());
    }
    
    return -2;
  #else 
    // Testing-specific, mocked out version
    //put wdt here, code will hang when not 
    //maybe loop the wdt and if timer expires, reset the system...
    wdtMillis = T_WDT_MILLIS;
    totalElapsed = T_TOT_ELAPSED;
    activeState = T_STATE_DS;
    if(T_WATCHDOG == 1) { 
      WDT.refresh();
      totalElapsed = 0; //reset total elapsed
    }
    else {
      if(T_MILLIS - wdtMillis >= wdtInterval - 1) {
        WDT.refresh();
        wdtMillis = T_MILLIS;

        totalElapsed += wdtInterval;

        if (totalElapsed >= totalInterval){
          //trigger wdt!! reset here bc its been hanging too long
          activeState = GAME_RESET;
        }
      }
    }
    T_WDT_MILLIS = wdtMillis;
    T_TOT_ELAPSED = totalElapsed; 
    T_STATE_DS = activeState;
    return T_RESPONSE_HB;
  #endif
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

// Modified from basic WiFi example!
void printWifiStatus() {
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
