/* Elements to test for cabinets:
   - board is cleared properly 
   - matrix is updated correctly upon hole made 
   - display is correctly updated when matrix updated (not sure how to test)

   - T1-2: attempting to wait for game --> when connected (cabinet # changes)
   - T2-3: wait for game to init game --> when receive game start 
   - T3-4: init game to wait for ball --> -- 
   - T4-5: wait for ball to ball sensed --> error check pollSensors()
   - T5-6: ball sensed to update coverage 
   - T5-4: ball sensed to wait for ball 
   - T6-7: update coverage to send update --> --
   - T7-8: send update to game win 
   - T7-9: send update to game loss 
   - T7-4: send update to wait for ball 
   - T7-1: send update to attempting 

   - correct winner/loser is displayed 
*/

int testData[27][11] = {
  /* 0 */ {1, 1, -1, 0, false, -1, 10, -1, false, 0, -1},
  /* 1 */ {1, 2, 0, 0, false, -1, 10, -1, false, 0, -1},
  /* 2 */ {2, 2, 0, 0, false, -1, 10, -1, false, 0, -1},
  /* 3 */ {2, 3, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 4 */ {3, 4, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 5 */ {4, 4, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 6 */ {4, 5, 0, 1, false, 0, 10, -1, false, 0, -1},
  /* 7 */ {4, 12, 0, 1, true, -1, 10, -1, false, 0, -1},
  /* 8 */ {4, 13, 0, 1, false, -1, 49999, -1, true, 0, -1},
  /* 9 */ {5, 6, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 10 */ {6, 7, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 11 */ {7, 4, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 12 */ {7, 9, 0, 1, false, -1, 10, 0, false, 0, -1},
  /* 13 */ {7, 10, 0, 1, false, -1, 10, 1, false, 0, -1},
  /* 14 */ {7, 14, 0, 1, false, -1, 10, -2, false, 0, -1},
  /* 15 */ {9, 11, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 16 */ {10, 11, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 17 */ {11, 14, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 18 */ {12, 4, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 19 */ {12, 12, 0, 1, true, -1, 10, -1, false, 0, -1},
  /* 20 */ {12, 12, 0, 1, true, -1, 10, -1, false, 0, -1},
  /* 21 */ {12, 13, 0, 1, true, -1, 10, -1, true, 0, -1},
  /* 22 */ {13, 4, 0, 1, false, -1, 10, -1, false, 0, -1},
  /* 23 */ {13, 9, 0, 1, false, -1, 10, -1, false, 0, 0},
  /* 24 */ {13, 10, 0, 1, false, -1, 10, -1, false, 0, 1},
  /* 25 */ {13, 14, 0, 1, false, -1, 10, -1, false, 0, -2},
  /* 26 */ {14, 1, 0, 1, false, -1, 10, -1, false, 0, -1},
};

int testAfter[27][5] = {
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 11, false, -1, 0},
  {0, 10, false, 0, 0},
  {0, 10, true, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0}, 
  {0, 10, false, -1, 0},
  {0, 10, true, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0}, 
  {0, 11, true, -1, 1},
  {0, 11, true, -1, 1},
  {0, 11, true, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 10, false, -1, 0},
  {0, 1, false, -1, 0},
};

// int wdtTest[4][4] = {
//   {0, 6000, 3000, 0},
//   {0, 7000, 1000, 0},
//   {0, 7000, 1000, 115000},
//   {1, 7000, 1000, 6000},
// };

// int wdtAfter[4][3] = {
//   {3000, 0, 13},
//   {7000, 5000, 13},
//   {7000, 120000, 14},
//   {1000, 0, 13},
// };

DeviceState states[13] = { 
  /* 1 */  ATTEMPTING,  //0
  /* 2 */  WAITING_FOR_GAME, //1
  /* 3 */  INITIALIZE_GAME, //2

  /* 4 */  WAITING_FOR_BALL, //3
  /* 5 */  BALL_SENSED, //4
  /* 6 */  UPDATE_COVERAGE, //5
  /* 7 */  SEND_UPDATE, //6
  
  /* 9 */  GAME_WIN, //7
  /* 10 */ GAME_LOSS, //8
  /* 11 */ GAME_END, //9

  /* 12 */ HOLE_LOCKOUT, //10
  /* 13 */ SEND_HEARTBEAT, //11
  
  /* 14 */ GAME_RESET, //12
};

bool testTransition(int startState, int endState) {
  if (startState <=7) {
    startState -= 1;
  } else {
    startState -= 2;
  }
  if (endState <=7) {
    endState -= 1;
  } else {
    endState -= 2;
  }
  T_STATE_DS = states[startState];

  manageFSM();
  bool passedTest = (
    T_STATE_DS == states[endState] and 
    T_CABINET_NUMBER_A == CABINET_NUMBER and
    T_HEARTBEAT_COUNTER_A == HEARTBEAT_COUNT and 
    T_LOCKED_OUT_A == LOCKED_OUT and 
    T_ACTIVE_ROW_A == activeRow and 
    T_LOCKOUT_COUNTER_A == LOCKOUT_COUNT
  );
  return passedTest;
}

// bool testWDT(int testNum) {
//   sendHeartbeat();
//   int endState = wdtAfter[testNum][2];
//   if (endState <=7) {
//     endState -= 1;
//   } else {
//     endState -= 2;
//   } 
//   bool passedTest = (
//     T_WDT_MILLIS == wdtAfter[testNum][0] and 
//     T_TOT_ELAPSED == wdtAfter[testNum][1] and
//     T_STATE_DS == states[endState]
//   );
//   return passedTest;
// }

const int numFSMTests = 27;
// const int numWDTTests = 4;

/*
 * Runs through all the test cases defined above
 */
extern bool testAllTests() {
  for (int i = 0; i < numFSMTests; i++) {
    Serial.print("Running FSM test ");
    Serial.print(i);
    T_CABINET_NUMBER_B = testData[i][2];
    T_CABINET_NUMBER_A = testAfter[i][0]; 
    T_START = testData[i][3];
    T_LOCKED_OUT = testData[i][4];
    T_ACTIVE_ROW = testData[i][5];
    T_HEARTBEAT_COUNTER = testData[i][6];
    T_HEARTBEAT_COUNTER_A = testAfter[i][1];
    T_RESPONSE_HU = testData[i][7];
    T_LOCKED_OUT_A = testAfter[i][2];
    T_ACTIVE_ROW_A = testAfter[i][3];
    T_CHECK_HEARTBEAT = testData[i][8];
    T_LOCKOUT_COUNTER = testData[i][9];
    T_LOCKOUT_COUNTER_A = testAfter[i][4];
    T_RESPONSE_HB = testData[i][10];
    if (!testTransition(testData[i][0], testData[i][1])) {
      Serial.print(": FAILED ");
      return false;
    } else {
      Serial.print(": PASSED ");
    }
    Serial.println();
  }

  // Note: we originally did our WDT in a way that we could unit test however when we pivoted these tests were no longer feasible
  // for (int j = 0; j < numWDTTests; j++) {
  //   Serial.print("Running WDT test ");
  //   Serial.print(j);
  //   // var sets go here
  //   T_WATCHDOG = wdtTest[j][0];
  //   T_MILLIS = wdtTest[j][1];
  //   T_WDT_MILLIS = wdtTest[j][2];
  //   T_TOT_ELAPSED = wdtTest[j][3];
  //   T_STATE_DS = SEND_HEARTBEAT;
  //   if (!testWDT(j)) {
  //     Serial.print(": FAILED ");
  //     return false;
  //   } else {
  //     Serial.print(": PASSED ");
  //   }
  //   Serial.println();
  // } 

  Serial.println("All tests passed!");
  return true;
}