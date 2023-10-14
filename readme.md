## About

This is a module for you to simulate a reconfigurable datacenter network (RDCN) based on ns3.
We defined useful class and methods to simulate the controller, EPS switches and optical circuit switching.
You are allowed to config your own network topology based on this repository, including the routing algorithm, link speed, RDCN schedule algorithm and so on.
With this work, we wish to provide a extension to ns3 which can help developers to simulate RDCN easily.

## Overview

We extended ns3 with RDCN controllers(without openflow module) and RDCN routing algorithms, Which is detailed in our paper.

## Build and run
If you do not have ns-3 installed in your PC, please refer to the following link for installation:

https://www.nsnam.org/docs/release/3.38/tutorial/singlehtml/index.html Installation guide: https://www.nsnam.org/wiki/Installation (Follow the instructions for installation depending on which flavor of Linux you prefer)

Once an example script is made and compiles, to run this script it must be in the scratch directory: To get to this directory run the cmd: cd repos/ns-3-allinone/ns-3-dev/scratch 

### Steps to install RDCNsimulate module in ns-3:

1.download .zip file of the RDCNsimulate module


2.open .zip file and you should see a simple folder named RDCNsimulate


3.Find your ns-3 file system and in the "src" folder there should be a "RDCNsimulate" folder, open that and you should see similar folder names to that of the repository.

4.Look back to where you unziped the downloaded repository files and copy and paste those folders and files into this newly made folder (do not copy the scratch folder)

5.Now, start up the terminal and go to the ns-3-dev directory
 While in this directory build all the ns-3 modules (if unsure use the installation guide to help, I use ubuntu and my command is ./ns3 build)
 
 6.now you can use the RDCNsimulate-module in ns-3 simulations

## Analyze

With the python3 scripts in folder analyze/, user can analyze the performance of your own algorithm.