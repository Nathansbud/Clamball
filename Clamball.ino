#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "WDT.h"

#define DEBUGGING true
#define DEBUG_PRINT(A) (DEBUGGING ? Serial.print(A) : NULL)
#define DEBUG_PRINTLN(A) (DEBUGGING ? Serial.println(A) : NULL)

#define LOCKOUT_PIN 3

/* UNCOMMENT ME TO TEST FSM! */
// #define TESTING
bool networked = true;

// Defines the running average window size each sensor uses
const int NUM_SENSORS = 5;
const int SENSOR_WINDOW = 15;
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
int HEARTBEAT_COUNT = 1;
const int HEARTBEAT_THRESHOLD = 50000;

int LOCKOUT_COUNT = 0;
bool LOCKOUT_SWAP = false;
const int LOCKOUT_FLASH = 50000;
volatile bool LOCKED_OUT = false;

const int TEXT_CYCLE_COUNT = 5;

enum ResponseType {
  SUCCESS = 1,
  FAILURE = 0,
  ERROR = -1
};

enum DeviceState { 
  /* 1 */  ATTEMPTING,  
  /* 2 */  WAITING_FOR_GAME,
  /* 3 */  INITIALIZE_GAME,

  /* 4 */  WAITING_FOR_BALL,
  /* 5 */  BALL_SENSED,
  /* 6 */  UPDATE_COVERAGE,
  /* 7 */  SEND_UPDATE,
  
  /* 9 */  GAME_WIN,
  /* 10 */ GAME_LOSS,
  /* 11 */ GAME_END,

  /* 12 */ HOLE_LOCKOUT,
  /* 13 */ SEND_HEARTBEAT,
  
  /* 14 */ GAME_RESET,
};

// For testing:
int T_CABINET_NUMBER_B;
int T_CABINET_NUMBER_A;
int T_START;
bool T_LOCKED_OUT;
int T_ACTIVE_ROW;
int T_HEARTBEAT_COUNTER;
int T_HEARTBEAT_COUNTER_A;
int T_RESPONSE_HU;
bool T_LOCKED_OUT_A;
int T_ACTIVE_ROW_A;
bool T_CHECK_HEARTBEAT;
int T_LOCKOUT_COUNTER;
int T_LOCKOUT_COUNTER_A;
int T_RESPONSE_HB;
DeviceState T_STATE_DS;
// int T_WATCHDOG; 
// int T_MILLIS;
// int T_WDT_MILLIS;
// int T_TOT_ELAPSED;

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

