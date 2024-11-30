#include "fsm.h" 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

}

void loop() {
  // put your main code here, to run repeatedly:
  static cabState CURRENT_STATE = sATTEMPTING;
  updateInputs(); // ??
  CURRENT_STATE = updateCabFSM(CURRENT_STATE, millis(), ??);
  delay(10);
}

cabState updateCabFSM(cabState curState, long mils, ??) {
  cabState nextState;
  switch(curState) {
  case sATTEMPTING: // during this state should be sending handshakes...helper function? 
    if (/* received some acknowledgement from server*/) {
      nextState = sWAIT_FOR_GAME;
    } else {
      nextState = sATTEMPTING;
    }
    break;
  case sWAIT_FOR_GAME: // need some global var to keep track of message?
    if (/* received start game message from manager*/) {
      nextState = sINIT_GAME
    } else {
      nextState = sWAIT_FOR_GAME;
    }
    break;
  case sINIT_GAME:
    // not totally sure here...does this automatically go to wait for ball? why have this state? 
    nextState = sWAIT_FOR_BALL;
    break;
  case sWAIT_FOR_BALL:
    if (c1_inp != NULL || c2_inp != NULL) {
      // send something to manager so they can update game state 
      // upate matrices? 
      nextState = sBALL_SENSED;
    } else if (/* message from server? */) {
      nextState = sWAIT_FOR_MESSAGE; // not sure about this transition, could be an interrupt instead? 
    } else {
      nextState = sWAIT_FOR_BALL;
    }
    break;
  
  // honestly feel like a lot of these don't need to be their own states...we could probs just call them in the 
  // transitions...would simplify fsm a lot 
  case sBALL_SENSED:
  case sUPDATE_COVERAGE:
  case sSEND_UPDATE:
  case sGAME_TIE:
    // no gaurd here, just sends messages to users and moves to game end 
    nextState = sGAME_END;
    break;
  case sGAME_WON:
    // no gaurd here, just sends messages to users and moves to game end 
    nextState = sGAME_END;
    break;
  case sGAME_LOSS:
    // no gaurd here, just sends messages to users and moves to game end 
    nextState = sGAME_END;
    break;
  case sGAME_END:
    nextState = sWAIT_FOR_GAME;
    break;
  case sWAIT_FOR_MESSAGE: // not sure how we want to handle this part...this is just one option 
    if (/* got game won...*/) {
      nextState = sGAME_WON;
    } else if (/* got game loss...*/) {
      nextState = sGAME_LOSS;
    } else if (/* got game tie...*/) {
      nextState = sGAME_TIE;
    } else if (/* got disconnection/problem...*/) {
      nextState = sATTEMPTING;
    } else {
      nextState = sWAIT_FOR_BALL;
    }
    break;
  }
  return nextState;
}
