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
// Background thread and timer implementation
//
// This library is designed to be used in scenarios whee one foreground and one background thread is required
// e.g. when part of the calculations are heavy and need to be done in background based on asynchronous fashion without
// affecting the foreground processing working with gauges and communication

#include "TimerOne.h"

//////////////////////////////////////////////////////////////////////////////////////
// ALL AVAILABLE FUNCTIONS ( .h )

// Please call in setup() method
// In case only timer is required pass NULL as a parameter
inline void T64_INIT(void (*background)(), uint16_t background_stack_size);

// In case you need only a timer - use this one ( same as NULL, 0)
inline void T64_INIT();

// Notify the system that the current thread is willing to "give up the CPU" for a while
inline void T64_YIELD();

// Max background stack size hit so far - use this to determining while
// testing what is the stack size that you need to give to the background thread
inline uint16_t T64_BACKGROUND_MAX_STACK_GET();

//////////////////////////////////////////////////////////////////////////////////////
// Timer related

// Current value of timer
inline int32_t T64_TIMER_GET();

// Will return true only if the function is ready for execute
inline bool T64_DO_EXECUTE(int32_t loopCounter, int32_t lastExecute, int32_t timeout);

//////////////////////////////////////////////////////////////////////////////////////
// Variable access ( only for ordinal )

// return value, pass variable
template <typename T>
inline  T const  T64_GET (T const& v)
{
  T ret;
  noInterrupts();
  ret = v;
  interrupts();
  return ret;
};

// return old value, pass new value, variable
template <typename T>
inline T const T64_SET (T const n, T & v)
{
  T ret;
  noInterrupts();
  ret = v;
  v = n;
  interrupts();
  return ret;
};

// basic lock
#define T64_LOCK noInterrupts();

// basic unlock
#define T64_UNLOCK interrupts();

//////////////////////////////////////////////////////////////////////////////////////
// PROVIDER -> PROCESSOR -> CONSUMER pattern related to Gps64 usage

// declare shared coordinate variables for P->P->C pattern
// with start point (GPS_DATA) with default values, current point (GPS_DATA ), return distance(double) and bearing(double)
#define T64_DEF_G64( a, start_lat_default_value, start_lng_default_value) \
  GPS_DATA T64_##a##_lat_start = start_lat_default_value; \
  GPS_DATA T64_##a##_lng_start = start_lng_default_value; \
  \
  GPS_DATA T64_##a##_lat_to = start_lat_default_value; \
  GPS_DATA T64_##a##_lng_to = start_lng_default_value; \
  GPS_DATA T64_##a##_lat_start_to; \
  GPS_DATA T64_##a##_lng_start_to; \
  GPS_DATA T64_##a##_lat_last_to; \
  GPS_DATA T64_##a##_lng_last_to; \
  \
  double T64_##a##_bearing_start_from; \
  double T64_##a##_bearing_current_from; \
  double T64_##a##_distance_start_from; \
  double T64_##a##_distance_current_from; \
  \
  uint64_t T64_##a##_to_sequence = 0L; \
  uint64_t T64_##a##_from_sequence = 0L; \
  uint64_t T64_##a##_consumed_sequence = 0L;


// get the value of the last point into passed variables
#define T64_GET_LAST_G64(o_lat,o_lng, a) \
  { \
    T64_noInterrupts(); \
    o_lat = T64_##a##_lat_to; \
    o_lng = T64_##a##_lng_to; \
    T64_interrupts(); \
  };

#define T64_SET_START_G64(v_lat,v_lng, a) \
  { \
    T64_noInterrupts(); \
    T64_##a##_lat_start = v_lat; \
    T64_##a##_lng_start = v_lng; \
    T64_interrupts(); \
  };

