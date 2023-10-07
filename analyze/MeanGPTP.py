import csv
import numpy as np

throuputfilename = 'LongFlowThroughput.csv'
goodputfilename = 'FlowGoodput.csv'

TPFile = open(throuputfilename,'r',newline='')
tpreader = csv.reader(TPFile)
i = 0
interval = 49
count = 0
for row in tpreader:
    i += 1
    if i != 65:
        continue
    totalTP = 0
    TPlist = []
    start = 13
    index = 0
    while start+index < len(row):
        totaltemp = int(row[start+index])
        totaltemp += int(row[start+index+1])
        TPlist.append(totaltemp)
        index += interval
    meantp = np.mean(TPlist)
    print(meantp)

GPFile = open(goodputfilename,'r',newline='')
gpreader = csv.reader(GPFile)
i = 0
count = 0
for row in gpreader:
    i += 1
    if i != 65:
        continue
    totalGP = 0
    start = 13
    index = 0
    while start+index < len(row):
        totaltemp = int(row[start+index])
        totaltemp += int(row[start+index+1])
        totalGP += totaltemp
        count += 1
        index += interval
    meangp = totalGP*1.0/count
    print(meangp)

