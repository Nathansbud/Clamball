import threading
import requests

from http.server import HTTPServer, SimpleHTTPRequestHandler
from enum import Enum

from utils import *

# If running local, we are testing requests/responses from our mock client; 
# if not, we are processing incoming requests from Arduino
LOCAL = True

PORT = 6813
DIRECTORY = "."

ACTIVE_CABINETS = {}

def create_cabinet(cid):
    ACTIVE_CABINETS[cid] = Cabinet(cid)

class GamePattern(Enum):
    LINE = 0
    BLACKOUT = 1

class Cabinet:
    def __init__(self, cabinet_id, holes=None):
        self.cabinet_id = cabinet_id
        self.holes = [False for _ in range(5)] * 5 if not holes else holes

        # TODO: hold some connection metadata
        self.connection = None
    
    def update_hole(self, row, col, state=True):
        self.holes[row][col] = state
    
    def did_win(self, pattern = GamePattern.LINE):
        return False
    
class RequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        # Print details of the GET request to the console
        print(f"Received GET request: Path = {self.path}, Headers = {self.headers}")
        if self.path == "/register-cabinet":
            self.register_cabinet()
        else:
            self.fallback()
            
            # Call the parent class's method to handle the GET request as usual
            # super().do_GET()
    
    def do_POST(self):
        print(f"Received POST request: Path = {self.path}, Headers = {self.headers}")
        if self.path == "/hole-update":
            self.handle_hole_update()
        else:
            self.fallback()
        
    def respond(self, status, content=None):
        c = (content if content else "").encode('utf-8')
        c_len = len(c)
            
        self.send_response(status)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", c_len)
        
        # Arduino cannot construct a new http connection every time it needs to process a response
        # self.send_header("Connection", "keep-alive")

        self.end_headers()
        if c_len > 0: 
            self.wfile.write(c)

    def fallback(self):
        print("Attempting to handle fallback...")
        self.respond(404, "ya done goofed shawty")
    
    def register_cabinet(self):
        # TODO: Change this message to register a cabinet ID (maybe hardcode this?)
        create_cabinet(0)

        self.respond(200, "swaggggg")

    def handle_hole_update(self):
        msg_length = self.headers.get("Content-Length", "") or 0
        if msg_length != 3:
            body = self.rfile.read(int(msg_length))
            cabinet, row, col = list(body.decode())
            
            if cabinet not in ACTIVE_CABINETS:
                self.respond(401, "Invalid cabinet")
            else:
                ACTIVE_CABINETS[cabinet].update_hole(row, col)
                
                # TODO: Update w active pattern
                if ACTIVE_CABINETS[cabinet].did_win():
                    self.respond(200, "0")
                else:
                    self.respond(200, "ACK")
        else:
            self.respond(400)

    def debug_heartbeat(self):
        self.respond(200, "okayyyy heartbeat")

def launch_server():
    # This is bad practice, as it accepts all inbound connections; for testing, do this though!
    with HTTPServer(("0.0.0.0", PORT), RequestHandler) as httpd:
        httpd.serve_forever()

class RawMainMenuChoices(Enum):
    GameSettings = "Game Settings"
    StartGame = "Start Game"
    DebugMsg = "Debug Message"

MainMenuChoices = list(RawMainMenuChoices)

def repl_loop():
    def poll(options):
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

    while True:
        response = poll([m.value for m in MainMenuChoices])
        if response:
            chosen = MainMenuChoices[response]            
            if chosen == RawMainMenuChoices.GameSettings:
                print("swag")
            elif chosen == RawMainMenuChoices.StartGame:
                print("swaaaag")
            elif chosen == RawMainMenuChoices.DebugMsg:
                _ = requests.post(
                    "http://localhost:6813/hole-update", 
                    data="abc",
                    timeout=1
                )
            
if __name__ == "__main__":
    # launch a separate thread to manage the messages being passed around 
    message_thread = threading.Thread(target=launch_server)
    message_thread.start()

    repl_loop()
        
    

