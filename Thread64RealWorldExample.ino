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
uint64_t data_to_background_to_process_sequence = 0;
uint64_t data_to_background_to_process;

uint64_t data_from_background_ready_sequence = 0;
uint64_t data_from_background_ready;

uint64_t data_from_background_consumed_sequence = 0;

////////////////////////////////////////////////////////////////////////////////
//foreground function3 - executes every 10 millis and try to provide data to
//background to process if background is finished with previous data
int32_t function3_lastExecute;
// 10 millis  in 10's of millis
#define function3_timeout  (1L)

uint64_t function3_sequence = 0;
void function_3()
{
  if (!T64_DO_EXECUTE(T64_TIMER_GET(), function3_lastExecute, function3_timeout))
  {
    return;
  }
  function3_lastExecute = T64_TIMER_GET();

  T64_LOCK;
  {
    if (data_from_background_ready_sequence == function3_sequence)
    {
      // last data has been processed
      // background is ready for new data
      // we send some data for background
      uint64_t data;
      // always keep unlocked when going out of our function as we do not know how long it will take
      T64_UNLOCK;
      {
        data = random(1000);
        ++function3_sequence;
      }
      T64_LOCK;

      data_to_background_to_process_sequence = function3_sequence;
      data_to_background_to_process = data;

      T64_UNLOCK;
      {
        Serial.println("function_3 (data to background provided) : (seq,data)[" + String((uint32_t)function3_sequence) + "," + String((uint32_t)data) + "]");
      }
      T64_LOCK;
    }
  }
  T64_UNLOCK;
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

  T64_LOCK;
  {
    if (data_from_background_consumed_sequence != data_from_background_ready_sequence)
    {
      // there is ready to be consumed data
      uint64_t data = data_from_background_ready;
      uint64_t sequence = data_from_background_ready_sequence;
      data_from_background_consumed_sequence = data_from_background_ready_sequence;

      // always keep unlocked when going out of our function as we do not know how long it will take

      T64_UNLOCK;
      {
        Serial.println("function_4 (data from background consumed) : (seq,data)[" + String((uint32_t)sequence) + "," + String((uint32_t)data) + "]");
      }
      T64_LOCK;
    }
  }
  T64_UNLOCK;
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

  T64_LOCK;
  {
    if (data_to_background_to_process_sequence != data_from_background_ready_sequence)
    {
      // there is ready to be processed data
      uint64_t data = data_to_background_to_process;
      uint64_t sequence = data_to_background_to_process_sequence;

      // always keep unlocked when going out or doing hard job

      T64_UNLOCK;
      {
        // do some calculations/job
        for (uint8_t i = 0; i < 100; i++)
        {
          data += data;
        }
      }
      T64_LOCK;

      // set the result now if the consumer is ready to consume
      if (data_from_background_consumed_sequence == data_from_background_ready_sequence)
      {
        data_from_background_ready = data;
        data_from_background_ready_sequence = sequence;
      }
      else
      {
         //we will do the calculation again and send teh result on next cycle
      }
    }
  }
  T64_UNLOCK;
}

void loop()
{
  function_1();

  function_2();

  function_3();

  function_4();


  delay(1);
}

