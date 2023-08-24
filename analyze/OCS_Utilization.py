# we wanna output a form include every device's utilization in every day
import decimal
import csv
class DeviceUtil:
    utils = []
    def __init__(self,deviceno,timeunits,daylen,daynightlen) -> None:
        self.deviceno = deviceno
        self.timeunits = timeunits
        self.daylen = daylen
        self.daynightlen = daynightlen
        self.worktimes = [0.0]*timeunits
    def CalculateUtil(self):
        self.utils = [i/self.daylen for i in self.worktimes]


totaltime = 10000000000   #this is 10 second,unit is ns
daylen = 180000
daynightlen = 200000
time_units = int(totaltime/daynightlen)
print(time_units)
devicenum = 8
Devices = []
for i in range(0,devicenum):
    Devices.append(DeviceUtil(i,time_units,daylen,daynightlen))
pkt_lenth = 1500
ocs_transfertime = 200
with open('../ocs.tr','r') as f:
    for line in f.readlines():
        datas = line.split()
        #out put start
        if(datas[0] == '-'):
            time = decimal.Decimal(datas[1])
            timens = int(time*1000000000)
            timeunit_no = int(timens/daynightlen)
            infos = datas[2].split('/')
            deviceno = int(infos[4])
            device_temp = Devices[deviceno]
            device_temp.worktimes[timeunit_no] += 200
for dev in Devices:
    dev.CalculateUtil()

with open("OcsUtil.csv","w",newline='') as csvf:
    csvwriter = csv.writer(csvf)
    name = []
    name.append("TimeUnit")
    for i in range(0,devicenum):
        name.append(str(i))
    csvwriter.writerow(name)
    for i in range(0,time_units):
        tempr = []
        tempr.append(i)
        for j in range(0,devicenum):
            tempr.append(Devices[j].utils[i])
        csvwriter.writerow(tempr)


# print(Devices[0].worktimes)

