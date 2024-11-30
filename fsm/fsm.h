#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

typedef enum {
  sATTEMPTING = 1,
  sWAIT_FOR_GAME = 2,
  sINIT_GAME = 3,
  sWAIT_FOR_BALL = 4,
  sBALL_SENSED = 5,
  sUPDATE_COVERAGE = 6,
  sSEND_UPDATE = 7,
  sGAME_TIE = 8,
  sGAME_WON = 9,
  sGAME_LOSS = 10,
  sGAME_END = 11,
  sWAIT_FOR_MESSAGE = 12,
} cabState;

typedef enum {
  sLISTENING = 1,
  sORCHESTRATION = 2,
  sCHECK_FOR_MESSAGE = 3,
  sCHECK_FOR_INPUT = 4,
  sSEND_MESSAGE = 5,
  sHANDLE_MESSAGE = 6,
} manState;

bool cab1_connect = false;
bool cab2_connect = false;

int led_matrix1[5][5] = {0};
int led_matrix2[5][5] = {0};

int c1_inp2[2] = NULL // tuple of row, col where ball fell
int c2_inp[2] = NULL

bool start_game = false; 

// used in the case of a tie...
// long cab1_win = NULL;
// long cab2_win = NULL; 

int game_winner = -1; 
bool repl_input = false;