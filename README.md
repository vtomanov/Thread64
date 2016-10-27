# Thread64
Arduino muthithreading (Background thread and timer implementation)

This library is designed to be used in scenarios whee one foreground and one background thread is required
e.g. when part of the calculations are heavy and need to be done in background based on asynchronous fashion without
affecting the foreground processing working with gauges and communication


RUNNING/TESTING INSTRUCTIONS

Make sure you already have Timer1 e.g. the following folder exists:

C:\Program Files (x86)\Arduino\libraries\Timer1

download Thread64) from :

https://github.com/vtomanov

create a folder:

C:\Program Files (x86)\Arduino\libraries\Thread64

and copy the content of Thread64 ( the .h file)  there

create a folder somewhere on your disk with name : Thread64Example

copy the file Thread64Example.ino inside

restart your arduino environment ( to pickup the new libraries)

navigate to the folder Thread64Example and open the file Thread64Example.ino from it.

compile - upload and run - open the serial interface monitor to see the results ( the example assumes you serial monitor is initialised with 115200 if different change the following line in the example : Serial.begin(115200);

that should be all - no changes in includes etc. should be needed
