# iRAM

(c) David T. Nguyen, 
November 2015,
dunguk@gmail.com

iRAM is a service for maximizing the amount of available memory in order to improve the overall performance
of the device. This is not simple, since overly aggressive policy will degrade the performance itself. iRAM provides a set of various 
configurations that allows us choose the best policy. This includes the aggression level, minimal memory threshold, heap size, etc. 
Since Android invokes its garbage collection right before issuing the OutOfMemoryError, iRAM 
gives us an excellent tool to manage the level of available memory in a more efficient way that avoids such errors and provides superior 
end-user performance.
 
This file explains from the higher level how iRAM works, for more details please see our presentation slides. 
iRAM consists of several key modules: Config module, Priority Manager, Cron module, and Heap Manager. Roughly,
the Cron module manages the execution of the system. Once it starts, iRAM configures the variables as required in the Config.
If the amount of free memory is below the predefined threshold, it starts cleaning the processes from the lowest to the highest priority.
This cleaning is managed by the Priority Manager. Certain processes related to high-priority products are given the highest priority.
Once obtained PIDs of the 'unwanted' processes, they are killed. The list of custom top priority apps/processes is included in the Whitelist.

There are three levels of aggression, level 1 is the lowest (cleans mainly background processes), level 2 (background processes 
+ system caches), 
and level 3 (background processes + system caches + foreground processes). 

To run iRAM once:
	./start.sh

To run iRAM indefinitely:
	./cron.sh

The Heap Manager maintains the global limit of the heap size available to each application/process. The module tracks the run-time
heap usage of selected apps with the highest priority (listed in heap_tracking_list.dat). If the heap usage of any high priority 
apps reaches the limit, the limit is increased incrementally by 10%. 
The module also dynamically decreases the global max heap limit based on the process with the highest heap usage.
It finds such a process with the highest heap usage, and decreases the max heap limit to the value that is 10% higher than the top usage value. 
This way the module keeps a margin for each process to grow its heap usage. 

To run heap management module
	./cron_heap.sh

===================

To evaluate iRAM's performance without PERF test:
        ./evaluate.sh               
                                    
The evaluation includes launching 5 applications repeatedly after a predefined period, and running iRAM as a background service. The
applications are being launched, and 100 user events are issued.                        
                                                                                                                                         
To log current memory levels:                                                                                                            
        ./log.sh output_file_name.txt 

To log current CPU levels:
	./log_cpu.sh output_file_name.txt

To log current battery levels with/without iRAM:
	./log_battery.sh output_file_name.txt


To evaluate iRAM's performance with PERF test:                                                                                       
        ./evaluate_perf.sh


	      
