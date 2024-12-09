#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

#define DEBUGGING true
#define DEBUG_PRINT(A) (DEBUGGING ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUGGING ? Serial.println(A) : NULL)

#define sensor0 A0
#define sensor1 A1
#define sensor2 A2
#define sensor3 A3
#define sensor4 A4

/* UNCOMMENT ME TO TEST FSM! */
// #define TESTING

bool networked = false;
bool production = false;

// Defines the running average window size each sensor uses
const int NUM_SENSORS = 5;
const int SENSOR_WINDOW = 10;

const int TOTAL_INDEX = SENSOR_WINDOW;
const int AVG_INDEX = SENSOR_WINDOW + 1;

// Store the readings per sensor, + total [-2] and average [-1]
float sensorReadings[NUM_SENSORS][SENSOR_WINDOW + 2];

// Index to use for our sensor moving average
int readIndex = 0;

// Wire up the push buttons on pins D0-D10
const int NUM_BUTTONS = 10;

// Sensed row, based on button being read; coupled with sensors to get a fairly accurate picture of the correct hole
int activeRow = -1;
int activeHole = -1;

// Counter used to trigger heartbeat messages, once HEARTBEAT_COUNT == HEARTBEAT_THRESHOLD - 1;
// the larger the heartbeat threshold, the less likely it is to interfere with pin functionality
int HEARTBEAT_COUNT = 0;
const int HEARTBEAT_THRESHOLD = 50;

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
  SEND_HEARTBEAT,
  BALL_SENSED,
  
  UPDATE_COVERAGE,
  SEND_UPDATE,

  HOLE_LOCKOUT,

  GAME_TIE,
  GAME_WIN,
  GAME_LOSS,

  GAME_END
};

DeviceState activeState = networked ? ATTEMPTING : INITIALIZE_GAME;
ArduinoLEDMatrix matrix;

// We need to assign our cabinet number via WiFi request
int CABINET_NUMBER = -1;

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

volatile int x = 0;

void doAThing() {
  x = 100;
}

