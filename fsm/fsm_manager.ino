void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

}

void loop() {
  // put your main code here, to run repeatedly:
  static manState CURRENT_STATE = sLISTENING;
  // updateInputs(); // ??
  CURRENT_STATE = updateManFSM(CURRENT_STATE, millis()); // need millis?? 
  delay(10);

}

int sum(int array[]) {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    total += array[i];
  }
  return total;
}

// return 0 if TIE, 1 if cab1 won, 2 if cab2 won, or -1 if no one won/tie 
int checkStatus() {
  bool cab1 = false;
  bool cab2 = false;
  int tot1;
  int tot2;

  // checking rows for both cabs 
  for (int row =0; row < 5; row++) {
    tot1 = sum(led_matrix1[row]);
    tot2 = sum(led_matrix2[row]);
    if tot1 == 5 && tot2 != 5 { 
      // winner 
      cab1 = true;
    } else if tot1 != 5 && tot2 == 5 {
      cab2 = true;
    } else if tot1 == 5 && tot2 == 5 {
      cab = true;
      cab2 = true;
    }
  }

  // checking cols for both cabs 
  for (int col = 0; col < 5; col++) {
    tot1 = sum([led_matrix1[0][col], led_matrix1[1][col], led_matrix1[2][col], led_matrix1[3][col], led_matrix1[4][col]);
    tot2 = sum([led_matrix2[0][col], led_matrix2[1][col], led_matrix2[2][col], led_matrix2[3][col], led_matrix2[4][col]);
    if tot1 == 5 && tot2 != 5 { 
      // winner 
      cab1 = true;
    } else if tot1 != 5 && tot2 == 5 {
      cab2 = true;
    } else if tot1 == 5 && tot2 == 5 {
      cab = true;
      cab2 = true;
    }
  }

  // checking diags for both cabs 
  tot1 = sum([led_matrix1[0][0], led_matrix1[1][1], led_matrix1[2][2], led_matrix1[3][3], led_matrix1[4][4]);
  tot2 = sum([led_matrix2[0][0], led_matrix2[1][1], led_matrix2[2][2], led_matrix2[3][3], led_matrix2[4][4]);
  if tot1 == 5 && tot2 != 5 { 
    // winner 
    cab1 = true;
  } else if tot1 != 5 && tot2 == 5 {
    cab2 = true;
  } else if tot1 == 5 && tot2 == 5 {
    cab = true;
    cab2 = true;
  }

  tot1 = sum([led_matrix1[0][4], led_matrix1[1][3], led_matrix1[2][2], led_matrix1[3][1], led_matrix1[4][0]);
  tot2 = sum([led_matrix2[0][4], led_matrix2[1][3], led_matrix2[2][2], led_matrix2[3][1], led_matrix2[4][0]);
  if tot1 == 5 && tot2 != 5 { 
    // winner 
    cab1 = true;
  } else if tot1 != 5 && tot2 == 5 {
    cab2 = true;
  } else if tot1 == 5 && tot2 == 5 {
    cab = true;
    cab2 = true;
  }

  // checking for winners 
  if (cab1 && !cab2) {
    return 1;
  } else if (cab2 && !cab1) {
    return 2; 
  } else if (cab1 && cab2) {
    return 0;
  } else {
    return -1;
  }
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
      if (relp_input) { /* game params are set - receive user input */
        start_game = true; // send game start message to all cabs 
        nextState = sCHECK_FOR_MESSAGE;
      } else {
        nextState = sORCHESTRATION; 
      }
      break;
    case sCHECK_FOR_MESSAGE:
      if (/* no message from cabs */) {
        nextState = sCHECK_FOR_INPUT;
      } else if (/* there is a message from cabs */) {
        nextState = sHANDLE_MESSAGE;
      }
    case sCHECK_FOR_INPUT:
      if (/* no input from REPL*/) {
        nextState = sCHECK_FOR_MESSAGE;
      } else if (/* there is input from REPL*/ ) {
        nextState = sSEND_MESSAGE; 
        // TRIGGER ISR?? IS THE ONLY WAY WE GET HERE IF THE USER IS ENDING THE GAME?
      }
    case sSEND_MESSAGE:
      if (/* message from user */) {
        // send message to cabinets 
      } 
      // if (/* game not over */ ) { 
      //   nextState = sCHECK_FOR_MESSAGE;
      // } else {
      //   nextState = sORCHESTRATION;
      // }
      break;
    case sHANDLE_MESSAGE:
      // message will be about someone making it in a hole 
      if ( /* player 1 made */ ) {
        /* Display their new matrix on screen */
        game_winner = checkStatus();
      } else if (/* player 2 made */) {
        /* Display their new matrix on screen */
        game_winner = checkStatus();
      } else if (/* both made */) {
        /* Display their new matrices on screens */
        game_winner = checkStatus();
      }
  }
  return nextState;
}