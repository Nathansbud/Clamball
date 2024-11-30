#include "fsm.h" 

#define CAB1 // have this uncommented for one arduino and commented for the other 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

}

void checkSensors() {
  // update cab1_inp and cab2_inp using outputs from sensors 
}

void loop() {
  // put your main code here, to run repeatedly:
  static cabState CURRENT_STATE = sATTEMPTING;
  checkSensors(); 
  CURRENT_STATE = updateCabFSM(CURRENT_STATE, millis()); // need millis?? 
  delay(10);
}

cabState updateCabFSM(cabState curState, long mils) {
  cabState nextState;
  switch(curState) {
  case sATTEMPTING: // during this state should be sending handshakes...helper function? 
    if (/* received some acknowledgement from server*/) {
      #ifdef CAB1 
      cab1_connect = true;
      #else 
      cab2_connect = true;
      #endif 
      nextState = sWAIT_FOR_GAME;
    } else {
      nextState = sATTEMPTING;
    }
    break;
  case sWAIT_FOR_GAME:
    if (start_game) { /* received start game message from manager*/
      // nextState = sINIT_GAME
      nextState = sWAIT_FOR_BALL;
    } else {
      nextState = sWAIT_FOR_GAME;
    }
    break;
  // case sINIT_GAME:
  //   // not totally sure here...does this automatically go to wait for ball? why have this state? 
  //   nextState = sWAIT_FOR_BALL;
  //   break;
  case sWAIT_FOR_BALL:
    if (c1_inp != NULL || c2_inp != NULL) {
      #ifdef CAB1 
      if (cab1_inp) {
        /* send message to manager about c1_inp made */
        led_matrix1[cab1_inp[0]][cab1_inp[1]] = 1;
      }
      #else 
      if (cab2_inp) {
        /* send message to manager about c2_inp made */
        led_matrix2[cab2_inp[0]][cab2_inp[1]] = 1;
      }
      #endif 
      nextState = sBALL_SENSED;
    // } else if (/* message from server? */) {
    //   nextState = sWAIT_FOR_MESSAGE; // not sure about this transition, could be an interrupt instead? 
    } else {
      nextState = sWAIT_FOR_BALL;
    }
    break;
  case sBALL_SENSED:
    if (game_winner != -1) { 
      if (game_winner == 1){ 
        #ifdef CAB1 
        /* disply WINNER on LED screen*/
        #else 
        /* disply LOSER on LED screen*/
        #endif 
      } else if (game_winner == 2){ 
        #ifdef CAB1 
        /* disply LOSER on LED screen*/
        #else 
        /* disply WINNER on LED screen*/
        #endif 
      } else if (game_winner == 0){ 
        /* disply TIE on LED screen for cab1 */
        /* disply TIE on LED screen for cab2 */
        #endif 
      }
      nextState = sGAME_END;
    } else { 
      c1_inp = NULL;
      c2_inp = NULL;
      nextState = sWAIT_FOR_BALL;
    }
    break;

  // honestly feel like a lot of these don't need to be their own states...we could probs just call them in the 
  // transitions...would simplify fsm a lot 
  // case sUPDATE_COVERAGE:
  // case sSEND_UPDATE:
  // case sGAME_TIE:
  //   // no gaurd here, just sends messages to users and moves to game end 
  //   nextState = sGAME_END;
  //   break;
  // case sGAME_WON:
  //   // no gaurd here, just sends messages to users and moves to game end 
  //   nextState = sGAME_END;
  //   break;
  // case sGAME_LOSS:
  //   // no gaurd here, just sends messages to users and moves to game end 
  //   nextState = sGAME_END;
  //   break;
  // case sGAME_END:
  //   nextState = sWAIT_FOR_GAME;
  //   break;
  // case sWAIT_FOR_MESSAGE: // not sure how we want to handle this part...this is just one option 
  //   if (/* got game won...*/) {
  //     nextState = sGAME_WON;
  //   } else if (/* got game loss...*/) {
  //     nextState = sGAME_LOSS;
  //   } else if (/* got game tie...*/) {
  //     nextState = sGAME_TIE;
  //   } else if (/* got disconnection/problem...*/) {
  //     nextState = sATTEMPTING;
  //   } else {
  //     nextState = sWAIT_FOR_BALL;
  //   }
  //   break;
  // }
  return nextState;
}
