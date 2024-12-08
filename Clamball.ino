#include "ArduinoGraphics.h"
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
  #include "Arduino_LED_Matrix.h"
#endif

#include "Clamball.h"

#define DEBUGGING true
#define DEBUG_PRINT(A) (DEBUGGING ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUGGING ? Serial.println(A) : NULL)
#define NUM_SENSORS 5

#define sensor0 A0
#define sensor1 A1
#define sensor2 A2
#define sensor3 A3
#define sensor4 A4

#define TESTING /* UNCOMMENT ME TO TEST FSM! */

enum ResponseType {
  SUCCESS = 1,
  FAILURE = 0,
  ERROR = -1
};

enum DeviceState { 
  ATTEMPTING, //1
  WAITING_FOR_GAME, //2
  INITIALIZE_GAME, //3
  WAITING_FOR_BALL, //4 
  BALL_SENSED, //5
  
  UPDATE_COVERAGE, //6
  SEND_UPDATE, //7

  GAME_TIE, //8
  GAME_WIN, //9
  GAME_LOSS, //10

  GAME_END //11
};

DeviceState activeState = ATTEMPTING;
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
ArduinoLEDMatrix matrix;
#endif

// We need to assign our cabinet number via WiFi request
int CABINET_NUMBER = -1;

// This architecture assumes all sensors are being driven at the same time; there is no real reason not to do this,
// since they should always be being checked at the same time, so using this wrapper
// HCSR04 monitors(
//   /* Trigger Pin */ 8,
//   /* Echo Pins */ new int[NUM_SENSORS]{ 7, 12, 11, 10, 9 },
//   /* Sensors */ NUM_SENSORS);

const int THRESHOLD_COUNT = 10;
int activeHole = -1;
int sensedCount = 0;

// Per https://docs.arduino.cc/tutorials/uno-r4-wifi/led-matrix/,
// use a more concise memory representation
uint32_t boardState[3] = {
  0x000000000,
  0x000000000,
  0x000000000
};

void clearBoardState() {
  for (int i = 0; i < 3; i++) boardState[i] = 0;
}

void activateHole(int holeIdx) {
  if(0 <= holeIdx && holeIdx < 25) { 
    uint8_t rowIdx = holeIdx / 5;
    uint8_t colIdx = holeIdx % 5;
    enableLED(rowIdx, colIdx);
  }

  matrix.loadFrame(boardState);
}

