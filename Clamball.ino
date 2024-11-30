// #include "ArduinoGraphics.h"
// #include "Arduino_LED_Matrix.h"
// #include <HCSR04.h>

// #define DEBUG true
// #define DEBUG_PRINT(A) (DEBUG ? Serial.print(A) : NULL)
// #define DEBUG_PRINTLN(A) (DEBUG ? Serial.println(A) : NULL)
// #define NUM_SENSORS 5

// ArduinoLEDMatrix matrix;

// // This architecture assumes all sensors are being driven at the same time; there is no real reason not to do this, 
// // since they should always be being checked at the same time, so using this wrapper
// // HCSR04 monitors(
// //   /* Trigger Pin */ 8, 
// //   /* Echo Pins */ new int[NUM_SENSORS]{7, 12, 11, 10, 9},
// //   /* Sensors */ NUM_SENSORS
// // );

// float readings[5] = {0, 0, 0, 0, 0};

// // Per https://docs.arduino.cc/tutorials/uno-r4-wifi/led-matrix/,
// // use a more concise memory representation
// uint32_t boardState[3] = {
//   0x000000000, 
//   0x000000000,
//   0x000000000
// };

// void clearBoardState() {
//   for(int i = 0; i < 3; i++) boardState[i] = 0;
// }

// // Toggle an LED on in the 8x12 matrix
// void enableLED(int row, int column) {
//   if(!(0 <= row && row < 8 && 0 <= column && column < 12)) {
//     DEBUG_PRINT("Failed to enable invalid LED: ");
//     DEBUG_PRINT(row);
//     DEBUG_PRINT(", ");
//     DEBUG_PRINTLN(column);
//     return;
//   }

//   // Convert an 8x12 row-column index to a 3x32 boardState index
//   uint8_t index1D = row * 12 + column;
//   uint8_t newRow = index1D / 32;
//   uint8_t newCol = index1D % 32;
//   boardState[newRow] |= (1 << (31 - newCol));
// }

// // Display a message on the LED matrix using the ArduinoGraphics library,
// // per https://docs.arduino.cc/tutorials/uno-r4-wifi/led-matrix/#scrolling-text-example
// void displayMessage(char message[], int textScrollSpeed) {
//   matrix.beginDraw();
//   matrix.stroke(0xFFFFFFFF);
//   matrix.textScrollSpeed(textScrollSpeed);
//   matrix.textFont(Font_5x7);
//   matrix.beginText(0, 1, 0xFFFFFF);
//   matrix.println(message);
//   matrix.endText(SCROLL_LEFT);
//   matrix.endDraw();
// } 

// void setup() {
//   Serial.begin(9600);
//   matrix.begin();
//   // setupWifi();
// }

// void loop() {
//   /* Preliminary setup for simply reading all the sensors */
//   for(int i = 0; i < NUM_SENSORS; i++) {
//     Serial.print(i);
//     Serial.print("] Sensor Reading: ");
//     Serial.println(monitors.dist(i)); 
//   }
  
//   // static int counter = 0;
//   // enableLED(counter, counter++);
//   // delay(100);
//   // loopWifi();
//   // float dist = monitor.dist();
//   // if(dist > 4 && dist < 8.5) {
//   //   Serial.println("Hole 1!!!");
//   //   enableLED(0, 0);
//   // } else if(dist >= 8.5 && dist < 13) {
//   //   Serial.println("Hole 2!!!");
//   //   enableLED(2, 0);
//   // } else if(dist >= 13 && dist < 17.5) {
//   //   Serial.println("Hole 3!!!");
//   //   enableLED(4, 0);
//   // } else if(dist < 18) {
//   //   Serial.print("something close but not in the hole bounds: ");
//   //   Serial.println(dist);
//   //   enableLED(6, 0);
//   // }

//   // matrix.loadFrame(boardState);
//   delay(10);
// }
