import threading
import requests

from http.server import HTTPServer, SimpleHTTPRequestHandler
from enum import Enum

from utils import *

# If running local, we are testing requests/responses from our mock client; 
# if not, we are processing incoming requests from Arduino
LOCAL = True
SURPRESS_SYSTEM_LOGS = False
SURPRESS_SERVER_LOGS = False

ADDRESS = "localhost" if LOCAL else "10.37.117.156"
PORT = 6813
DIRECTORY = "."

CABINET_LOCK = threading.Lock()
NEXT_CABINET = 0
ACTIVE_CABINETS = {}

WIN_LOCK = threading.Lock()
GAME_STARTED = False
WINNER = None

# comment or uncomment as needed
# TESTING = False
TESTING = True

T_RESPOND_STATUS = -1 # status
T_RESPOND_CONTENT = "" # content 
T_CABINET = -1 
T_ROW = -1 
T_COL = -1
T_POLL = None

def request_url(endpoint):
    return f"http://{ADDRESS}:{PORT}/{endpoint}"

def create_cabinet(cid):
    ACTIVE_CABINETS[cid] = Cabinet(cid)

class GamePattern(Enum):
    LINE = "5-in-a-Row"
    BLACKOUT = "Blackout"

ACTIVE_PATTERN = GamePattern.LINE

class Cabinet:
    def __init__(self, cabinet_id, holes=None):
        self.cabinet_id = cabinet_id
        self.holes = [[False for _ in range(5)] for _ in range(5)] if not holes else holes

    def update_hole(self, row, col, state=True):
        self.holes[row][col] = state
    
    def did_win(self, row, col, pattern = GamePattern.LINE):
        if pattern == GamePattern.LINE:
            # Per RO-K, we check:
                # All holes of a given row index
                # All holes of a given column index
                # All holes having equal row and column index (i.e. the downward-sloping central diagonal)
                # All holes with row + column index = 4 (i.e. the upward-sloping central diagonal)

            # Check each column in this row
            for c in range(5):
                if not self.holes[row][c]: break
            else:
                return True

            # Check each row in this column
            for r in range(5):
                if not self.holes[r][col]: break
            else:
                return True
            
            # Check each hole in the downward diagonal,
            # (0, 0), (1, 1), ..., (4, 4)
            if row == col: 
                for dd in range(5):
                    if not self.holes[dd][dd]: break
                else:
                    return True

            # Check each hole in the upward diagonal,
            # (0, 4), (1, 3), ..., (4, 0)
            if row + col == 4:
                for ud in range(5):
                    if not self.holes[ud][4 - ud]: break
                else:
                    return True
                
        elif pattern == GamePattern.BLACKOUT:
            for i in range(5):
                if sum(self.holes[i]) != 5: break
            else:
                return True 
    
        return False
    
class RequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)
    
    def log_message(self, format: str, *args) -> None:
        if SURPRESS_SYSTEM_LOGS:
            return         
        
        return super().log_message(format, *args)

    def do_GET(self):
        # Print details of the GET request to the console
        if not SURPRESS_SERVER_LOGS:
            print(green(f"Received GET request: Path = {self.path}, Headers = {self.headers}"))
        
        if self.path == "/register-cabinet":
            self.handle_register_cabinet()
        elif self.path == "/request-start":
            self.handle_request_start()
        elif self.path == "/heartbeat":
            self.sendHeartbeat()
        else:
            self.fallback()
            
            # Call the parent class's method to handle the GET request as usual
            # super().do_GET()
    
    def do_POST(self):
        if not SURPRESS_SERVER_LOGS:
            print(blue(f"Received POST request: Path = {self.path}, Headers = {self.headers}"))

        if self.path == "/hole-update":
            self.handle_hole_update()
        else:
            self.fallback()
        
    def respond(self, status, content=None):
        if not TESTING:
            c = (content if content else "").encode('utf-8')
            c_len = len(c)
                
            self.send_response(status)
            self.send_header("Content-Type", "text/plain")
            
            if c_len: self.send_header("Content-Length", c_len)
            
            # Arduino cannot construct a new http connection every time it needs to process a response
            # self.send_header("Connection", "keep-alive")

            self.end_headers()
            if c_len: self.wfile.write(c)
        else:
            T_RESPOND_STATUS = status
            T_RESPOND_CONTENT = content

    def fallback(self):
        print("Attempting to handle fallback...")
        self.respond(404, "ya done goofed shawty")
    
    def handle_register_cabinet(self):
        # Acquire a lock on the cabinet index variable, in case two cabinets request at the same time;
        # I don't know if this lock is strictly necessary, but..........just in case
        CABINET_LOCK.acquire()
        
        global NEXT_CABINET
        
        active = NEXT_CABINET
        create_cabinet(NEXT_CABINET)
        
        NEXT_CABINET += 1
        
        CABINET_LOCK.release()
        
        # Message starting with R tells a cabinet that it should send all future messages 
        # with the digit sent
        self.respond(200, f"R{active}")

    def handle_request_start(self):
        # Would prefer this to be a 401, but 401 seems to break things for the wee 'duino...
        # not sure why, but it causes a hang. Very odd.
        if not GAME_STARTED: self.respond(200, "N")
        else:
            self.respond(200, "Y")

    def handle_hole_update(self):
        global WINNER
        if not TESTING:
            msg_length = self.headers.get("Content-Length", "") or 0
            if msg_length != 3:
                body = self.rfile.read(int(msg_length)).decode()
                _cabinet, _idx = body.split("-")
                
                print(f"Raw: {body}")

                cabinet, row, col = (int(v) for v in (_cabinet, _idx[0], _idx[1]))
                print(f"Received: {cabinet} @ ({row}, {col})")


                if cabinet not in ACTIVE_CABINETS:
                    self.respond(401, "Invalid cabinet")
                else:                
                    ACTIVE_CABINETS[cabinet].update_hole(row, col)
                    WIN_LOCK.acquire()
                    
                    if WINNER is not None:
                        self.respond(200, f"W{WINNER}")
                    elif ACTIVE_CABINETS[cabinet].did_win(row, col, ACTIVE_PATTERN):
                        WINNER = cabinet
                        self.respond(200, f"W{cabinet}")
                    else:
                        self.respond(200, "N")

                    WIN_LOCK.release()
                    
            else:
                self.respond(400)
        else: 
            cabinet = T_CABINET
            row = T_ROW
            col = T_COL

            if cabinet not in ACTIVE_CABINETS:
                self.respond(401, "Invalid cabinet")
            else:                
                ACTIVE_CABINETS[cabinet].update_hole(row, col)
                
                if WINNER is not None:
                    self.respond(200, f"W{WINNER}")
                elif ACTIVE_CABINETS[cabinet].did_win(row, col, ACTIVE_PATTERN):
                    WINNER = cabinet
                    self.respond(200, f"W{cabinet}")
                else:
                    self.respond(200, "N")

    def sendHeartbeat(self):
        if WINNER is not None:
            self.respond(200, f"W{WINNER}")
        else:
            self.respond(200, "N")

def launch_server(port):
    # This is bad practice, as it accepts all inbound connections; for testing, do this though!
    with HTTPServer(("0.0.0.0", port), RequestHandler) as httpd:
        httpd.serve_forever()

class MainMenuChoices(Enum):
    GameSettings = "Game Settings"
    StartGame = "Start Game"
    ResetAll = "Reset All"

L_MainMenuChoices = list(MainMenuChoices)
L_GamePatterns = list(GamePattern)

def repl_loop():
    global GAME_STARTED, NEXT_CABINET, WINNER, ACTIVE_PATTERN

    def poll(options):
        if not TESTING:
            while True:
                for i, c in enumerate(options, start=1):
                        print(f"{bold(i)}. {c}")
        
                inp = input("> ")   
                try: 
                    choice = int(inp)
                    idx = choice - 1
                    
                    if not ((0 <= idx <= len(options) - 1) or choice == -1):
                        raise ValueError()
                    
                    return idx if choice != -1 else None
                except:
                    print(f"Invalid choice (must be between {bold('1')} and {bold(len(options))})!")
                    continue
        else:
            return T_POLL

    while True:
        choice = poll([m.value for m in L_MainMenuChoices])
        if choice != None:
            chosen = L_MainMenuChoices[choice]            
            if chosen == MainMenuChoices.GameSettings:
                pattern = poll([m.value for m in L_GamePatterns])
                if pattern is not None:
                    ACTIVE_PATTERN = L_GamePatterns[pattern]
                    print(f"Setting game pattern to {bold(ACTIVE_PATTERN.value)}!")
                else:
                    print(f"No pattern selected; keeping pattern unchanged ({bold(ACTIVE_PATTERN.value)})")
            elif chosen == MainMenuChoices.StartGame:
                GAME_STARTED = True
            elif chosen == MainMenuChoices.ResetAll:
                CABINET_LOCK.acquire()
                
                ACTIVE_CABINETS.clear()
                NEXT_CABINET = 0
                WINNER = None
                GAME_STARTED = False
                
                CABINET_LOCK.release()

if __name__ == "__main__":
    # launch a separate thread to manage the messages being passed around
    c1_thread = threading.Thread(target=lambda: launch_server(port=6813))
    c1_thread.start()    
    
    repl_loop()
        
    

