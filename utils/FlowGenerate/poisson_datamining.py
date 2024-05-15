import Flowdistribution_Datamining
import csv
import random
import math

class FlowGenerator:
    def __init__(self) -> None:
        self.flowsize = Flowdistribution_Datamining.FlowSizeDis()
        self.path = "./FlowTrace_data.csv"
        self.endtime = 1e9
        self.flow_per_second = 10000
        self.hostnum = 64
        random.seed(15078)
    
    def GenerateSizeTrace(self):
        timenow = 0.0
        x = 0
        totalbyte = 0
        f = open(self.path,'w',newline='')
        writer = csv.writer(f)
        while(timenow <= self.endtime):
            intervalue = -math.log(random.random()) / (self.flow_per_second*1.0 / 1e9)
            intertime = int(intervalue)
            timenow += intertime
            # fsize = self.flowsize.GetFlowsize()
            fsize = 500
            self.RegisterFlow(timenow,fsize,writer,x%self.hostnum)
            x += 1
            totalbyte += fsize
        print(x)
        print(totalbyte)


    def RegisterFlow(self,time,flowsize,writer,counter):
        temp = []
        temp.append(int(time))
        temp.append(counter)
        temp.append(flowsize)
        writer.writerow(temp)

def main():
    fg = FlowGenerator()
    fg.GenerateSizeTrace()

if  __name__ == '__main__':
    main()

        