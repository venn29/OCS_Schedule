import csv

timetotalNS = 1000000000.0
UnitLength = 1820000.0
UnitsNumber = int(timetotalNS/UnitLength)+1

FlowNUM = 64

class FlowCwnd:
    def __init__(self,id,recordlength,UnitLength) -> None:
        self.flowid = id
        self.unitlength = recordlength
        self.UnitLength = UnitLength
        self.cwnds = []
        for i in range(0,UnitsNumber):
            temp = {}
            self.cwnds.append(temp)

    def AddCwndVariation(self,time,cwnd):
        timeNs = time*1000000000
        unitnm = int(timeNs)/self.UnitLength
        cwndinunit = self.cwnds[unitnm]
        cwndinunit[timeNs] = cwnd
    
    def GetCwndVariation(self,unitnum,offset,currentime):
        cwndinunit = self.cwnds[unitnum]
        temp = [0]*self.recordlength
        prevcwnd = 1458
        prevreletivetime = 0
        for item in sorted(cwndinunit.items(),key=lambda d:d[0]):
            time = item[0]
            reletivetime = time - currentime
            if(offset > reletivetime):
                continue
            for i in range(prevreletivetime,reletivetime):
                temp[i] = prevcwnd
            prevcwnd =  item[1]
            prevreletivetime = reletivetime
        for i in range(prevreletivetime,-1):
            temp[i] = prevcwnd
        return temp

            
        

InputPath = './cwnd.csv'
OutputPath = './perflowcwnd.csv'

inputfile = open(InputPath,'r',newline='')
csvreader = csv.reader(inputfile)
flows = {}
for row in csvreader:
    flowid =  int(row[0]) - 20000
    time = float(row[1])
    cwnd = int(row[2])
    if(flowid not in flows.keys()):
        flow = FlowCwnd(flowid)
        flow.AddCwndVariation(time,cwnd)
        flows[flowid] = flow
    else:
        flow = flows[flowid]
        flow.AddCwndVariation(time,cwnd)

outputfile = open(OutputPath,'w',newline='')
csvwriter = csv.writer(outputfile)
prevcwnd = [0]*64
offset = 780000.0
recordlength = 780000.0
for i in range(0,UnitsNumber):
    temp = ['day'+str(i)]
    csvwriter.writerow(temp)
    currenttime = i*int(UnitLength)
    for j in range(0,FlowNUM):
        flow = flows[j]
        cwndvaries = flow.GetCwndVariation(i,offset,currenttime)
        temp = [flow.flowid]+cwndvaries
        csvwriter.writerow(temp)





    