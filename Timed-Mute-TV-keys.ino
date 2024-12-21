/*

Timed-Mute-TV-keys.ino

This code operates a special purpose IR remote control for a TV which
mutes the TV's audio for a specific time in response to entering the
number of minutes on a keypad.  Volume changes and channel selection
are also handled.

This sketch does not use the Keypad.h library, but instead directly reads
the keypad lines.  It uses the IRremote.hpp library v4.4.1 to send IR codes.
The file PinDefinitionsAndMore.h should be in the same folder with this sketch.

Here is the menu for the device:


   {N}        - Mute TV for {N} minutes
   {N}*       - Mute TV for {N + 0.5} minutes (0* = 30 seconds)
              - Hit any key to unMute early
    * long    - Toggle TV Mute
    # long    - Toggle TV Power
    *         - Soundbar audio: minus one tick
    #         - Soundbar audio: plus one tick
   {N}#       - Cable box channel number entry


Arduino Pro Mini - 3.3V 8MHz, powered directly from two AA alkaline cells.
Remove power indicator LED and voltage regulator from Pro Mini.
IRLED drive on D3.

*/
#define DISABLE_CODE_FOR_RECEIVER               // setup for IRremote
#define NO_LED_FEEDBACK_CODE
#define SEND_PWM_BY_TIMER
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>                         // v4.4.1 or later

#include <avr/sleep.h>
#include <avr/wdt.h>

const int Interval = 500;                       // interval between channel number digits

const uint8_t TVaddr = 0x1;                     // Sharp TV IR codes - address
const uint8_t TVmute = 0x17;                    // commands
const uint8_t TVpwr  = 0x16;

const unsigned long sBarPlus  = 0x00FF827D;     // Vizio soundbar audio level keys, MSB
const unsigned long sBarMinus = 0x00FFA25D;

unsigned long Cable[] = {                       // Cox Minibox channel number keys 0-9, MSB
  0xF50608F7,
  0xF5068877,
  0xF50648B7,
  0xF506C837,
  0xF50628D7,
  0xF506A857,
  0xF5066897,
  0xF506E817,
  0xF50618E7,
  0xF5069867};

const byte ROWS = 4;                  // four rows
const byte COLS = 3;                  // three columns
                                      // D3 reserved for IRLED output drive
byte rowPins[ROWS] = {4, 5, 6, 7};    // row pins of the keypad - all must be in Port D
byte colPins[COLS] = {A0, A1, A2};    // column pins of the keypad
const byte PCIenable = 4;             // pin change interrupt for Port D - for PCICR register
byte PCImask = 0;                     // match row pins in Port D - for PCMSK2 register
byte eachKey [5];                     // individual channel number digits
byte numPress;                        // number of channel number digits

const char keys[ROWS][COLS] = {       // keypad layout
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};


int Minutes;                          // the number keyed in to the keypad
unsigned long keyTime;                // millis value when last key pressed
unsigned long deb;                    // current debounce count
unsigned long endTime;                // time wt4release() begins
const unsigned long debTime = 5;      // debounce time
const unsigned long timeLimit = 1500; // assume no more keys after this
const unsigned long shortLimit = 500; // long press after this
unsigned long nowTime;                // beginning of mute period
unsigned long muteTime;               // total mute period (ms)
unsigned long blinkTime;              // blink period during mute
int i, c, r,ikey;
char key, keyCopy;
bool longPress;                       // flag for short or long press

char getkey() {                       // function to get current key pressed if any
  key = 0;
  for (c = 0; c < COLS; c++) {
    pinMode (colPins[c],OUTPUT);      // one column at a time - output low
    for (r = 0; r < ROWS; r++) {
      if (digitalRead(rowPins[r]) == LOW) {  // low if pressed
        key = keys[r][c];
        break;                        // first pressed key - no multiple keys
      }
    }
    pinMode (colPins[c],INPUT);       // restore column to input mode
    if (key) break;                   // abort if a key has been found
  }
  return key;
}

bool wt4release() {                   // wait for debounced release of pressed key
  endTime = millis();                 //    or for shortLimit to be exceeded
  longPress = false;
  pinMode (colPins[c],OUTPUT);
  deb = millis();                     // must be continuously released for debTime ms
  while(1) {
    if (digitalRead(rowPins[r]) == LOW) deb = millis();
    else if ((millis() - deb) > debTime) break;
    if ((millis() - endTime) > shortLimit) {
      longPress = true;
      break;
    }
  }
  pinMode (colPins[c],INPUT);
  return longPress;
}

