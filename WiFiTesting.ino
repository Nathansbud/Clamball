#include "WiFiS3.h"
#include "network_config.h" 

char ssid[] = NETWORK_SSID;        // your network SSID (name)
char pass[] = NETWORK_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// If running locally, replace this with active IP address; for macOS,
// this can be retrieved from CLI via: ipconfig getifaddr en0
IPAddress server("10.37.74.223");

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

/* -------------------------------------------------------------------------- */
void setupWifi() {
/* -------------------------------------------------------------------------- */  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    if(!NETWORK_OPEN) {
      Serial.print("Attempting to connect to non-open SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
    } else {
      Serial.print("Attempting to connect to open SSID: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid);
    }
     
    // wait 10 seconds for connection:
    delay(10000);
  }
  
  // printWifiStatus();
 
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 6813)) {
    Serial.println("Connected to server!");
    // Make a HTTP request to no particular endpoint
    client.println("GET / HTTP/1.1");
    
    // Make a request to the specific IP we care about
    client.print("Host: ");
    client.println(server);

    client.println("Connection: close");
    client.println();
  }
}

void read_response() {
  uint32_t received_data_num = 0;
  while (client.available()) {
    /* actual data reception */
    char c = client.read();
    /* print data to serial port */
    Serial.print(c);
    /* wrap data to 80 columns*/
    received_data_num++;
    if(received_data_num % 80 == 0) { 
      Serial.println();
    }
  }  
}

void loopWifi() {
  read_response();

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
    
    // do nothing forevermore:
    while (true);
  }
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
