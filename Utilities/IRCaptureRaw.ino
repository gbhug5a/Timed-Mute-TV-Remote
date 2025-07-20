// IRCaptureRaw.ino   Rev 3
// for ATmega328P processors (Uno, Nano, ProMini - 8 or 16MHz)
// This captures incoming IR from a remote control as raw data.
// An IR receiver module should be connected to D8.
// The Adjust variable adjusts time between On and Off periods since
//   receivers may turn on faster than they turn off, or vice versa.
// Results will include any repeats.
// The displayed numbers alternate between transmitting and idle states.

#ifndef __AVR_ATmega328P__
#error - Board must use ATmega328P microcontroller
#endif

const int maxEntries = 600;                       // total number of entries
const int maxExtras = 32;                         // number of long entries
unsigned int bitTime[maxEntries];                 // store uSeconds in each state - integers
unsigned long Extra[maxExtras];                   // long Extra entries
int Adjust = 20, newAdjust;                       // receiver turn on, turn off times not the same
const byte recvPin = 8;                           // Timer1 output - do not change
volatile unsigned int timerMSB, capMSB;           // third byte of 24-bit timer (ICR1 = 1st two)
volatile bool Capture;                            // a state change occurred, and time was captured
unsigned long timerLO, timerHI, newTime, prevTime, longTime, outVal;
int i, j, k, n, maxMSB;
bool activeLO;                                    // receiver output is low when receiving carrier
bool Waiting, Start, nothing;

void setup() {

  delay(1000);
  Serial.begin (57600);
  Serial.println();
  pinMode(recvPin, INPUT_PULLUP);
  activeLO = true;                                // assume no transmission happening now
  if (digitalRead(recvPin) == LOW) activeLO = false;
  maxMSB = 60 >> (F_CPU == 8000000);              // capture time = ~2 sec

  cli();
  TCCR1A = 0;                                     // set up Timer1
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT1  = 0;                                     // clear Timer1 count
  sei();
  Start = true;
}

void loop() {

  if (Start) {                                    // initialize everything on start or restart
    Start = false;
    i = 0;
    n = 0;
    j = 1;
    Waiting = true;
    Capture = false;
    nothing = false;

    Serial.print(F("Adjust = ")); Serial.println(Adjust);
    Serial.println(F("Enter new Adjust value, or [Enter]"));
    while (!Serial.available());

    if ((Serial.peek() == 13) || (Serial.peek() == 10)) nothing = true;
    newAdjust = Serial.parseInt(SKIP_NONE);       // Adjust can be negative
    if ((newAdjust) || !nothing) Adjust = newAdjust;
    Serial.print(F("Adjust = ")); Serial.println(Adjust);
    while (Serial.available()) Serial.read();
    Serial.println(F("Waiting for IR transmission..."));
    timerMSB = 0;

    cli();
    TIMSK0 = 0;                             // Disable Timer0 interrupts (millis)
    TIFR1  = 0xFF;                          // clear flags register
    if (activeLO) TCCR1B = 0b00000010;      // falling edge capture, timer1 on, prescale /8
    else TCCR1B = 0b01000010;               // rising edge capture, timer1 on, prescale /8
    TIFR1  = 0xFF;                          // clear flags
    TIMSK1 = 0b00100001;                    // enable capture and overflow interrupts
    TIFR1  = 0xFF;                          // clear flags
    TCCR1A = 0b00000000;                    // Normal mode, no output, WGM #0
    sei();
  }

  if (Waiting) {                                  // keep MSB cleared
    if (timerMSB) timerMSB = 0;
  }

  if (Capture) {                                  // input has changed state
    Capture = false;
    Waiting = false;
    timerLO = ICR1;                               // read timer values
    timerHI = capMSB;
    newTime = (timerHI << 16) + timerLO;          // combine timer counts to one long value
    if (maxMSB == 60) newTime = (newTime + 1) >> 1;
    if (i) {                                      // skip i=0
      longTime = newTime - prevTime;              // collect duration and activity state
      if (longTime > 0xFFFF) {
        bitTime[i] = n;
        Extra[n] = longTime;
        n++;
      }
      else bitTime[i] = longTime;
    }
    prevTime = newTime;                           // prepare for next state change
    TCCR1B ^= 0b01000000;                         // switch to capture on opposite edge
    i++;
  }

  if ((timerMSB > maxMSB) || (i > maxEntries)) {  // collect data for two seconds after first
    TCCR1B &= 0xFE;                               // stop Timer1 clock
    TIMSK1 = 0b000000000;                         // disable capture and overflow interrupt
    TIMSK0 = 1;                                   // resume millis interrupt
    for (k = 1; k < i; k++) {                     // adjust on (Mark) and off (Space) times
      if (bitTime[k] >= maxExtras) {              // not an Extra
        if (k & 1) bitTime[k] -= Adjust;          // shift Adjust from On to Off
        else bitTime[k] += Adjust;
      }
      else {
        if (k & 1) Extra[bitTime[k]] -= Adjust;
        else Extra[bitTime[k]] += Adjust;
      }
    }
    Serial.println();                             // display output
    Serial.println(F("     On     Off      On     Off      On     Off      On     Off"));
    Serial.println();
    while (j < i) {
      for (k = 0; k < 8; k++) {
        outVal = bitTime[j];
        if (outVal < maxExtras) outVal = Extra[bitTime[j]];
        Serial.print(" ");
        if (outVal < 100000) Serial.print(" ");
        if (outVal < 10000) Serial.print(" ");
        if (outVal < 1000) Serial.print(" ");
        if (outVal < 100) Serial.print(" ");
        if (outVal < 10) Serial.print(" ");
        Serial.print (outVal);
        j++;
        if (j == i) break;
        else Serial.print (",");
      }
      Serial.println();
    }
    Serial.println();
    Start = true;
  }
}

ISR(TIMER1_CAPT_vect) {
  capMSB = timerMSB;                              // also capture MSB at same instant
  Capture = true;
}

ISR(TIMER1_OVF_vect) {
  timerMSB++;                                     // increment MSB on overflow
}