// Toggle an LED on in the 8x12 matrix
void enableLED(int row, int column) {
  if (!(0 <= row && row < 8 && 0 <= column && column < 12)) {
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

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
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
#endif

void setup() {
  Serial.begin(115200);
  #if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
  matrix.begin();
  #endif
  setupWifi();
}

DeviceState checkWinnerTransition(int response) {
  if(response >= 0) {
    if(response == CABINET_NUMBER) {
      return GAME_WIN; 
    } else {
      return GAME_LOSS;
    }        
  } else if(response == -1) {
    return WAITING_FOR_BALL;
  } else if(response == -2) {
    return ATTEMPTING;
  }
}

void manageFSM() {
  static int candidateHole = -1;
  #ifdef TESTING
  activeState = T_STATE;
  #endif
  switch(activeState) {
    case ATTEMPTING:
      setupServer();      

      // If our server setup did not provide our cabinet with an ID to use, we self-loop back to keep trying again; 
      // the server might not be started, or our Arduino might not actually be connected to WiFi yet
      if(CABINET_NUMBER == -1) { // Transition: ATTEMPTING -> ATTEMPTING
        activeState = ATTEMPTING;
      } else { // Transition: ATTEMPTING -> WAITING_FOR_GAME
        activeState = WAITING_FOR_GAME;
      }

      T_STATE = activeState;
      break;
    case WAITING_FOR_GAME:
      // This is a bit of an odd model, but it's a pain to have the Arduino acting as client AND server,
      // without some sort of threading model that might necessitate an OS; as such, we use polling even in contexts where we'd
      // ideally have the server send a request to the client
      switch(checkShouldStart()) {
        case SUCCESS:
          Serial.println("Let the games...begin!");
          activeState = INITIALIZE_GAME;
          T_STATE = activeState;
          break;
        case FAILURE:
          Serial.println("Waiting for game...");
          activeState = WAITING_FOR_GAME;
          T_STATE = activeState;
          break;
        case ERROR:
        default:
          break;
      }
      
      delay(1000);
      break;
    case INITIALIZE_GAME:
      clearBoardState();
      matrix.loadFrame(boardState);
      activeState = WAITING_FOR_BALL;
      T_STATE = activeState;
      break;
    case WAITING_FOR_BALL:
      candidateHole = pollSensors();
      if(candidateHole != -1) {
        Serial.print("Sensed ball in hole: ");
        Serial.println(candidateHole);
        activeState = BALL_SENSED;   
        T_STATE = activeState;     
      } else {
        Serial.println("Sensed no ball! Sending heartbeat...");
        // TODO-Mikayla+Lucy: Do we want to clear out the active hole if any sensor reading fails?
        activeHole = -1;
        sensedCount = 0;
        
        // Ignore heartbeat for now
        break;
        
        // Heartbeat serves the purpose of checking if a winner has been decided by the server,
        // as we need to poll for it; response is one of: -2 (server error), -1 (no winner), 0, ..., 99 (winning ID)
        int response = sendHeartbeat();
        Serial.print("Response in no ball: ");
        Serial.println(response);
        activeState = checkWinnerTransition(response);
      }
      break;
    case BALL_SENSED:
      // If we have already seen this hole, increase the sensedCount;
      // otherwise, we have seen a new hole, so should check that one instead
      // activeHole = candidateHole;
      // activeState = UPDATE_COVERAGE;
      if(candidateHole == activeHole) {
        sensedCount += 1;
      } else {
        activeHole = candidateHole;
        sensedCount = 1;
      }
      delay(2000);
      #ifdef TESTING
      sensedCount = T_COUNT;
      #endif
      
      if(sensedCount >= THRESHOLD_COUNT) {
        activeState = UPDATE_COVERAGE;
      } else {
        activeState = WAITING_FOR_BALL;
      }
      T_STATE = activeState;
      break;
    case UPDATE_COVERAGE:
      activateHole(activeHole);
      activeState = SEND_UPDATE;
      T_STATE = activeState;
      break;
    case SEND_UPDATE: {
      int response = sendHoleUpdate(CABINET_NUMBER, activeHole);
      
      Serial.print("Got response after sending hole: ");
      Serial.println(response);


      // Reset hole metadata!
      activeHole = -1;
      sensedCount = 0;

      // This state transition is guaranteed to leave SEND_UPDATE;
      // it takes either to WAITING_FOR_HOLE (no winner), GAME_WIN/GAME_LOSS (winner), or ATTEMPTING (server error) 
      activeState = checkWinnerTransition(response);
      Serial.print("Transitioning to: ");
      Serial.println(activeState);
      T_STATE = activeState;
      break;
    }
    case GAME_WIN:
      displayMessage("winner, winner, chicken dinner!", 150);
      break;
    case GAME_LOSS:
      displayMessage("loser, loser, lemon snoozer!", 150);
      break;
    case GAME_TIE:
      displayMessage("tie, tie, no need to cry!", 150);
      break;
    case GAME_END:  
      break;
    default:
      activeState = ATTEMPTING;
      break;
  }
}

int sensor0_hole() {
  float volts0 = analogRead(sensor0) * 0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13 * pow(volts0, -1);
  // Serial.println(distance0);
  // test ret;

  //4.7 to 5.81 is our average calibrated range
  if(distance0 >= 2 && distance0 < 6.82) {
    // ret.hole = 0;
    // ret.dist = distance0;
    return 0;
  } // 7.83 to 8.72
  else if(distance0 >= 6.82 && distance0  < 9.5){
    // ret.hole = 1;
    // ret.dist = distance0;
    return 1;
  }

  // 10.27 to 11
  else if(distance0 >= 9.5 && distance0 < 11.7){
    // ret.hole = 2;
    // ret.dist = distance0;
    return 2;
  }
  
  // 12.40 to 13.08
  else if(distance0 >= 11.7 && distance0 < 13.7){
    // Serial.println(distance0);
    // ret.hole = 3;
    // ret.dist = distance0;
    return 3;
  }

  // actual 14.33 to 14.88
  else if(distance0 >= 13.7 && distance0 < 15){
    // ret.hole = 4;
    // ret.dist = distance0;
    return 4;
  } else { 
    return -1; 
  }
}

int sensor1_hole() {
  float volts0 = analogRead(sensor1)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 11.5){
    return 2;
  }
  else if(distance0 >= 11.5 && distance0 < 14.5){ // struggling with these two
    // Serial.println(distance0);
    return 3;
  }
  else if(distance0 >= 14.5 && distance0 < 18){ // struggling with these two
    // Serial.println(distance0);
    return 4;
  }
  else{
    return 33;
  }
}

int sensor2_hole() {
  float volts0 = analogRead(sensor2)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 12){
    return 2;
  }
  else if(distance0 >= 12 && distance0 < 15){
    // Serial.println(distance0);
    return 3;
  }
  else if(distance0 >= 15 && distance0 < 18.5){
    // Serial.println(distance0);
    return 4;
  }
  else{
    // Serial.println(distance0);
    return 33;
  }
}

