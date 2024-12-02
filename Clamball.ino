#include "ArduinoGraphics.h"
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_RENESAS)
  #include "Arduino_LED_Matrix.h"
#endif

#define DEBUGGING true
#define DEBUG_PRINT(A) (DEBUGGING ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUGGING ? Serial.println(A) : NULL)
#define NUM_SENSORS 5

enum ResponseType {
  SUCCESS = 1,
  FAILURE = 0,
  ERROR = -1
};

enum DeviceState { 
  ATTEMPTING,
  WAITING_FOR_GAME,
  INITIALIZE_GAME,
  WAITING_FOR_BALL,
  BALL_SENSED,
  
  UPDATE_COVERAGE,
  SEND_UPDATE,

  GAME_TIE,
  GAME_WIN,
  GAME_LOSS,

  GAME_END
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

const int THRESHOLD_COUNT = 3;
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
  switch(activeState) {
    case ATTEMPTING:
      setupServer();      

      // If our server setup did not provide our cabinet with an ID to use, we self-loop back to keep trying again; 
      // the server might not be started, or our Arduino might not actually be connected to WiFi yet
      if(CABINET_NUMBER == -1) { // Transition: ATTEMPTING -> ATTEMPTING
        activeState = ATTEMPTING;
      } else {
        activeState = WAITING_FOR_GAME;
      }
      break;
    case WAITING_FOR_GAME:
      // This is a bit of an odd model, but it's a pain to have the Arduino acting as client AND server,
      // without some sort of threading model that might necessitate an OS; as such, we use polling even in contexts where we'd
      // ideally have the server send a request to the client
      switch(checkShouldStart()) {
        case SUCCESS:
          Serial.println("Let the games...begin!");
          activeState = INITIALIZE_GAME;
          break;
        case FAILURE:
          Serial.println("Waiting for game...");
          activeState = WAITING_FOR_GAME;
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
      break;
    case WAITING_FOR_BALL:
      candidateHole = pollSensors();
      if(candidateHole != -1) {
        Serial.print("Sensed ball in hole: ");
        Serial.println(candidateHole);

        activeState = BALL_SENSED;        
      } else {
        Serial.println("Sensed no ball! Sending heartbeat...");
        // TODO-Mikayla+Lucy: Do we want to clear out the active hole if any sensor reading fails?
        activeHole = -1;
        sensedCount = 0;
        
        // Heartbeat serves the purpose of checking if a winner has been decided by the server,
        // as we need to poll for it; response is one of: -2 (server error), -1 (no winner), 0, ..., 99 (winning ID)
        int response = sendHeartbeat();
        activeState = checkWinnerTransition(response);
      }
      break;
    case BALL_SENSED:
      // If we have already seen this hole, increase the sensedCount;
      // otherwise, we have seen a new hole, so should check that one instead
      if(candidateHole == activeHole) {
        sensedCount += 1;
      } else {
        activeHole = candidateHole;
        sensedCount = 1;
      }
      
      if(sensedCount >= THRESHOLD_COUNT) {
        activeState = UPDATE_COVERAGE;
      } else {
        activeState = WAITING_FOR_BALL;
      }
      break;
    case UPDATE_COVERAGE:
      activateHole(activeHole);
      activeState = SEND_UPDATE;
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

int pollSensors() {
  // TODO-Mikayla+Lucy: Do the sensor logic here! For now, mocking out that it always slowly-increasing holes
  static int returnedHole = 0;
  static int counter = 0;

  if(counter < 5) {
    counter++;
  } else {
    counter = 0;
    returnedHole += 1;
  }
  
  return returnedHole;
}

void loop() {
  manageFSM();
}