import os

os.system("rm ./Tscsv/*.pcap")
PcapPath = "../"
TscsvPath = "./Tscsv/"
Prefix = "HO0"
cmdpre = "tshark -r "
cmdtail = " -T fields -e frame.time -e ip.src -e ip.dst -e tcp.srcport -e tcp.dstport -e frame.len -e tcp.seq -e tcp.ack -E separator=, > "+TscsvPath
for root,ds,fs in os.walk(PcapPath):
    for f in fs:
        if(f[0:3] == Prefix and root == "../"):
            print(f)
            cmdline = cmdpre + PcapPath + f + cmdtail + f[2:-7]
            print(cmdline)
            os.system(cmdline)
