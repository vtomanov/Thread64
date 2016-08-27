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

// Current value of timer
inline int32_t T64_TIMER_GET();

// Max background stack size hit so far
inline uint16_t T64_BACKGROUND_MAX_STACK_GET();

//////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS ( .cpp )

volatile int32_t T64_TIMER_millis10;  // store timer counter

volatile uint16_t T64_BK_ST_MAX_SIZE;  // calculated MAX value ever hit by the background stack size

volatile bool T64_IS_BK_INITIALIZED; // if true we have background process of false only timer will be used

volatile uint16_t T64_BK_SP_S_START;  // start of background stack memory
volatile uint16_t T64_BK_SP;          // current background stack pointer last know value
volatile uint16_t T64_FG_SP;          // current foreground stack pointer last know value

volatile void * T64_ONE_PTR; // used during init/start thread process to store pointer for direct jump
volatile uint16_t T64_ONE_ST;// used during init/start thread process to store stack pointer
volatile int8_t T64_ONE_START; // flag for just start of background to indicate need of direct jump

uint16_t T64_tmpSP;     // temporary storage for stack pointer during calculations

inline void T64_INIT(void (*background)(), uint16_t background_stack_size)
{
  T64_BK_ST_MAX_SIZE = 0L;
  T64_TIMER_millis10 = 0L;
  T64_ONE_START = 0;

  if (background != NULL)
  {
    T64_ONE_START = -1;
    T64_IS_BK_INITIALIZED = true;
    T64_BK_SP_S_START = (uint16_t)malloc(background_stack_size) + background_stack_size;
  }
  else
  {
    T64_IS_BK_INITIALIZED = false;
  }

  Timer1.initialize(1000000);//10000);                      // initialise timer1, and set a 10 millis period
  Timer1.attachInterrupt(T64_CALLBACK_10MSEC);  // attaches callback() as a timer overflow interrupt

  if (T64_IS_BK_INITIALIZED)
  {
    // we have background thread
    // store local stack pointer
    T64_ONE_ST = (SP);
    // start background
    T64_ONE_PTR = && ONE_LABEL;
    T64_ONE_START = 1;
    SP = T64_BK_SP_S_START;
    T64_BK_LOOP(background);

ONE_LABEL:;
    SP = T64_ONE_ST;
    interrupts();
    Serial.println("Z");
  };

}

uint32_t cnt;
 
// call the background function in a endless loop
void T64_BK_LOOP(void (*backgrund)())
{
  for (; T64_ONE_START == 1;) {};
  for (;;)
  {
    for (cnt = 0;cnt < 1000;) {}; 
    backgrund();
  };
}

void T64_CALLBACK_10MSEC()
{
  if (T64_ONE_START < 0)
  {
    return;
  }
  
  if (T64_IS_BK_INITIALIZED)
  {
    // store all registers
    // we push all registers as there is no way to predict which registers the
    // compiler interrupt handler generator will store and which not
    asm volatile( " push  r0  ");
    asm volatile( " push  r1  ");
    asm volatile( " push  r2  ");
    asm volatile( " push  r3  ");
    asm volatile( " push  r4  ");
    asm volatile( " push  r5  ");
    asm volatile( " push  r6  ");
    asm volatile( " push  r7  ");
    asm volatile( " push  r8  ");
    asm volatile( " push  r9  ");
    asm volatile( " push  r10 ");
    asm volatile( " push  r11 ");
    asm volatile( " push  r12 ");
    asm volatile( " push  r13 ");
    asm volatile( " push  r14 ");
    asm volatile( " push  r15 ");
    asm volatile( " push  r16 ");
    asm volatile( " push  r17 ");
    asm volatile( " push  r18 ");
    asm volatile( " push  r19 ");
    asm volatile( " push  r20 ");
    asm volatile( " push  r21 ");
    asm volatile( " push  r22 ");
    asm volatile( " push  r23 ");
    asm volatile( " push  r24 ");
    asm volatile( " push  r25 ");
    asm volatile( " push  r26 ");
    asm volatile( " push  r27 ");
    asm volatile( " push  r28 ");
    asm volatile( " push  r29 ");
    asm volatile( " push  r30 ");
    asm volatile( " push  r31 ");
    // status is already stored from the compiler interrupt handler generator
  }

  if (T64_IS_BK_INITIALIZED)
  {
    // get the current value of stack pointer
    T64_tmpSP = (SP);
    if (T64_ONE_START == 1)
    {
      T64_ONE_START = 0;
      T64_BK_SP = T64_tmpSP;
      goto * T64_ONE_PTR;
    }
  }
  
  // timer calculations
  T64_TIMER_millis10++;
  if (T64_TIMER_millis10 < 0)
  {
    T64_TIMER_millis10 = 0;
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
    asm volatile( " pop r31 ");
    asm volatile( " pop r30 ");
    asm volatile( " pop r29 ");
    asm volatile( " pop r28 ");
    asm volatile( " pop r27 ");
    asm volatile( " pop r26 ");
    asm volatile( " pop r25 ");
    asm volatile( " pop r24 ");
    asm volatile( " pop r23 ");
    asm volatile( " pop r22 ");
    asm volatile( " pop r21 ");
    asm volatile( " pop r20 ");
    asm volatile( " pop r19 ");
    asm volatile( " pop r18 ");
    asm volatile( " pop r17 ");
    asm volatile( " pop r16 ");
    asm volatile( " pop r15 ");
    asm volatile( " pop r14 ");
    asm volatile( " pop r13 ");
    asm volatile( " pop r12 ");
    asm volatile( " pop r11 ");
    asm volatile( " pop r10 ");
    asm volatile( " pop r9  ");
    asm volatile( " pop r8  ");
    asm volatile( " pop r7  ");
    asm volatile( " pop r6  ");
    asm volatile( " pop r5  ");
    asm volatile( " pop r4  ");
    asm volatile( " pop r3  ");
    asm volatile( " pop r2  ");
    asm volatile( " pop r1  ");
    asm volatile( " pop r0  ");
    // status will be restored in the compiler interrupt handler generator
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

//////////////////////////////////////////////////////////////////////////////////////
// MAIN


// this variable will be set by the background and will be read from the foreground
int32_t shared_background_count = 0;

void set_shared(uint32_t shared)
{
  noInterrupts();
  shared_background_count = shared;
  interrupts();
}

void  backgroud()
{
  // calculate the value of shared_backgroud_count
  // do some work here
  for (uint32_t i ; i < 100000; i++) {};

  // accessing shared variable
  set_shared(random(8606));
}


uint32_t get_shared()
{
  uint32_t ret;
  noInterrupts();
  ret = shared_background_count;
  interrupts();
  return ret;
}

void setup()
{
  // Init Serial
  Serial.begin(115200);
  for (; !Serial;) {};

  Serial.println("Thread64 TEST");

  // Thread64
  T64_INIT(backgroud, 1024);

}

void loop()
{
  Serial.println(String(get_shared()));
  // ensure a loop takes at least 1msec
  delay(1000);
}
