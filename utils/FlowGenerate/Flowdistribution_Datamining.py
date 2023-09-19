import random

class FlowSizeDis:
    def __init__(self) -> None:
        self.seed = 1507
        random.seed(self.seed)
    
    def GetFlowsize(self):
        prob = random.random()
        if(prob<=0.1):
            return 180
        elif (prob <= 0.2):
            return 216
        elif (prob <= 0.3):
            return 560
        elif (prob <= 0.4):
            return 900
        elif (prob <= 0.5):
            return 1100
        elif (prob <= 0.6):
            return 1870
        elif (prob <= 0.7):
            return 3160
        elif (prob <= 0.8):
            return 5840
        elif (prob <= 0.9):
            return 10000
        else:
            return 50000