import csv
import numpy as np
BaseFCTPath = './BaseSmallFlowGoodput.csv'
FCTResultPath = './SmallFlowGoodput.csv'
OutputFile = './NormalizedFCTs.csv'
statisticFile = 'NormalFCTStatistics.csv'

class Flow:
    def __init__(self,dstport,duration,totalGP,finished) -> None:
        self.dstport = dstport
        self.duration = duration
        self.totalGP = totalGP
        self.NormalizedFCT = 1
        self.finished = finished
    
    def NLFCT(self,fct,GP):
        #pkt only finished
        if self.finished ==  True:
            self.NormalizedFCT = self.duration/fct
        else:
            if self.totalGP == 0:
            	self.NormalizedFCT = -1
            	return
            finishtime = self.duration*GP*1.0/self.totalGP
            self.NormalizedFCT = finishtime/fct
            self.duration = finishtime

baseflows = {}
resultflows = {}
NLFCTs = []
FCTs = []
with open(BaseFCTPath,'r') as basefile:
    reader = csv.reader(basefile)
    for row in reader:
        dstport = int(row[3])
        startime = float(row[4])
        endtime = float(row[5])
        duration = float(row[6])
        finished = row[7]
        GoodPut = int(row[8])
        if(finished == 'True'):
            baseflows[dstport] = Flow(dstport,duration,GoodPut,True)
        else:
            baseflows[dstport] = Flow(dstport,endtime-startime,GoodPut,False)

with open(FCTResultPath,'r') as resultfile:
    reader = csv.reader(resultfile)
    for row in reader:
        dstport = int(row[3])
        startime = float(row[4])
        endtime = float(row[5])
        duration = float(row[6])
        finished = row[7]
        GoodPut = int(row[8])
        if(finished == 'True'):
            resultflows[dstport] = Flow(dstport,duration,GoodPut,True)
        else:
            resultflows[dstport] = Flow(dstport,endtime-startime,GoodPut,False)

outfile = open(OutputFile,'w',newline='')
writer = csv.writer(outfile)

for item in baseflows.items():
    dstport = item[0]
    baseflow = item[1]
    resultflow = resultflows[dstport]
    if(baseflow.finished == True):
        resultflow.NLFCT(baseflow.duration,baseflow.totalGP)
    if(resultflow.NormalizedFCT == -1):
    	continue
    NLFCTs.append(resultflow.NormalizedFCT)
    FCTs.append(resultflow.duration)
    temp = []
    temp.append(dstport)
    temp.append(baseflow.duration)
    temp.append(baseflow.totalGP)
    temp.append(resultflow.duration)
    temp.append(resultflow.totalGP)
    writer.writerow(temp)



a = np.array(NLFCTs)
print(np.mean(a))
print(np.percentile(a,99))
print(np.percentile(a,99.9))
b = np.array(FCTs)
print(np.mean(b))
print(np.percentile(b,99))
print(np.percentile(b,99.9))

