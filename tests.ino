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

// const char* csvTestData = "Test,Start,End,Cab#,start,hole_made,response,count\n0,1,1,-1,0,0,0,0\n1,1,2,0,0,0,0,0\n2,2,2,0,0,0,0,0\n3,2,3,0,1,0,0,0\n4,4,5,0,1,0,0,0\n5,4,9,0,1,-1,0,0\n6,4,10,0,1,-1,1,0\n7,4,4,0,1,-1,-1,0\n8,4,1,0,1,-1,-2,0\n9,5,6,0,1,0,0,10\n10,5,4,0,1,0,0,5\n11,7,9,0,1,0,0,0\n12,7,10,0,1,0,1,0\n13,7,4,0,1,0,-1,0\n14,7,1,0,1,0,-2,0"

int testData[15][8] = {
  {0, 1, 1, -1, 0, 0, 0, 0},
  {1, 1, 2, 0, 0, 0, 0, 0},
  {2, 2, 2, 0, 0, 0, 0, 0},
  {3, 2, 3, 0, 1, 0, 0, 0},
  {4, 4, 5, 0, 1, 0, 0, 0},
  {5, 4, 9, 0, 1, -1, 0, 0},
  {6, 4, 10, 0, 1, -1, 1, 0},
  {7, 4, 4, 0, 1, -1, -1, 0},
  {8, 4, 1, 0, 1, -1, -2, 0},
  {9, 5, 6, 0, 1, 0, 0, 10},
  {10, 5, 4, 0, 1, 0, 0, 5},
  {11, 7, 9, 0, 1, 0, 0, 0},
  {12, 7, 10, 0, 1, 0, 1, 0},
  {13, 7, 4, 0, 1, 0, -1, 0},
  {14, 7, 1, 0, 1, 0, -2, 0}
};

extern int T_CABINET_NUMBER;
extern int T_START;
extern int T_HOLE_MADE;
extern int T_RESPONSE;
extern int T_COUNT;
extern int T_STATE;

DeviceState[] states = [ATTEMPTING, WAITING_FOR_GAME, INITIALIZE_GAME, WAITING_FOR_BALL, BALL_SENSED, UPDATE_COVERAGE, SEND_UPDATE, GAME_TIE, GAME_WIN, GAME_LOSS, GAME_END];

bool testTransition(DeviceState startState, DeviceState endState) {
  T_STATE = states[startState-1];
  manageFSM();
  return T_STATE == states[endState-1];
}

const int numTests = 15;

/*
 * Runs through all the test cases defined above
 */
bool testAllTests() {
  for (int i = 0; i < numTests; i++) {
    Serial.print("Running test ");
    Serial.println(testData[i][0]);
    T_CABINET_NUMBER = testData[i][3];
    T_START = testData[i][4];
    T_HOLE_MADE = testData[i][5];
    T_RESPONSE = testData[i][6];
    T_COUNT = testData[i][7];
    if (!testTransition(testData[i][1], ttestData[i][2])) {
      return false;
    }
    Serial.println();
  }
  Serial.println("All tests passed!");
  return true;
}