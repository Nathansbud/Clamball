#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

#define DEBUG true
#define DEBUG_PRINT(A) (DEBUG ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUG ? Serial.println(A) : NULL)

ArduinoLEDMatrix matrix;

// Per https://docs.arduino.cc/tutorials/uno-r4-wifi/led-matrix/,
// use a more concise memory representation
uint32_t boardState[3] = {
  0x000000000, 
  0x000000000,
  0x000000000
};

void clearBoardState() {
  for(int i = 0; i < 3; i++) boardState[i] = 0;
}

// Toggle an LED on in the 8x12 matrix
void enableLED(int row, int column) {
  if(!(0 <= row && row < 8 && 0 <= column && column < 12)) {
    DEBUG_PRINT("Failed to enable invalid LED: ");
    DEBUG_PRINT(row);
    DEBUG_PRINT(", ");
    DEBUG_PRINTLN(column);
    return;
  }

  // Convert an 8x12 row-column index to a 3x32 boardState index
  uint8_t index1D = row * 12 + column;
  uint8_t newRow = index1D / 32;
  uint8_t newCol = index1D % 32;
  boardState[newRow] |= (1 << (31 - newCol));
}

// Display a message on the LED matrix using the ArduinoGraphics library,
// per https://docs.arduino.cc/tutorials/uno-r4-wifi/led-matrix/#scrolling-text-example
void displayMessage(char message[], int textScrollSpeed) {
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(textScrollSpeed);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(message);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();
} 

void setup() {
  Serial.begin(115200);
  matrix.begin();
}

void loop() {
  static int counter = 0;
  enableLED(counter, counter++);
  matrix.loadFrame(boardState);
  delay(100);
}
