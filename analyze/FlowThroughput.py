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
        elif ( dstport >= 20000 and dstport <= 40000):
            self.flowtype = 2
        #acks
        else:
            self.flowtype = 0
        self.throughputlist = [0]*unitnum
        self.interval = interval
    
    def addpacket(self,time,length,seq,ack):
        if(self.flowtype == 0):
            pass
        else:
            unit = int(time/self.interval)
            self.throughputlist[unit]+=length
            if(ack>1 and seq >1):
                self.lastime = time
                self.duration = self.lastime - self.startime
                self.finished = True
    
def cmp(f1,f2):
    if(f1.flowtype == 0 and f2.flowtype != 0 ):
        return 1
    if(f2.flowtype == 0 and f1.flowtype != 0):
        return -1
    if(f1.srcint != f2.srcint):
        if(f1.srcint > f2.srcint):
            return 1
        else:
            return -1
    else:
        if(f1.dstport > f2.dstport):
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
interval = 1400000  
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
with open("FlowThroughput.csv","w",newline='') as csvoutfile:
    csvwriter = csv.writer(csvoutfile)
    vls = sorted(flows.values(),key = functools.cmp_to_key(cmp))
    for flow in vls:
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
        csvwriter.writerow(temp)
