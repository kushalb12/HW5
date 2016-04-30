# Problem
* Implement a failure detector using a gossip-based heartbeat algorithm of nodes.
* The program has to contain a server thread that listens for UDP connections and the main thread that sends UDP messages or sleeps as described below. Be careful to protect shared variables that require protection!

### Functionalities
*Following the process in the problem description

### Instructions to run
#### Compile and run test file
1. Compile the program with Makefile: make clean make
2. Running test file: ./p5 N b c F B P S T
	- number of peer nodes N,
	- gossip parameter b,
	- gossip parameter c,
	- number of seconds after which a node is considered dead F,
	- number of bad nodes that should fail B,
	- number of seconds to wait between failures P,
	- the seed of the random number generator S,
	- the total number of seconds to run T.

## Created files:
- p4.h
- p4_main.c
- p4_server.c
- Makefile, README.txt

### Example
1. Start different shells on same/different m/cs.
2. Cd to a common directory path. (e.g. /fakepath/users/k/kbansal/h5p4)
3. Remove any old 'endpoints' file.
4. compile code (p4.h, p4_main.c, p4_server.c)
 gcc -pthread  -o p4 p4_main.c p4_server.c -g
5. Run (for N=2)
./p4 2 1 5 2 1 5 100 8
(N=2, b=1, c=5, F=2, B=1, P=5, S=100, T=8)
on both the machines.
6. Expected output:
(N=2, b=1, c=5, F=2, B=1, P=5, S=100, T=8)
list0:
OK
Idx:0,HB:11,TS:10,OK
Idx:1,HB:4,TS:3,FAIL

list1:
FAIL
Idx:0,HB:4,TS:4,OK
Idx:1,HB:11,TS:10,FAIL

(N=3, b=2, c=5, F=2, B=1, P=5, S=100, T=8)
list0:
OK
Idx:0,HB:11,TS:10,OK
Idx:1,HB:10,TS:8,OK
Idx:2,HB:5,TS:3,FAIL
list1:
OK
Idx:0,HB:10,TS:9,OK
Idx:1,HB:11,TS:10,OK
Idx:2,HB:5,TS:4,FAIL
list2:
FAIL
Idx:0,HB:5,TS:4,OK
Idx:1,HB:5,TS:4,OK
Idx:2,HB:11,TS:10,FAIL
