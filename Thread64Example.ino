/**
   USE OF THIS SOFTWARE IS GOVERNED BY THE TERMS AND CONDITIONS
   OF THE LICENSE STATEMENT AND LIMITED WARRANTY FURNISHED WITH
   THE PRODUCT.
   <p/>
   IN PARTICULAR, YOU WILL INDEMNIFY AND HOLD ITS AUTHOR, ITS
   RELATED ENTITIES AND ITS SUPPLIERS, HARMLESS FROM AND AGAINST ANY
   CLAIMS OR LIABILITIES ARISING OUT OF THE USE, REPRODUCTION, OR
   DISTRIBUTION OF YOUR PROGRAMS, INCLUDING ANY CLAIMS OR LIABILITIES
   ARISING OUT OF OR RESULTING FROM THE USE, MODIFICATION, OR
   DISTRIBUTION OF PROGRAMS OR FILES CREATED FROM, BASED ON, AND/OR
   DERIVED FROM THIS SOURCE CODE FILE.
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Background thread example
//
// This library is designed to be used in scenarios whee one foreground and one background thread is required
// e.g. when part of the calculations are heavy and need to be done in background based on asynchronous fashion without
// affecting the foreground processing working with gauges and communication


//////////////////////////////////////////////////////////////////////////////////////
// MAIN

#include "Thread64.h"

// this variable will be set by the background and will be read from the foreground
int32_t shared_background_count = 0;

void  backgroud()
{
  // calculate the value of shared_background_count
  // do some work here

  // accessing shared variable
  // if the provided T64_SET & T64_GET templates cannot do the job for you
  // you can build your own - the only thing is that when you access a shared variable 
  // you need to do it between:  noInterrupts(); ... access it..; interrupts();
  T64_SET(random(8606), shared_background_count);
}



void setup()
{
  // Init Serial
  Serial.begin(115200);
  for (; !Serial;) {};

  Serial.println("Thread64 TEST");

  // Thread64 ( background function & background stack size )
  T64_INIT(backgroud, 256);

}

void loop()
{
  Serial.println("Shared : " + String(T64_GET(shared_background_count)));
  Serial.println("Timer : " + String(T64_TIMER_GET()));
  Serial.println("Background max stack : " + String(T64_BACKGROUND_MAX_STACK_GET()));
  Serial.println();
  

  delay(1000);
}

