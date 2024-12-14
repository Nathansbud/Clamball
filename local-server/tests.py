from server import Cabinet, GamePattern
from utils import red, green
import server

'''
Run all tests
'''
def run_all_tests():
    TESTS = {k: f for k, f in globals().items() if k.startswith("server_test_")}
    for test_name, test in enumerate(TESTS):
        try:
            RESULT = test()
        except:
            RESULT = None
        
        print(f"{test_name} Result: {green('PASS') if {RESULT} else red('FAIL')}")

''' 
Elements to test for server: 
   - test successfully connecting...is this possible?
   - allows two connections 
   - can receive/send messages  

   - correct row,col is received when hole is made 

   - test repl 
'''
## Test did_win: vertical win 
def server_test_0():
    cab0 = Cabinet(0)
    for i in range(5):
        cab0.update_hole(i, 0)
    res = cab0.did_win(4,0)
    return res==True

## Test did_win: horizontal win 
def server_test_1():
    cab0 = Cabinet(0)
    for i in range(5):
        cab0.update_hole(0, i)
    res = cab0.did_win(0,4)
    return res==True

## Test did_win: down diagonal win 
def server_test_2():
    cab0 = Cabinet(0)
    for i in range(5):
        cab0.update_hole(i, i)
    res = cab0.did_win(4,4)
    return res==True

## Test did_win: up diagonal win 
def server_test_3():
    cab0 = Cabinet(0)
    for i in range(5):
        cab0.update_hole(i, 4-i)
    res = cab0.did_win(0,4)
    return res==True

## Test did_win: blackout win 
def server_test_4():
    cab0 = Cabinet(0)
    for i in range(5):
        for j in range(5):
            cab0.update_hole(i, j, GamePattern.BLACKOUT)
    res = cab0.did_win(4,4)
    return res==True

## Test did_win: no win LINE
def server_test_5():
    cab0 = Cabinet(0)
    for i in range(3):
        cab0.update_hole(i, 0)
    res = cab0.did_win(2,0)
    return res==False

## Test did_win: no win BLACKOUT
def server_test_6():
    cab0 = Cabinet(0)
    for i in range(5):
        cab0.update_hole(i, 0)
    res = cab0.did_win(4,0, GamePattern.BLACKOUT)
    return res==False

## Test handle_register_cabinet:
def server_test_7():
    rh = server.RequestHandler()
    rh.path = "/register-cabinet"
    server.NEXT_CABINET = 0
    server.ACTIVE_CABINETS = {}
    rh.do_GET()
    # check next cabinet 
    # check cab created 
    return (server.NEXT_CABINET == 1) and (len(server.ACTIVE_CABINETS) == 1) and (list(server.ACTIVE_CABINETS)[0].cabinet_id == 0) and (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "R0")

## Test handle_request_start: no start
def server_test_8():
    rh = server.RequestHandler()
    rh.path = "/request-start"
    server.GAME_STARTED = False 
    rh.do_GET()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "N")

## Test handle_request_start: yes start
def server_test_9():
    rh = server.RequestHandler()
    rh.path = "/request-start"
    server.GAME_STARTED = True 
    rh.do_GET()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "Y")

## Test sendHeartbeat(): winner NONE
def server_test_10():
    rh = server.RequestHandler()
    rh.path = "/heartbeat"
    server.WINNER = None 
    rh.do_GET()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "N")

## Test sendHeartbeat(): winner not NONE
def server_test_11():
    rh = server.RequestHandler()
    rh.path = "/heartbeat"
    server.WINNER = 0 
    rh.do_GET()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "W0")

## Test handle_hole_update(): inactive cabinet
def server_test_12():
    rh = server.RequestHandler()
    rh.path = "/hole-update"
    server.T_CABINET = 0
    server.ACTIVE_CABINETS = {}
    rh.do_POST()
 
    return (server.T_RESPOND_STATUS == 401) and (server.T_RESPOND_CONTENT == "Invalid cabinet")

## Test handle_hole_update(): old winner
def server_test_13():
    rh = server.RequestHandler()
    rh.path = "/hole-update"
    server.T_CABINET = 0
    server.create_cabinet(server.T_CABINET)
    T_ROW = 0
    T_COL = 0
    server.WINNER = 1
    rh.do_POST()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "W1")

## Test handle_hole_update(): new winner
def server_test_14():
    rh = server.RequestHandler()
    rh.path = "/hole-update"
    server.T_CABINET = 0
    server.create_cabinet(server.T_CABINET)
    T_ROW = 0
    T_COL = 0
    server.WINNER = None
    server.ACTIVE_CABINETS[server.T_CABINET].holes = [[False, True, True, True, True], [False, False, False, False, False], [False, False, False, False, False], [False, False, False, False, False], [False, False, False, False, False]]
    rh.do_POST()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "W0")

## Test handle_hole_update(): no winner
def server_test_15():
    rh = server.RequestHandler()
    rh.path = "/hole-update"
    server.T_CABINET = 0
    server.create_cabinet(server.T_CABINET)
    T_ROW = 0
    T_COL = 0
    server.WINNER = None
    rh.do_POST()
 
    return (server.T_RESPOND_STATUS == 200) and (server.T_RESPOND_CONTENT == "N")

## Test fallback(): do_GET
def server_test_16():
    rh = server.RequestHandler()
    rh.path = "/oops"
    rh.do_GET()
 
    return (server.T_RESPOND_STATUS == 404) and (server.T_RESPOND_CONTENT == "ya done goofed shawty")

## Test fallback(): do_POST
def server_test_17():
    rh = server.RequestHandler()
    rh.path = "/oops"
    rh.do_POST()
 
    return (server.T_RESPOND_STATUS == 404) and (server.T_RESPOND_CONTENT == "ya done goofed shawty")

## Test repl_loop(): game settings
def server_test_18():
    server.T_POLL = 0
    server.ACTIVE_PATTERN == server.GamePattern.BLACKOUT
    server.GAME_STARTED = False
    server.ACTIVE_CABINETS = {}
    server.NEXT_CABINET = 0
    server.WINNER = None
    server.repl_loop()
    return (server.ACTIVE_PATTERN == server.GamePattern.LINE) and (server.GAME_STARTED == False) and (server.ACTIVE_CABINETS == {}) and (server.NEXT_CABINET == 0) and (server.WINNER == None)

## Test repl_loop(): start game
def server_test_19():
    server.T_POLL = 1
    server.ACTIVE_PATTERN == server.GamePattern.LINE
    server.GAME_STARTED = False
    server.ACTIVE_CABINETS = {}
    server.NEXT_CABINET = 0
    server.WINNER = None
    server.repl_loop()
    return (server.ACTIVE_PATTERN == server.GamePattern.LINE) and (server.GAME_STARTED == True) and (server.ACTIVE_CABINETS == {}) and (server.NEXT_CABINET == 0) and (server.WINNER == None)

## Test repl_loop(): reset all
def server_test_20():
    server.T_POLL = 2
    server.ACTIVE_PATTERN == server.GamePattern.LINE
    server.GAME_STARTED = True
    server.create_cabinet(0)
    server.create_cabinet(1)
    server.NEXT_CABINET = 2
    server.WINNER = 0
    server.repl_loop()
    return (server.ACTIVE_PATTERN == server.GamePattern.LINE) and (server.GAME_STARTED == False) and (server.ACTIVE_CABINETS == {}) and (server.NEXT_CABINET == 0) and (server.WINNER == None)

if __name__ == "__main__":
    run_all_tests()
