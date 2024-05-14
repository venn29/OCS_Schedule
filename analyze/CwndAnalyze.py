import csv

timetotalNS = 1000000.0
UnitLength = 1820.0
UnitsNumber = int(timetotalNS/UnitLength)+1

FlowNUM = 64

class FlowCwnd:
    def __init__(self,id,recordlength,UnitLength) -> None:
        self.flowid = id
        self.recordlength = recordlength
        self.UnitLength = UnitLength
        self.cwnds = []
        for i in range(0,UnitsNumber):
            temp = {}
            self.cwnds.append(temp)

    def AddCwndVariation(self,time,cwnd):
        timeNs = (int)(time*1000000)
        unitnm = int( int(timeNs)/self.UnitLength)
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
            # if(reletivetime>recordlength):
            #     print(reletivetime)
            if(offset > reletivetime):
                prevcwnd =  item[1]
                continue
            for i in range(prevreletivetime,reletivetime-offset):
                temp[i] = prevcwnd
            prevcwnd =  item[1]
            prevreletivetime = reletivetime-offset
        for i in range(prevreletivetime,self.recordlength-1):
            temp[i] = prevcwnd
        return temp

            
        

InputPath = '../cwnd.csv'
OutputPath = './perflowcwnd.csv'
prevcwnd = [0]*64
offset = 1300
recordlength = 780
inputfile = open(InputPath,'r',newline='')
csvreader = csv.reader(inputfile)
flows = {}
for row in csvreader:
    if not row[0].isdigit():
        continue
    flowid =  int(row[0]) - 20000
    time = float(row[1])
    cwnd = int(row[2])
    if(flowid not in flows.keys()):
        flow = FlowCwnd(flowid,recordlength,UnitLength)
        flow.AddCwndVariation(time,cwnd)
        flows[flowid] = flow
    else:
        flow = flows[flowid]
        flow.AddCwndVariation(time,cwnd)

outputfile = open(OutputPath,'w',newline='')
csvwriter = csv.writer(outputfile)


for i in range(0,UnitsNumber):
    temp = ['day'+str(i)]
    csvwriter.writerow(temp)
    currenttime = i*int(UnitLength)
    for flow in flows.values():
        cwndvaries = flow.GetCwndVariation(i,offset,currenttime)
        temp = [flow.flowid]+cwndvaries
        csvwriter.writerow(temp)





    