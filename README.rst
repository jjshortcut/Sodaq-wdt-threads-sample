.. WDT threads example:

Overview
********

 *  Design and implement a basic system for using a watchdog within a multi-threaded application. 
 *  A system which is able to detect a lockup in any of the threads. All threads report to a watchdog thread which feeds the watchdog
 *  
 *  The watchdog gets triggered by simulating a lockup in thread 2, this is done by pressing Button 1
 *  The user gets feedback by the serial port (baudrate 115200)