void setup() {

  pinMode (IR_SEND_PIN, OUTPUT);      // no float on powerup

  for (r = 0; r < ROWS; r++) {        // the ROW pins will generate pin change interrupts
    pinMode(rowPins[r],INPUT_PULLUP); //   to wake the MCU, then sense key press in getkey()
  }
  pinMode(13, OUTPUT);

  ADCSRA = 0;                         // disable ADC for power saving
  wdt_disable();                      // disable WDT for power saving

  for (r = 0; r < ROWS; r++) {        // generate mask for row pins
    PCImask |= 1 << rowPins[r];
  }
  PCICR = PCICR | PCIenable;          // enable pin change interrupts, but PCMSK2 still empty
}

void loop() {

  for (c = 0; c < COLS; c++) {        // column pins all low for pin change interrupt
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c],LOW);
  }

  while ((PIND & PCImask) != PCImask);  // make sure no keys pressed
  delay (debTime << 1);
  while ((PIND & PCImask) != PCImask);

  digitalWrite(13,LOW);               // turn off LED during sleep time
  PCIFR = PCIFR | PCIenable;          // clear any pending interrupt flag bit
  PCMSK2 = PCMSK2 | PCImask;          // enable interrupt mask bits for the ROW pins

  set_sleep_mode (SLEEP_MODE_PWR_DOWN); // Deep sleep
  sleep_enable();
  sleep_bod_disable();                // disable brownout detector during sleep
  sleep_cpu();                        // now go to sleep

  //  *WAKE UP* from pressing any key

  digitalWrite(13,HIGH);              // LED on or blinking when awake

  for (c = 0; c < COLS; c++) {        // all columns back to INPUT - for getkey function
    pinMode(colPins[c], INPUT);       //   getkey takes one at a time OUTPUT LOW
  }

  key = 0;

  for (i = 0; i < 100; i++) {         // get key currently pressed
    key = getkey();
    if (key) break;
  }
  if (!key) return;
  
  Minutes = 0;
  numPress = 0;
  keyTime = millis();                 // mark time when key pressed
  wt4release();                       // detect debounced release or long press
  keyCopy = key;
  if (key < '0') {                    // the character zero - first key is "*" or "#"

    if (keyCopy == '*') {
      if (longPress) IrSender.sendSharp(TVaddr,TVmute,0);
      else IrSender.sendNECMSB(sBarMinus,32,0);
    }
    else if (keyCopy == '#'){
      if (longPress) IrSender.sendSharp(TVaddr,TVpwr,0);
      else IrSender.sendNECMSB(sBarPlus,32,0);
    }
    while (wt4release());             // make sure key released
  }
  else {                                     // first key is a number
    while (1) {                              // look until timeout, then process, then sleep
      if (key) {
        keyTime = millis();                  // mark time when key pressed
        while (wt4release());                // wait for final release of key
        keyCopy = key;
        if ((key >= '0') && (key <= '9')) {
          ikey = key - 48;                   // convert character to integer value
          Minutes = (Minutes * 10) + ikey;   // if another key, shift left & add
          eachKey[numPress] = ikey;          // save each key value for channel number
          numPress++;
        }
      }
      else {                                           // if no key, check time
        if ((millis() - keyTime) > timeLimit) {        // if time has expired
          if (keyCopy == '#') {                        // this was channel number
            for (i=0;i<numPress;i++) {
              IrSender.sendNECMSB(Cable[eachKey[i]],32,0);
              delay(Interval);
            }                                          // back to loop()
            break;
          }
          nowTime = millis();                          // this was mute time
          muteTime = ((60000 * Minutes));              // do MUTE, delay, MUTE
          if (keyCopy == '*') muteTime += 30000;       // extra half minute
          if (muteTime > 2000) {
            IrSender.sendSharp(TVaddr,TVmute,0);
            muteTime -= 2000;
            blinkTime = millis();                      // blink during mute period
            digitalWrite(13,LOW);
            while((millis() - nowTime) < muteTime) {   // any key cancels MUTE early
              if ((millis() - blinkTime) >= 990) {     // blink about once per second
                digitalWrite(13,HIGH);
                delay(5);                              // blink for 3ms
                digitalWrite(13,LOW);
                blinkTime = millis();
              }
              if(getkey()) {                           // any keypress unmutes audio
                while (wt4release());
                break;
              }
            }
            digitalWrite(13,HIGH);
            IrSender.sendSharp(TVaddr,TVmute,0); // unmute at end of delay
          }
          break;
        }                             // if time has expired
      }                               // if no key
      key = getkey();                 // get key currently pressed, if any
    }                                 // While
  }                                   // else first key is number
}                                     // Loop

ISR (PCINT2_vect) {                   // pin change interrupt service routine
  PCMSK2 = PCMSK2 & (~PCImask);       // clear mask bits = disable further interrupts
  PCIFR = PCIFR | PCIenable;          // clear any pending interrupt flag bit
}
