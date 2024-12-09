from server import Cabinet, GamePattern
from utils import red, green
'''
Run all tests
'''
def run_all_tests():
    for i in range(7):
        res = globals()[f"server_test_{i}"]()
        print(f"Server Test {i}: {green('PASS') if res else red('FAIL')}")

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

if __name__ == "__main__":
    run_all_tests()
