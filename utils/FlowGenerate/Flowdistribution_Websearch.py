import random

class FlowSizeDis:
    def __init__(self) -> None:
        self.seed = 1507
        random.seed(self.seed)
    
    def GetFlowsize(self):
        prob = random.random()
        if(prob<=0.25):
            return 10000
        elif (prob <= 0.3333):
            return 20000
        elif (prob <= 0.5):
            return 30000
        elif (prob <= 0.6667):
            return 50000
        elif (prob <= 0.88):
            return 80000
        elif (prob <= 1):
            return 200000