void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

}

void loop() {
  // put your main code here, to run repeatedly:
  static manState CURRENT_STATE = sLISTENING;
  updateInputs(); // ??
  CURRENT_STATE = updateManFSM(CURRENT_STATE, millis(), ??);
  delay(10);

}

manState updateManFSM(manState curState, long mils, ??) {
  manState nextState;
  switch(curState) {
    case sLISTENING:
      if (cab1_connect && cab2_connect) {
        nextState = sORCHESTRATION; 
      } else {
        nextState = sLISTENING;
      }
      break;
    case sORCHESTRATION:
      if (/* game params are set - receive user input */) {
        // send game start message to all cabs 
        nextState = sCHECK_FOR_MESSAGE;
      } else {
        nextState = sORCHESTRATION; 
      }
      break;
    case sCHECK_FOR_MESSAGE:
      if (/* no message from cabs */) {
        nextState = sCHECK_FOR_INPUT;
      } else if (/* there is a message from cabs */) {
        nextState = sSEND_MESSAGE;
      }
    case sCHECK_FOR_INPUT:
      if (/* no input from REPL*/) {
        nextState = sCHECK_FOR_MESSAGE;
      } else if (/* there is input from REPL*/ ) {
        nextState = sSEND_MESSAGE;
      }
    case sSEND_MESSAGE:
      if (/* message from cab */) {
        // send message to user 
      } else if (/* message from user */) {
        // send message to cabinets 
      } 

      if (/* game not over */ ) { 
        nextState = sCHECK_FOR_MESSAGE;
      } else {
        nextState = sORCHESTRATION;
      }
      break;
  }
  return nextState;
}