// provider try to supply data to processor if processor is ready for P->P->C pattern
#define T64_TRY_TO_G64( v_lat,v_lng, a) \
  ( \
    (T64_noInterrupts(), T64_##a##_to_sequence == T64_##a##_from_sequence) ? \
    (T64_##a##_lat_start_to = T64_##a##_lat_start, T64_##a##_lng_start_to = T64_##a##_lng_start ,\
     T64_##a##_lat_last_to = T64_##a##_lat_to, T64_##a##_lng_last_to = T64_##a##_lng_to, \
     T64_##a##_lat_to = v_lat, T64_##a##_lng_to = v_lng, ++T64_##a##_to_sequence , \
     T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )

// processor try to get data from provider if the provider has supplied new data
#define T64_TRY_GET_G64( o_lat,o_lng, start_lat,start_lng, last_lat,last_lng, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_to_sequence != T64_##a##_from_sequence) ? \
    (o_lat = T64_##a##_lat_to, o_lng = T64_##a##_lng_to, \
     start_lat = T64_##a##_lat_start_to, start_lng = T64_##a##_lng_start_to, \
     last_lat = T64_##a##_lat_last_to, last_lng = T64_##a##_lng_last_to, \
     T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )

// processor try to send data to consumer if the consumer has finished with previous data
#define T64_TRY_SET_G64( bearing_start,bearing_current, distance_start,distance_current, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_from_sequence == T64_##a##_consumed_sequence) ? \
    (T64_##a##_bearing_start_from = bearing_start, T64_##a##_bearing_current_from = bearing_current, \
     T64_##a##_distance_start_from = distance_start, T64_##a##_distance_current_from = distance_current, \
     T64_##a##_from_sequence = T64_##a##_to_sequence, T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )

// consumer try to get data from processor if the processor has finished with processor has finished with processing
#define T64_TRY_FROM_G64( bearing_start,bearing_current, distance_start,distance_current, a )  \
  ( \
    (T64_noInterrupts(), T64_##a##_from_sequence != T64_##a##_consumed_sequence) ? \
    (bearing_start = T64_##a##_bearing_start_from, bearing_current = T64_##a##_bearing_current_from , \
     distance_start = T64_##a##_distance_start_from, distance_current = T64_##a##_distance_current_from, \
     T64_##a##_consumed_sequence = T64_##a##_from_sequence, T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )


//////////////////////////////////////////////////////////////////////////////////////
// PROVIDER -> PROCESSOR -> CONSUMER pattern helpers

// declare shared variable for P->P->C pattern
#define T64_DEF( T , a ) \
  T T64_##a##_to; \
  T T64_##a##_from;  \
  uint64_t T64_##a##_to_sequence = 0L; \
  uint64_t T64_##a##_from_sequence = 0L; \
  uint64_t T64_##a##_consumed_sequence = 0L;

// provider try to supply data to processor if processor is ready for P->P->C pattern
#define T64_TRY_TO( v, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_to_sequence == T64_##a##_from_sequence) ? \
    (T64_##a##_to = v, ++T64_##a##_to_sequence , T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )

// processor try to get data from provider if the provider has supplied new data
#define T64_TRY_GET( o, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_to_sequence != T64_##a##_from_sequence) ? \
    (o = T64_##a##_to, T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )

// processor try to send data to consumer if the consumer has finished with previous data
#define T64_TRY_SET( v, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_from_sequence == T64_##a##_consumed_sequence) ? \
    (T64_##a##_from = v, T64_##a##_from_sequence = T64_##a##_to_sequence, T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )


// consumer try to get data from processor if the processor has finished with processor has finished with processing
#define T64_TRY_FROM( o, a ) \
  ( \
    (T64_noInterrupts(), T64_##a##_from_sequence != T64_##a##_consumed_sequence) ? \
    (o = T64_##a##_from, T64_##a##_consumed_sequence = T64_##a##_from_sequence, T64_interrupts(), true) : \
    (T64_interrupts(), false) \
  )


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS


//////////////////////////////////////////////////////////////////////////////////////
// Prevent the compiler from  optimising the code
#pragma GCC push_options
#pragma GCC optimize ("O0")

//////////////////////////////////////////////////////////////////////////////////////
// Wrap interrupts and noInterrupts in a function style to be able to use it in macro
inline bool T64_interrupts()
{
  interrupts();
  return true;
}

inline bool T64_noInterrupts()
{
  noInterrupts();
  return true;
}
//////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS (MARCO)

#define T64_PUSH  \
  asm volatile( " push  r0  "); \
  asm volatile( " push  r1  ");  \
  asm volatile( " in r0,0x03f"); \
  asm volatile( " push r0");     \
  asm volatile( " eor r1,r1");   \
  asm volatile( " in r0,0x3b");  \
  asm volatile( " push  r0  ");  \
  asm volatile( " push  r2  ");  \
  asm volatile( " push  r3  ");  \
  asm volatile( " push  r4  ");  \
  asm volatile( " push  r5  ");  \
  asm volatile( " push  r6  ");  \
  asm volatile( " push  r7  ");  \
  asm volatile( " push  r8  ");  \
  asm volatile( " push  r9  ");  \
  asm volatile( " push  r10 ");  \
  asm volatile( " push  r11 ");  \
  asm volatile( " push  r12 ");  \
  asm volatile( " push  r13 ");  \
  asm volatile( " push  r14 ");  \
  asm volatile( " push  r15 ");  \
  asm volatile( " push  r16 ");  \
  asm volatile( " push  r17 ");  \
  asm volatile( " push  r18 ");  \
  asm volatile( " push  r19 ");  \
  asm volatile( " push  r20 ");  \
  asm volatile( " push  r21 ");  \
  asm volatile( " push  r22 ");  \
  asm volatile( " push  r23 ");  \
  asm volatile( " push  r24 ");  \
  asm volatile( " push  r25 ");  \
  asm volatile( " push  r26 ");  \
  asm volatile( " push  r27 ");  \
  asm volatile( " push  r28 ");  \
  asm volatile( " push  r29 ");  \
  asm volatile( " push  r30 ");  \
  asm volatile( " push  r31 ");

#define T64_POP  \
  asm volatile( " pop r31 "); \
  asm volatile( " pop r30 ");  \
  asm volatile( " pop r29 ");  \
  asm volatile( " pop r28 ");  \
  asm volatile( " pop r27 ");  \
  asm volatile( " pop r26 ");  \
  asm volatile( " pop r25 ");  \
  asm volatile( " pop r24 ");  \
  asm volatile( " pop r23 ");  \
  asm volatile( " pop r22 ");  \
  asm volatile( " pop r21 ");  \
  asm volatile( " pop r20 ");  \
  asm volatile( " pop r19 ");  \
  asm volatile( " pop r18 ");  \
  asm volatile( " pop r17 ");  \
  asm volatile( " pop r16 ");  \
  asm volatile( " pop r15 ");  \
  asm volatile( " pop r14 ");  \
  asm volatile( " pop r13 ");  \
  asm volatile( " pop r12 ");  \
  asm volatile( " pop r11 ");  \
  asm volatile( " pop r10 ");  \
  asm volatile( " pop r9  ");  \
  asm volatile( " pop r8  ");  \
  asm volatile( " pop r7  ");  \
  asm volatile( " pop r6  ");  \
  asm volatile( " pop r5  ");  \
  asm volatile( " pop r4  ");  \
  asm volatile( " pop r3  ");  \
  asm volatile( " pop r2  ");  \
  asm volatile( " pop r0");    \
  asm volatile( " out 0x3b,r0");\
  asm volatile( " pop r0");     \
  asm volatile( " out 0x3f,r0");\
  asm volatile( " pop r1  ");  \
  asm volatile( " pop r0  ");

//////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS ( .cpp )


volatile int32_t T64_TIMER_millis10;  // store timer counter

volatile uint16_t T64_BK_ST_MAX_SIZE;  // calculated MAX value ever hit by the background stack size

volatile bool T64_IS_BK_INITIALIZED; // if true we have background process of false only timer will be used

volatile uint16_t T64_BK_SP_S_START;  // start of background stack memory
volatile uint16_t T64_BK_SP;          // current background stack pointer last know value
volatile uint16_t T64_FG_SP;          // current foreground stack pointer last know value

volatile uint16_t T64_ONE_ST;// used during init/start thread process to store stack pointer
volatile int8_t T64_ONE_START; // flag for just start of background to indicate need of direct jump
volatile void * T64_ONE_START_PTR; // ptr for just start of background and return with direct jump

uint16_t T64_tmpSP;     // temporary storage for stack pointer during calculations

void T64_CALLBACK_10MSEC(); // the timer callback
void T64_BK_LOOP(void (*background)()); // indefinite background loop

bool T64_flag_call_from_yield;

inline void T64_YIELD()
{
  noInterrupts();
  T64_flag_call_from_yield = true;
  Timer1.isrCallback();
  interrupts();
}

inline void T64_INIT(void (*background)(), uint16_t background_stack_size)
{
  T64_BK_ST_MAX_SIZE = 0L;
  T64_TIMER_millis10 = 0L;
  T64_ONE_START = 0;
  T64_flag_call_from_yield = false;

  if (background != NULL)
  {
    T64_ONE_START = -1;
    T64_ONE_START_PTR = && T64_ONE_START_LABEL;
    T64_IS_BK_INITIALIZED = true;
    T64_BK_SP_S_START = (uint16_t)malloc(background_stack_size) + background_stack_size;
  }
  else
  {
    T64_IS_BK_INITIALIZED = false;
  }

  Timer1.initialize(10000);                      // initialise timer1, and set a 10 millis period
  Timer1.attachInterrupt(T64_CALLBACK_10MSEC);  // attaches callback() as a timer overflow interrupt

  if (T64_IS_BK_INITIALIZED)
  {
    // we have background thread

    // store all registers
    // (we can push only the status but it doesn't matter as this is done only once during init and next all is pop)
    T64_PUSH;

    // save forground stack pointer
    T64_ONE_ST = (SP);

    // set stack pointer to the background stack
    (SP) = T64_BK_SP_S_START;

    // ready for first interrupt
    T64_ONE_START = 1;

    // start background
    T64_BK_LOOP(background);

T64_ONE_START_LABEL:

    // we return here from first timer interrupt
    // restore stack pointer to foreground stack
    SP = T64_ONE_ST;

    //restore all registers
    T64_POP;

  };

}

inline void T64_INIT()
{
  T64_INIT(NULL, 0);
}

// call the background function in a endless loop
void T64_BK_LOOP(void (*background)())
{
  for (; T64_ONE_START == 1;) {
    /* wait for first timer interrupt to happened */
  };
  for (;;)
  {
    background();
  };
}

void T64_CALLBACK_10MSEC()
{

  // make sure if preempted before ready for first interrupt we go out
  if (T64_ONE_START < 0)
  {
    return;
  }


  if (T64_IS_BK_INITIALIZED)
  {

    // store all registers
    // we push all registers as there is no way to predict which registers the
    // compiler interrupt handler generator will store and which not

    T64_PUSH;

  }

  if (T64_IS_BK_INITIALIZED)
  {

    // get the current value of stack pointer
    T64_tmpSP = (SP);

    if (T64_ONE_START == 1)
    {
      // this is first ever interrupt and we are in background

      // make sure we never enetr here again
      T64_ONE_START = 0;

      // store background stack pointer - all registers already stored in the background stack
      T64_BK_SP = T64_tmpSP;

      // jump and continue inicialization process
      goto * T64_ONE_START_PTR;
    }

  }

  if (T64_flag_call_from_yield)
  {
    // we do not increase the timer if teh interrupt is simulated from yeld;
    T64_flag_call_from_yield = false;
  }
  else
  {
    // timer calculations
    T64_TIMER_millis10++;
    if (T64_TIMER_millis10 < 0)
    {
      T64_TIMER_millis10 = 0;
    }
  }
  if (T64_IS_BK_INITIALIZED)
  {
    // stack calculations
    if (T64_tmpSP <= T64_BK_SP_S_START)
    {
      // we are in background thread

      // store the background SP
      T64_BK_SP = T64_tmpSP;

      // calc max stack usage so far
      T64_tmpSP = T64_BK_SP_S_START - T64_BK_SP;
      if (T64_tmpSP > T64_BK_ST_MAX_SIZE)
      {
        T64_BK_ST_MAX_SIZE = T64_tmpSP;
      }

      // switch to foreground
      SP = T64_FG_SP;

    }
    else
    {
      // we are in foreground thread

      // store foreground SP
      T64_FG_SP = T64_tmpSP;

      // switch to background
      SP = T64_BK_SP;
    }
  }

  if (T64_IS_BK_INITIALIZED)
  {
    // restore all registers

    T64_POP;

  }
}

inline int32_t T64_TIMER_GET()
{
  return T64_TIMER_millis10;
}

inline uint16_t T64_BACKGROUND_MAX_STACK_GET()
{
  return T64_BK_ST_MAX_SIZE;
}

// this will return true only if the fuction is ready for execute
inline bool T64_DO_EXECUTE(int32_t loopCounter, int32_t lastExecute, int32_t timeout)
{
  //handle the case when ++long become 0
  if (loopCounter < lastExecute)
  {
    return true;
  }

  if ((lastExecute + timeout) < loopCounter)
  {
    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////////////////
//Restore the prevention of the compiler from  optimising the code
#pragma GCC pop_options

