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


void setup()
{
  // Init Serial
  Serial.begin(115200);
  for (; !Serial;) {};

  Serial.println("Thread64 TEST");

  // Thread64 ( background function & background stack size )
  T64_INIT(background_1, 256);

}

////////////////////////////////////////////////////////////////////////////////
//foreground function1 - executes maximum once every 1 second
int32_t function1_lastExecute;
// 1 sec in 10's of millis
#define function1_timeout  (100L)

void function_1()
{
  if (!T64_DO_EXECUTE(T64_TIMER_GET(), function1_lastExecute, function1_timeout))
  {
    return;
  }
  function1_lastExecute = T64_TIMER_GET();

  // do some job here
  Serial.println("function_1(foreground)(every 1 sec)");
}

////////////////////////////////////////////////////////////////////////////////
//foreground function2 - executes maximum once every 100 millis
int32_t function2_lastExecute;
// 100 millis  in 10's of millis
#define function2_timeout  (10L)

void function_2()
{
  if (!T64_DO_EXECUTE(T64_TIMER_GET(), function2_lastExecute, function2_timeout))
  {
    return;
  }
  function2_lastExecute = T64_TIMER_GET();

  // do some job here
  Serial.println("function_2(foreground)(every 100 millis)");
}


////////////////////////////////////////////////////////////////////////////////
// shared foreground/background variables
T64_DEF (uint64_t, data);

////////////////////////////////////////////////////////////////////////////////
//foreground function3 - executes every 10 millis and try to provide data to
//background to process if background is finished with previous data
int32_t function3_lastExecute;
// 10 millis  in 10's of millis
#define function3_timeout  (1L)

void function_3()
{

  if (!T64_DO_EXECUTE(T64_TIMER_GET(), function3_lastExecute, function3_timeout))
  {
    return;
  }
  function3_lastExecute = T64_TIMER_GET();

  // generate new data
  uint64_t tmpData = random(1000);

  // try send data to the processor if processor is ready to receive data
  if (T64_TRY_TO( tmpData, data ))
  {
    Serial.println("function_3 (data to background provided) : (tmpData)[" + String((uint32_t)tmpData) + "]");
  }
  else
  {
    // processor is not read to process data , still dealing with previous data
  }

}

////////////////////////////////////////////////////////////////////////////////
//foreground function4 - executes every 10 millis and try to consume data ready from
//background
int32_t function4_lastExecute;
// 10 millis  in 10's of millis
#define function4_timeout  (1L)

void function_4()
{

  if (!T64_DO_EXECUTE(T64_TIMER_GET(), function4_lastExecute, function4_timeout))
  {
    return;
  }

  function4_lastExecute = T64_TIMER_GET();

  // define variable to store the data from the processor
  uint64_t tmpData;
  // try recieve data from the background processor if avialble
  if (T64_TRY_FROM( tmpData, data ))
  {
    // data is received
    Serial.println("function_4 (data from background consumed) : (tmpData)[" + String((uint32_t)tmpData) + "]");
  }
  else
  {
    // no data available yet from processor
  }

}


////////////////////////////////////////////////////////////////////////////////
// background - executes every 10 millis and try processing the data if available
//
int32_t background1_lastExecute;
// 10 millis  in 10's of millis
#define background1_timeout  (1L)

void  background_1()
{

  if (!T64_DO_EXECUTE(T64_TIMER_GET(), background1_lastExecute, background1_timeout))
  {
    return;
  }
  background1_lastExecute = T64_TIMER_GET();

  // temporary storage
  uint64_t tmpData;
  // try get data for pocessing if available
  if (T64_TRY_GET( tmpData, data ))
  {
    // do some calculations/job
    for (uint8_t i = 0; i < 2; i++)
    {
      tmpData += tmpData;
    }

    if (T64_TRY_SET ( tmpData, data ))
    {
      // data has been actually processed and left for consumer
    }
    else
    {
      // data cannot be left for consuming as the consumer
      // has not consumed the previous data

      // do one more cycle
    }
  }

}

void loop()
{

  function_1();

  function_2();

  function_3(); // provider

  // background_1 is processor

  function_4(); // consumer


  delay(1);
}

