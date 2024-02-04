# Proof Of Concept: SystemVerilog Co-Emulation

Note that this repo is simply a Proof of Concept demonstration, the testbench is very bare bones,
and the hardware DUT is actually another computer(in my case an R-PI) listening on an ethernet port
and mimicking how the simple hardware would act. Spoofing the DUT in this way has no implications on 
the testbench code, as it's simply an opaque network socket form testbench perspective regardless.

## Value Proposition
This repo demonstrates that, utilizing SystemVerilog DPI, co-emulation testbenches (i.e. testbenches that
run the same tests for both RTL simulation & on-board hardware tests) can be easily written in pure
SystemVerilog.

This solves two main problems:
1. Duplicating testbench logic across simulation and hardware testbenches.
2. The need to introduce software scripting languages to your test code, for hardware tests (e.g. TCL, python).

The approach here is also seperated enough in it's architecture that it lends itself to
mostly parallel development of the test logic and the C-lang network code, where hypothetical
verification engineers and software engineers only need to agree on the dpi function signatures,
as the common API, minimizing communication overhead.

## Architecture
Delagating all DUT related code to the SystemVerilog interface
means that, through a simple macro parameter only utilized
within said interface, the underlying implementation of the
methods defined in the interface can change.

When in simulation mode, the interface implements it's functions
and tasks by interacting directly with the DUT.
While, when in hardware test mode, the interface implements
the functions through dpi calls, which themselves interact with
the DUT via a network socket (here presumed to be an ethernet cable)

![projectarchitecture](./diagrams/sv_coemulation_architecture.png)

dpi2socket.c contains:
1. Implementation of all dpi functions.
2. Linux socket api used to send raw packets over ethernet
3. Txn in and txn out queues

Essentially, when the testbench starts, dpi2socket sets up
a seperate pthread for the code which pushes to and polls from
the linux network socket.

This process polls the txn in queue and sends it's contents over
the network.
It then polls the socket, and on succesful read: pushes what
it has read to the txn out queue.

txn out queue is in turn polled by the system verilog interface.

![dpi2socketarchitecture](./diagrams/dpi2socket_c_architecture.png)

## REPO structure
`dut` directory contains the code to spoof a dut on the other end of an ethernet cable
`testbench` directory contains the testbench and corresponding dpi etc. as well as the
DUT RTL

## Build and run
Need a simulator installed, the build scripts assume verilator, which is opensource and available for free.
However this code should run with any commericial simulator.

### Requirements
1. At least 2 computers, in my case I used my thinkpad x220 as the testbench host and a raspberry pi as the DUT spoofer
2. Ethernet cable
3. Linux OS, the code uses the linux socket api and POSIX threads  (could probably work on any unix like system but I'm not sure)
4. [Verilator](https://github.com/verilator/verilator)

### Setup
1. Connect your two computers with an ethernet cable
2. run `git clone https://github.com/Guiltybyte/SystemVerilogCoEmulation_POC`
3. copy the `./testbench/` directory to the testbench host machine
4. copy the `./dut` directory to the other machine (can simply clone the repo onto both machines or use something like scp to copy files over ssh)
5. On testbench machine:  
```
make simulation
```  
This runs the simulation test, you should see some simple info on stdout

6. On dut spoof machine:

```
cd dut
gcc *c
sudo ./a.out
```
You should see: "entering read loop" on stdout.
Leave the program running.

7. 
```
make hardware
```
This will build and run the hardware version of the testbench, 
you should see lots of info on stdout, and if you look at the
terminal on your DUT spoofer machine, you should see that it
received data from the socket it was polling and sent it back 
across the cable and that this is what the testbench is reading 
back and printing to stdout

### Future Work

The C code here has the potential to be fully general to any testbench.
e.g. as a library.

This would require the following:  

1. An ethernet packet protocol. This would have to defined in
such a way that it is: generalisable to any bit length of addresses & values,
can send an arbitrary number of addresses & values in a single packet while
(in the case that max packet size limit is reached) can define that next packet
is part of same transfer.

2. Ability to spawn and manage arbitrary number of txn queues for each interface as specified in 
   the systemverilog testbench (e.g. some designs may have 2 input "interfaces" and a single output interface
   where the input interfaces are independant of one another, this would require that the c-code maintain
   two input queues and a single output one, wheras another design may have 1 input and 1 output interface
   which would require one queue each for input and output)

With the ethernet packet protocol defined, making an RTL design which can parse said protocol
and map it to the registers of the design under test would be the final step, removing the need
for spoofing a dut on another computer, and finally having a solution that could co-emulate a real
design.
