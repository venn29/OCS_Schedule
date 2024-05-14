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
        self.srcport = int(srcport)
        self.dstport = int(dstport)
        #elephant flow
        if(self.dstport>= 10000 and self.dstport < 20000):
            self.flowtype = 1
        #mouse flow
        else: 
            self.flowtype = 2
        self.throughputlist = [0]*unitnum
        self.goodputacklist = [0]*unitnum
        self.interval = interval
        self.goodputlast = 0    
        self.throughputsum = 0

    def AddDataPacket(self,time,length,seq,ack):
        #goodput
        unit = int(time/self.interval)
        self.throughputlist[unit]+=length
        self.throughputsum += length
    
    def AddAckPacket(self,time,length,seq,ack):
        unit = int(time/self.interval)
        if self.goodputacklist[unit] < ack:
            self.goodputacklist[unit] = ack
            self.goodputlast = ack
            self.lastime = time
     
    def FinalPacket(self,time,length,seq,ack):
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
mintime = 13000000
interval = 260000  
totaltime = 100000000
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
                length = int(row[6])
                seq = int(row[7])
                ack = int(row[8])
                
                #flow start, SYN 
                if(seq == 0 and ack ==0):
                    flowid = nodenum+dstport
                    flowidint = int(flowid)
                    srcportint = int(srcport)
                    dstportint = int(dstport)
                    lengthint = int(length)
                    newflow = Flow(flowidint,src,dst,srcportint,dstportint,time,time_unit_num,interval)
                    flows[flowidint] = newflow                
                #SYN and ACK
                elif(ack == 1 and seq ==0):
                    continue
                #data packet

                if time<mintime:
                    continue
                elif (ack == 1 and seq > 0):
                    flowid = nodenum+dstport
                    flowidint = int(flowid)
                    flowget = flows[flowidint]
                    flowget.AddDataPacket(time,length,seq,ack)
                #ack packet
                elif (ack > 1 and seq == 1):
                    flowid = nodenum+srcport
                    flowidint = int(flowid)
                    flowget = flows[flowidint]
                    flowget.AddAckPacket(time,length,seq,ack)
                #final data packet
                elif (ack>1 and seq >1):
                    flowid = nodenum+dstport
                    flowidint = int(flowid)
                    if(flowidint not in flows.keys()):
                        continue
                    flowget = flows[flowidint]
                    flowget.FinalPacket(time,length,seq,ack)

#output
with open("LongFlowThroughput.csv","w",newline='') as csvlongoutfile:
    csvlongwriter = csv.writer(csvlongoutfile)
    csvsmallfile = open("SmallFlowThroughput.csv","w",newline='')
    csvsmallwriter = csv.writer(csvsmallfile)
    csvackfile = open("FlowGoodput.csv","w",newline='')
    csvackwriter = csv.writer(csvackfile)
    csvsmallackfile = open("SmallFlowGoodput.csv","w",newline='')
    csvsmallackwriter = csv.writer(csvsmallackfile)
    vls = sorted(flows.values(),key = functools.cmp_to_key(cmp))
    for flow in vls:
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
            temp.append(flow.throughputsum)
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
            temp.append(flow.throughputsum)
            csvsmallwriter.writerow(temp)
    vls2 = sorted(flows.values(),key = functools.cmp_to_key(cmp2))
    for flow in vls2:
        if int(flow.dstport) > 20000:
            temp = []
            temp.append(flow.src)
            temp.append(flow.dst)
            temp.append(flow.srcport)
            temp.append(flow.dstport)
            temp.append(flow.startime)
            temp.append(flow.lastime)
            temp.append(flow.duration)
            temp.append(flow.finished)
            temp.append(flow.goodputlast)
            csvsmallackwriter.writerow(temp)
        else:
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
            temp.append(flow.goodputlast)
            temp = temp + flow.goodputacklist
            csvackwriter.writerow(temp)    
