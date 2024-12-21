// This puts an ATmega328P to sleep so you can measure current draw.
// Sleep current should be 1uA or less.
// Some counterfeit chips have sleep current over 100uA.
// This is for a Pro Mini with power LED and voltage regulator removed,
//    so the processor is the only thing drawing current.     

#include <avr/sleep.h>
#include <avr/wdt.h>

void setup(){

  ADCSRA = 0;                         // disable ADC for power saving
  wdt_disable();                      // disable WDT for power saving
  set_sleep_mode (SLEEP_MODE_PWR_DOWN); // Deep sleep
  sleep_enable();
  sleep_bod_disable();                // disable brownout detector during sleep
  sleep_cpu();                        // now go to sleep

}

void loop(){
}