int sensor3_hole() {
  float volts0 = analogRead(sensor3)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13 * pow(volts0, -1);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  } else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  } else if(distance0 >= 9 && distance0 < 12){
    return 2;
  } else if(distance0 >= 12 && distance0 < 15){
    return 3;
  } else if(distance0 >= 15 && distance0 < 19){
    return 4;
  } else {
    return 33; 
  }
}

int sensor4_hole() {
  float volts0 = analogRead(sensor4)*0.0048828125;
  // float distance = 13*pow(volts, -1); // worked out from datasheet graph
  float distance0 = 13*pow(volts0, -1);
  // Serial.println(distance0);

  if(distance0 >= 3 && distance0 < 5.5){
    return 0;
  }
  else if(distance0 >= 5.5 && distance0  < 9){
    return 1;
  }
  else if(distance0 >= 9 && distance0 < 12){
    // Serial.println(distance0);
    return 2;
  }
  else if(distance0 >= 12 && distance0 < 15){
    return 3;
  }
  else if(distance0 >= 15 && distance0 < 20){ // not sure about this one 
    return 4;
  }
  else{
    return 33; 
  }
}

int argmin(int arr[]) {
  int minIndex = 0;
  for (int i = 1; i < 3; i++) {
    if (arr[i] < arr[minIndex]) {
      minIndex = i;
    }
  }
  return minIndex;
}

// int hole_found() {
//   int readings[] = {sensor0_hole(), sensor1_hole(), sensor2_hole(), sensor3_hole(), sensor4_hole()};
//   int col = argmin(readings);
//   int row = readings[col];
//   rowcol output;
//   output.row = row;
//   output.col = col;
//   return output;
// }



int pollSensors() {
  return sensor0_hole();

  // int readings[] = {
  //   sensor0_hole(), 
  //   sensor1_hole(), 
  //   sensor2_hole()
  //   // sensor3_hole(), 
  //   // sensor4_hole()
  // };
  
  // int col = argmin(readings);
  // int row = readings[col];
  
  // if (row == 33) {
  //   return -1;
  // }

  // int out = col * 5 + row;
  
  // delay(50);
  // return out;
  // Serial.println();
  // TODO-Mikayla+Lucy: Do the sensor logic here! For now, mocking out that it always slowly-increasing holes
  // static int returnedHole = 0;
  // static int counter = 0;

  // if(counter < 5) {
  //   counter++;
  // } else {
  //   counter = 0;
  //   returnedHole += 1;
  // }
  
  // return returnedHole;
}

#ifdef TESTING
int pollSensors() {
  return T_HOLE_MADE;

  // col * 5 + row
}
#endif

void calibrate() {
  int thresh = 100000;
  int value = 0;
  float counter = 0;
  while(value < thresh) {
    float volts0 = analogRead(sensor0) * 0.0048828125;
      // float distance = 13*pow(volts, -1); // worked out from datasheet graph
    float distance0 = 13 * pow(volts0, -1);
      // Serial.println(distance0);
      
    counter += distance0;
    value++;
  }

  // counter /= thresh;
  //Serial.println(numreadings);
  Serial.print("OUR READING WAS: ");
  Serial.println(counter / thresh);
  delay(1);
  
}

void loop() {
  // calibrate();
  manageFSM();
}