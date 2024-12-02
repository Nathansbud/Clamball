'''
Run all tests
'''
def run_all_tests():
    pass

''' 
Elements to test for server: 
   - test successfully connecting...is this possible?
   - allows two connections 
   - can receive/send messages  

   - correct row,col is received when hole is made 
   - self.holes is updated properly (update_hole)

   - winning state results in winning message (did_win)
   - non winning state continues game (did_win)
   - tie??

   - test repl 
'''
## Test self.holes is updated properly (update_hole)
def server_test_1():
    pass

'''
Elements to test for cabinets:
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

'''
def cabinet_test_1():
    pass

if __name__ == "__main__":
    run_all_tests()