void setup() {
  Serial.begin(115200);
  
  // Put each digital pin into INPUT_PULLUP, so we can get away with fewer wires on our buttons
  for(int i = 0; i < NUM_BUTTONS; i++) {
    // Interrupts can only be enabled on 2 and 3, so we use 13 for 3 instead
    int usePin = i != 3 ? i : i + 10;
    pinMode(usePin, INPUT_PULLUP);
  }

  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(3), doAThing, FALLING);

  // Initialize all sensor readings to 20, which is a little beyond what they should read at the wall
  for(int i = 0; i < NUM_SENSORS; i++) {
    for(int j = 0; j < SENSOR_WINDOW; j++) {
      sensorReadings[i][j] = 20;
    }
    
    sensorReadings[i][TOTAL_INDEX] = 20 * SENSOR_WINDOW;
    sensorReadings[i][AVG_INDEX] = 20;  
  }

  matrix.begin();
  
  // For running unit tests, we don't want to be worrying about WiFi configuration,
  // which slows things down and requires actual integration w our server
  #ifdef TESTING
  testAllTests();
  #else
  if(networked) {
    setupWifi();
  }
  #endif
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
    case HOLE_LOCKOUT:
      // This state is a lockout state until the user manually resets by pressing the retrieve ball button!
      displayMessage("retrieve your ball!", 16);
      break;
    case WAITING_FOR_BALL: {
      // The polling logic of waiting for ball is to be constantly polling our IR sensors; 
      // they are finicky, we don't actually consider a reading on them gospel. Instead, we keep a running average of their readings
      updateSensorAverages(); // TODO: Update averages

      // Check for any inputs on our push buttons; these mean a guaranteed hit has occurred!
      // Every 10 times this is not the case, we send a heartbeat message
      bool hitDetected = false;
      for(int i = 0; i < NUM_BUTTONS; i++)  {
        int usePin = i != 3 ? i : i + 10;
        
        // Each of our pins are on pullup, so a 0 reading is a hit!
        if(digitalRead(usePin) == 0) {
          // Each button sensing plate has 2 buttons on it, wired on pins [0, 1], [2, 3], ..., [8, 9].
          activeRow = i / 2;
          hitDetected = true;
          break;
        }
      }

      if(hitDetected) { 
        activeState = BALL_SENSED;
      } else if(networked) {
        HEARTBEAT_COUNT = (HEARTBEAT_COUNT + 1) % HEARTBEAT_THRESHOLD;
        if(HEARTBEAT_COUNT == HEARTBEAT_THRESHOLD - 1) {
          activeState = SEND_HEARTBEAT;
        }
      } else {
        activeState = WAITING_FOR_BALL;
      }
      break;
    }
    case SEND_HEARTBEAT: {
      // Heartbeat serves the dual purpose of checking server alive, and polling for a winner;
      // response is one of: -2 (server error), -1 (no winner), 0, ..., 99 (winning ID)
      int response = sendHeartbeat();
      Serial.print("Heartbeat Response: ");
      Serial.println(response);
      activeState = checkWinnerTransition(response);
      break;
    }
    case BALL_SENSED: {
      // Compute the active column based on running averages maintained in WAITING_FOR_BALL
      int activeColumn = computeActiveColumn();
      activeHole = 5 * activeRow + activeColumn;

      Serial.print("Got: ");
      Serial.print(activeRow);
      Serial.print(" - ");
      Serial.println(activeColumn);

      activeState = UPDATE_COVERAGE;
      break;
    }
    case UPDATE_COVERAGE:
      activateHole(activeHole);
      activeState = SEND_UPDATE;
      break;
    case SEND_UPDATE: {
      if(networked) {
        int response = sendHoleUpdate(CABINET_NUMBER, activeHole);
      
        Serial.print("Got response after sending hole: ");
        Serial.println(response);

        // TODO: Do we want to clear out sensorReadings here?

        // This state transition is guaranteed to leave SEND_UPDATE;
        // it takes either to WAITING_FOR_HOLE (no winner), GAME_WIN/GAME_LOSS (winner), or ATTEMPTING (server error) 
        activeState = checkWinnerTransition(response);
      } else {
        activeState = HOLE_TIMEOUT;
      }

      // Reset hole metadata!
      activeHole = -1;
      activeRow = -1;
      
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

  #ifdef TESTING
  T_STATE = activeState;
  #endif
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

// Update a running average for each sensor!
void updateSensorAverages() {
  // Iterate over the analog pins our IR sensors are plugged into
  for(int i = 0; i < NUM_SENSORS; i++) {
    int pin = A0 + i;

    float sensorRead = analogRead(pin) * 0.0048828125;
    float sensorDistance = 13 * pow(sensorRead, -1);
    
    // Failsafe to prevent nans from propagating; sometimes our readings are just infinite,
    // and we aren't certain why
    if(isnan(sensorDistance) || isinf(sensorDistance)) {
      sensorDistance = 30;
    }

    float oldValue = sensorReadings[i][TOTAL_INDEX];
    float oldCurrent = sensorReadings[i][readIndex];
    
    sensorReadings[i][TOTAL_INDEX] -= sensorReadings[i][readIndex];
    sensorReadings[i][TOTAL_INDEX] += sensorDistance;
    sensorReadings[i][readIndex] = sensorDistance;
    sensorReadings[i][AVG_INDEX] = sensorReadings[i][TOTAL_INDEX] / SENSOR_WINDOW;
  }

  readIndex = (readIndex + 1) % SENSOR_WINDOW;
}

int computeActiveColumn() {
  int minColumn = 0; 
  float minReading = 100;

  // Find the sensor with the lowest average reading (assuming that any reading below wall corresponds with having hit a hole)
  for(int i = 0; i < NUM_SENSORS; i++) {
    float sensorAvg = sensorReadings[i][AVG_INDEX];
    
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(" - ");
    Serial.println(sensorAvg);
    
    if(sensorAvg < minReading) {
      minColumn = i;
      minReading = sensorAvg;
    }
  }

  return minColumn;
}

void loop() {
  #ifndef TESTING
  manageFSM();
  #endif
}
