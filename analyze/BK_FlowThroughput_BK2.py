import csv
import os
import functools
import ipaddress


class Flow:
    def __init__(self,flowid,src,dst,srcport,dstport,startime,unitnum,interval) -> None:
        self.flowid = flowid
        self.startime = startime
        self.lastime = startime
        self.duration = -1.0
        self.finished = False
        self.src = src
        self.dst = dst
        self.srcint = int(ipaddress.ip_address(self.src))
        self.dstpint = int(ipaddress.ip_address(self.dst))
        self.srcport = srcport
        self.dstport = dstport
        #elephant flow
        if(dstport>= 10000 and dstport < 20000):
            self.flowtype = 1
        #mouse flow
        elif ( dstport >= 20000 and dstport <= 45000):
            self.flowtype = 2
        #acks
        else:
            self.flowtype = 0
        self.throughputlist = [0]*unitnum
        self.goodputacklist = [0]*unitnum
        self.goodputoutlist = [0]*unitnum
        self.interval = interval
    
    def addpacket(self,time,length,seq,ack):
        #goodput
        if(self.flowtype == 0):
            unit = int(time/self.interval)
            if self.goodputacklist[unit] < ack:
                self.goodputacklist[unit] = ack
        else:
            unit = int(time/self.interval)
            self.throughputlist[unit]+=length
            if(ack>1 and seq >1):
                self.lastime = time
                self.duration = self.lastime - self.startime
                self.finished = True

    def goodputfinal(self):
        lenthg = len(self.goodputacklist)
        for i in range (1,lenthg-1):
            if self.goodputacklist[i] == 0:
                self.goodputacklist[i] = self.goodputacklist[i-1]
        for i in range (lenthg-1,0,-1):
            self.goodputacklist[i] = self.goodputacklist[i] - self.goodputacklist[i-1]
    #bug exist, if a flow pause in one week, this program bugs
    #we need another list
            
        
    
def cmp(f1,f2):
    # if(f1.flowtype == 0 and f2.flowtype != 0 ):
    #     return 1
    # if(f2.flowtype == 0 and f1.flowtype != 0):
    #     return -1
    if(f1.dstport > f2.dstport):
        return 1
    else:
        if f1.srcint > f2.srcint :
            return 1
        else:
            return -1

def cmp2(f1,f2):
     if(f1.srcport > f2.srcport):
        return 1
     else:
        if f1.dstpint > f2.dstpint :
            return 1
        else:
            return -1


CsvPath = "./Tscsv"
#根据源到目的地的pair分类
# 10000 < dst port < 20000, elephant flow, uplink
# 20000 < dst port < 40000, mouse flow , uplink
# others, acks
# not have been implemented now 

#unit: Ns
interval = 12740000  
totaltime = 10000000000
time_unit_num = int(totaltime/interval)+1
flows = {}
for root,ds,fs in os.walk(CsvPath):
    for f in fs:
        nodenum = f.split("-")[1]
        with open(CsvPath+"/"+f,"r",newline="") as hostfile:
            csvreader = csv.reader(hostfile)
            for row in csvreader:
                time = float(row[1].split()[1][6:])*1000000000
                src = row[2]
                dst = row[3]
                srcport = row[4]
                dstport = row[5]
                length = row[6]
                seq = row[7]
                ack = row[8]
                flowid = nodenum+dstport
                
                flowidint = int(flowid)
                if(flowidint in flows):
                    flowget = flows[flowidint]
                    flowget.addpacket(time,int(length),int(seq),int(ack))
                else:
                    srcportint = int(srcport)
                    dstportint = int(dstport)
                    lengthint = int(length)
                    newflow = Flow(flowidint,src,dst,srcportint,dstportint,time,time_unit_num,interval)
                    flows[flowidint] = newflow
#output
with open("LongFlowThroughput.csv","w",newline='') as csvlongoutfile:
    csvlongwriter = csv.writer(csvlongoutfile)
    csvsmallfile = open("SmallFlowThroughput.csv","w",newline='')
    csvsmallwriter = csv.writer(csvsmallfile)
    csvackfile = open("FlowGoodput.csv","w",newline='')
    csvackwriter = csv.writer(csvackfile)
    vls = sorted(flows.values(),key = functools.cmp_to_key(cmp))
    for flow in vls:
        if flow.flowtype == 0:
            pass
        if(flow.flowtype == 1):
            temp = []
            temp.append(flow.src)
            temp.append(flow.dst)
            temp.append(flow.srcport)
            temp.append(flow.dstport)
            temp.append(flow.startime)
            temp.append(flow.lastime)
            temp.append(flow.duration)
            temp.append(flow.finished)
            temp = temp + flow.throughputlist
            csvlongwriter.writerow(temp)
        elif flow.flowtype == 2:
            temp = []
            temp.append(flow.src)
            temp.append(flow.dst)
            temp.append(flow.srcport)
            temp.append(flow.dstport)
            temp.append(flow.startime)
            temp.append(flow.lastime)
            temp.append(flow.duration)
            temp.append(flow.finished)
            csvsmallwriter.writerow(temp)
    vls2 = sorted(flows.values(),key = functools.cmp_to_key(cmp2))
    for flow in vls2:
        if flow.flowtype == 0:
            if int(flow.srcport) > 20000:
                continue
            flow.goodputfinal()
            temp = []
            temp.append(flow.src)
            temp.append(flow.dst)
            temp.append(flow.srcport)
            temp.append(flow.dstport)
            temp.append(flow.startime)
            temp.append(flow.lastime)
            temp.append(flow.duration)
            temp.append(flow.finished)
            temp = temp + flow.goodputacklist
            csvackwriter.writerow(temp)
