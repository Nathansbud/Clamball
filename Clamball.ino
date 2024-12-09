#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

#define DEBUGGING true
#define DEBUG_PRINT(A) (DEBUGGING ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUGGING ? Serial.println(A) : NULL)

#define LOCKOUT_PIN 3

/* UNCOMMENT ME TO TEST FSM! */
// #define TESTING
bool networked = true;

// Defines the running average window size each sensor uses
const int NUM_SENSORS = 5;
const int SENSOR_WINDOW = 10;
const int SENSOR_DEFAULT = 30;

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

volatile bool LOCKED_OUT = false;

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

void lockoutISR() {
  // Allow regular operations to resume; if we are in HOLE_LOCKOUT, we will return to WAITING_FOR_BALL
  LOCKED_OUT = false;
  petWDT(); //also pet here when lockout is occuring?
}

void initializeSensors() {
  for(int i = 0; i < NUM_SENSORS; i++) {
    for(int j = 0; j < SENSOR_WINDOW; j++) {
      sensorReadings[i][j] = SENSOR_DEFAULT;
    }
    
    sensorReadings[i][TOTAL_INDEX] = SENSOR_DEFAULT * SENSOR_WINDOW;
    sensorReadings[i][AVG_INDEX] = SENSOR_DEFAULT;  
  }
}

void initWDT() {
  R_WDT->WDTCR = (0b10 | (0b1111 << 4) | (0b11 << 8) | (0b11 << 12)); //set this time according to 
  //increase TOPS to maximum timtout duration (0b1111)
  R_WDT->WDTSR = 0; // clear watchdog status

  //data sheet pg. 580
  //timeout period = (prescalar * timeout count) / clock frequency --> not exactly sure on math
}


/* pet the watchdog */
/*pet dont kick :)*/
void petWDT() {
  R_WDT->WDTRR = 0;
  R_WDT->WDTRR = 0xff;
}

void setup() {
  Serial.begin(115200);

  //check if wdt caused a reset:
  if (R_WDT->WDTSR & 0x01) {  //0x01 is the timeout flag WDTSR register
    // Log the watchdog timeout event
    Serial.println("Watchdog timeout occurred. Restarting the game...");
    // Clear the watchdog timeout flag
    R_WDT->WDTSR = 0;
    //Reset the game or reset the server??
    //in theory would reset here...
  }
  
  // Put each digital pin into INPUT_PULLUP, so we can get away with fewer wires on our buttons
  for(int i = 0; i < NUM_BUTTONS; i++) {
    // Interrupts can only be enabled on 2 and 3, so we use 13 for 3 instead
    int usePin = i != 3 ? i : i + 10;
    pinMode(usePin, INPUT_PULLUP);
  }
  
  pinMode(LOCKOUT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LOCKOUT_PIN), lockoutISR, FALLING);  

  // Initialize running average computations to hold a default, unrealistically high sensor reading value to begin
  initializeSensors();
  
  // For running unit tests, we don't want to be worrying about WiFi configuration,
  // which slows things down and requires actual integration w our server
  #ifdef TESTING
  testAllTests();
  #else
  if(networked) {
    setupWifi();
  }
  #endif

  matrix.begin();
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

bool checkHeartbeat() {
  HEARTBEAT_COUNT = (HEARTBEAT_COUNT + 1) % HEARTBEAT_THRESHOLD;
  return HEARTBEAT_COUNT == HEARTBEAT_THRESHOLD - 1;
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
          DEBUG_PRINTLN("Let the games...begin!");
          activeState = INITIALIZE_GAME;
          break;
        case FAILURE:
          DEBUG_PRINTLN("Waiting for game...");
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
      
      // We should probably still do a heartbeat while locked out, as we can still lose while in this state;
      // also, should double check if displayMessage is blocking (lol)
      if(networked && checkHeartbeat()) {
        activeState = SEND_HEARTBEAT;
      }
      break;
    case WAITING_FOR_BALL: {
      // If we are currently in LOCKED_OUT mode, we should not actually be here; instead, place us in HOLE_LOCKOUT at the next transition
      if(LOCKED_OUT) { 
        activeState = HOLE_LOCKOUT;
        break;
      }

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
      } else if(networked && checkHeartbeat()) {
        activeState = SEND_HEARTBEAT;
      } else {
        activeState = WAITING_FOR_BALL;
      }
      break;
    }
    case SEND_HEARTBEAT: {
      // Heartbeat serves the dual purpose of checking server alive, and polling for a winner;
      // response is one of: -2 (server error), -1 (no winner), 0, ..., 99 (winning ID)
      int response = sendHeartbeat();
      DEBUG_PRINT("Heartbeat Response: ");
      DEBUG_PRINTLN(response);
      activeState = checkWinnerTransition(response);
      break;
    }
    case BALL_SENSED: {
      //pet the watchdog:
      petWDT();
      
      // Compute the active column based on running averages maintained in WAITING_FOR_BALL
      int activeColumn = computeActiveColumn();
      activeHole = 5 * activeRow + activeColumn;

      DEBUG_PRINT("Got: ");
      DEBUG_PRINT(activeRow);
      DEBUG_PRINT(" - ");
      DEBUG_PRINTLN(activeColumn);

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
      
        DEBUG_PRINT("Got response after sending hole: ");
        DEBUG_PRINTLN(response);

        // We are now "locked out", which means we should be placed into HOLE_LOCKOUT until further notice...eventually
        LOCKED_OUT = true;

        // This state transition is guaranteed to leave SEND_UPDATE;
        // it takes either to WAITING_FOR_HOLE (no winner), GAME_WIN/GAME_LOSS (winner), or ATTEMPTING (server error) 
        activeState = checkWinnerTransition(response);
      } else {
        activeState = HOLE_LOCKOUT;
      }

      // Reset hole metadata!
      activeHole = -1;
      activeRow = -1;
  
      // Update all sensors to have cleared values
      initializeSensors();
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

  // Find the sensor with the lowest average reading, assuming that 
  // any reading below wall corresponds with having hit a hole
  for(int i = 0; i < NUM_SENSORS; i++) {
    float sensorAvg = sensorReadings[i][AVG_INDEX];
    
    DEBUG_PRINT("Sensor ");
    DEBUG_PRINT(i);
    DEBUG_PRINT(" - ");
    DEBUG_PRINTLN(sensorAvg);
    
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