uint32_t lockoutState[3] = {
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

void refreshLockoutPattern() {
  for(int i = 0; i < 3; i++) {
    lockoutState[i] = boardState[i];
  }

  int holes[8][2] = {
    {2, 11},
    {2, 10},
    {2, 9},
    {2, 8},
    {2, 7},
    {3, 7},
    {4, 7},
    {5, 7}
  }; 
  

  for(int i = 0; i < 8; i++) {
    uint8_t index1D = holes[i][0] * 12 + holes[i][1];
    uint8_t newRow = index1D / 32;
    uint8_t newCol = index1D % 32;
  
    lockoutState[newRow] |= (1 << (31 - newCol));
  }
}


void toggleLockoutPattern(bool enable) {
  if(enable) {
    matrix.loadFrame(lockoutState);
  } else {
    matrix.loadFrame(boardState);
  }
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

void resetGame() {
  activeRow = -1;
  activeHole = -1;
  HEARTBEAT_COUNT = 1;

  LOCKOUT_COUNT = 0;
  LOCKOUT_SWAP = false;
  LOCKED_OUT = false;
  
  clearBoardState();
  initializeSensors();

  matrix.loadFrame(boardState);
}

void lockoutISR() {
  // Allow regular operations to resume; if we are in HOLE_LOCKOUT, we will return to WAITING_FOR_BALL
  LOCKOUT_COUNT = 0;
  LOCKOUT_SWAP = false;
  LOCKED_OUT = false;
  toggleLockoutPattern(false);
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

void setup() {  
  Serial.begin(115200);
  
  #ifndef TESTING
  if(networked) {
    setupWifi();
  }
  #endif
  
  setupWdt();
  
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
    return GAME_RESET;
  }
}

bool checkHeartbeat() {
  HEARTBEAT_COUNT = (HEARTBEAT_COUNT + 1) % HEARTBEAT_THRESHOLD;
  #ifdef TESTING
  T_HEARTBEAT_COUNTER_A = HEARTBEAT_COUNT;
  return T_CHECK_HEARTBEAT;
  #else
  return networked && HEARTBEAT_COUNT == 0;
  #endif
}


// WDT Zone;
// Credit: https://github.com/nadavmatalon/WatchDog/blob/master/examples/WatchDog_Uno_Example/WatchDog_Uno_Example.ino
const long wdtInterval = 5000;

void manageFSM() {
  static int candidateHole = -1;
  bool hitDetected = false;
  
  #ifndef TESTING
  // Reset the timer on our watchdog at the top of each FSM loop; this isn't exceptional practice,
  // but works as a broad stroke to catch that none of the network functions (setupServer, sendHoleUpdate, sendHeartbeat)
  // hung during the last iteration; if they did, something is wrong with our server, and we should have reset
  WDT.refresh();
  #else
  activeState = T_STATE_DS;
  activeRow = T_ACTIVE_ROW;
  if (activeRow != -1) {
    hitDetected = true;
  }
  HEARTBEAT_COUNT = T_HEARTBEAT_COUNTER;
  LOCKOUT_COUNT = T_LOCKOUT_COUNTER;
  CABINET_NUMBER = T_CABINET_NUMBER_B;
  LOCKED_OUT = T_LOCKED_OUT;
  #endif

  switch(activeState) {
    case ATTEMPTING:
      // If our server setup did not provide our cabinet with an ID to use, we self-loop back to keep trying again; 
      // the server might not be started, or our Arduino might not actually be connected to WiFi yet
      if(CABINET_NUMBER == -1) { // Transition: 1-1 [ATTEMPTING -> ATTEMPTING]
        setupServer();
        activeState = ATTEMPTING;
      } else { // Transition: 1-2 [ATTEMPTING -> WAITING_FOR_GAME]
        activeState = WAITING_FOR_GAME;
      }
      break;
    case WAITING_FOR_GAME:
      // This is a bit of an odd model, but it's a pain to have the Arduino acting as client AND server,
      // without some sort of threading model that might necessitate an OS; as such, we use polling even in contexts where we'd
      // ideally have the server send a request to the client
      switch(checkShouldStart()) {
        case SUCCESS: // Transition: 2-3 [WAITING_FOR_GAME -> INITIALIZE_GAME]
          DEBUG_PRINTLN("Let the games...begin!");
          activeState = INITIALIZE_GAME;
          break;
        case FAILURE: // Transition: 2-2 [WAITING_FOR_GAME -> WAITING_FOR_GAME]
          DEBUG_PRINTLN("Waiting for game...");
          activeState = WAITING_FOR_GAME;
          break;
        case ERROR:
        default:
          DEBUG_PRINTLN("Erroring :(");
          break;
      }
      
      delay(1000);
      break;
    case INITIALIZE_GAME:
      clearBoardState();
      matrix.loadFrame(boardState);
      activeState = WAITING_FOR_BALL; // Transition: 3-4 [INITIALIZE_GAME -> WAITING_FOR_BALL]
      break;
    case WAITING_FOR_BALL: {
      // If we are currently in LOCKED_OUT mode, we should not actually be here; instead, place us in HOLE_LOCKOUT at the next transition
      if(LOCKED_OUT) { // Transition: 4-12 [WAITING_FOR_BALL -> HOLE_LOCKOUT]
        activeState = HOLE_LOCKOUT;
        break;
      }
      
      #ifndef TESTING
      // The polling logic of waiting for ball is to be constantly polling our IR sensors; 
      // they are finicky, we don't actually consider a reading on them gospel. Instead, we keep a running average of their readings
      updateSensorAverages();

      // Check for any inputs on our push buttons; these mean a guaranteed hit has occurred!
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
      #endif

      if(hitDetected) { // Transition: 4-5 [WAITING_FOR_BALL -> BALL_SENSED]
        activeState = BALL_SENSED;
      } else if(checkHeartbeat()) { // Transition: 4-13 [WAITING_FOR_BALL -> SEND_HEARTBEAT]
        activeState = SEND_HEARTBEAT;
      } else { // Transition: 4-4 [WAITING_FOR_BALL -> WAITING_FOR_BALL]
        activeState = WAITING_FOR_BALL;
      }
      break;
    }
    case BALL_SENSED: {
      // Compute the active column based on running averages maintained in WAITING_FOR_BALL
      int activeColumn = computeActiveColumn();
      activeHole = 5 * activeRow + activeColumn;

      DEBUG_PRINT("Got: ");
      DEBUG_PRINT(activeRow);
      DEBUG_PRINT(" - ");
      DEBUG_PRINTLN(activeColumn);

      activeState = UPDATE_COVERAGE; // Transition: 5-6 [BALL_SENSED -> UPDATE_COVERAGE]
      break;
    }
    case UPDATE_COVERAGE:
      activateHole(activeHole);
      activeState = SEND_UPDATE; // Transition: 6-7 [UPDATE_COVERAGE -> SEND_UPDATE]
      break;
    case SEND_UPDATE: {
      int response = sendHoleUpdate(CABINET_NUMBER, activeHole);
      #ifdef TESTING
      response = T_RESPONSE_HU;
      #endif
      
      DEBUG_PRINT("Got response after sending hole: ");
      DEBUG_PRINTLN(response);

      // The game has not ended!
      if(response == -1) {
        // We are now "locked out", which means we should be placed into HOLE_LOCKOUT until further notice...eventually
        refreshLockoutPattern(); 
        LOCKED_OUT = true;

        // Reset hole metadata!
        activeHole = -1;
        activeRow = -1;
    
        // Update all sensors to have cleared values
        initializeSensors();
      }
      
      // This state transition is guaranteed to leave SEND_UPDATE;
      // it moves either to WAITING_FOR_BALL (no winner), GAME_WIN/GAME_LOSS (winner), or GAME_RESET (server error) 
      activeState = checkWinnerTransition(response); // Transition: 7-4 [SEND_UPDATE -> WAITING_FOR_BALL], 7-9 [SEND_UPDATE -> GAME_WIN], 7-10 [SEND_UPDATE -> GAME_LOSS], 7-14 [SEND_UPDATE -> GAME_RESET]
      break;
    }
    case GAME_WIN:
      #ifndef TESTING
      for(int i = 0; i < TEXT_CYCLE_COUNT; i++) {
        displayMessage("winner, winner, chicken dinner!", 150);
        // This operation is quite slow and blocking; make sure to refresh the WDT just in case each loop
        WDT.refresh();
      }
      #endif
      activeState = GAME_END; // Transition: 9-11 [GAME_WIN -> GAME_END]
      break;
    case GAME_LOSS:
      #ifndef TESTING
      for(int i = 0; i < TEXT_CYCLE_COUNT; i++) {
        displayMessage("loser, loser, lemon snoozer!", 150);
        // This operation is quite slow and blocking; make sure to refresh the WDT just in case each loop
        WDT.refresh();
      }
      #endif
      activeState = GAME_END; // Transition: 10-11 [GAME_LOSS -> GAME_END]
      break;
    case GAME_END:
      activeState = GAME_RESET; // Transition: 11-14 [GAME_END -> GAME_RESET]
      break;
    case HOLE_LOCKOUT:
      if(!LOCKED_OUT) { // Transition: 12-4 [HOLE_LOCKOUT -> WAITING_FOR_BALL]
        activeState = WAITING_FOR_BALL;
      } else {
        if(checkHeartbeat()) { // Transition: 12-13 [HOLE_LOCKOUT -> SEND_HEARTBEAT]
          activeState = SEND_HEARTBEAT;
        } else { // Transition: 12-12 [HOLE_LOCKOUT -> HOLE_LOCKOUT]
          if(LOCKOUT_COUNT == 0) { 
            LOCKOUT_SWAP = !LOCKOUT_SWAP;
            toggleLockoutPattern(LOCKOUT_SWAP);
          }

          LOCKOUT_COUNT = (LOCKOUT_COUNT + 1) % LOCKOUT_FLASH;
          activeState = HOLE_LOCKOUT;
        }
      }
      break;
    case SEND_HEARTBEAT: {
      // Heartbeat serves the dual purpose of checking server alive, and polling for a winner;
      // response is one of: -2 (server error), -1 (no winner), 0, ..., 99 (winning ID)
      int timeStart = millis();
      
      int response = sendHeartbeat();
      DEBUG_PRINT("Heartbeat Response: ");
      DEBUG_PRINTLN(response);

      DEBUG_PRINT("Took: ");
      DEBUG_PRINT(millis() - timeStart);
      DEBUG_PRINTLN(" ms");

      // This state transition is guaranteed to leave SEND_UPDATE;
      // it moves either to WAITING_FOR_HOLE (no winner), GAME_WIN/GAME_LOSS (winner), or GAME_RESET (server error) 
      activeState = checkWinnerTransition(response); // Transition: 13-4 [SEND_HEARTBEAT -> WAITING_FOR_BALL], 13-9 [SEND_HEARTBEAT -> GAME_WIN], 13-10 [SEND_HEARTBEAT -> GAME_LOSS], 13-14 [SEND_HEARTBEAT -> GAME_RESET]
      break;
    }
    case GAME_RESET:
      resetGame();
      activeState = ATTEMPTING; // Transition: 14-1 [GAME_RESEST -> ATTEMPTING]
      break;
    default:
      activeState = GAME_RESET;
      break;
  }

  #ifdef TESTING
  T_STATE_DS = activeState;
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

    // Reference: https://www.instructables.com/How-to-Use-the-Sharp-IR-Sensor-GP2Y0A41SK0F-Arduin/;
    float sensorRead = analogRead(pin) * 0.0048828125;
    float sensorDistance = 13 * pow(sensorRead, -1);

    // Post-demo, post-having the energy to spend any more time on this project note:
    // Wonder if the inaccuracy we ultimately faced was because this referenced code just seems to go for a "fuck it, we ball" 
    // approach to reading the sensors, and because we referenced this code during prototyping, we never interrogated our
    // sensor readings as the source of our strife. See https://github.com/guillaume-rico/SharpIR/blob/master/SharpIR.cpp for 
    // an example of more "robust" processing. Maybe the answer was to just use this library all along, as "13" might have been
    // an approximation of 12.08 * 1.058 = 12.78...we may never know.

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
