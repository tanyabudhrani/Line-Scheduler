# Line-Scheduler
This Production Line Scheduling (PLS) machine is specifically designed to enhance production planning and maximize the utilization of the three plants of a medium-sized steel manufacturer

Prerequisites to run:
You have a C compiler installed on your machine (We used GCC recommended)

How to run:
1) Use cd to go to the directory where the code is saved
2) gcc -o PLS_G23 PLS_G23.c
3) ./PLS_G23

Sample Input Commands:
* addPERIOD 2024-06-01 2024-06-30
* addORDER P0001 2024-06-10 2000 Product_A
* addBATCH test.txt
* runPLS SJF|printREPORT > test2.txt
* exitPLS
