// IRCaptureRaw.ino   Rev 1
// for ATmega328P processors (Uno, Nano, ProMini - 8 or 16MHz)
// This captures incoming IR from a remote control as raw data.
// An IR receiver module, such as the TSOP38438, should be connected to D8.
// The Adjust variable adjusts time between On and Off periods since
// receivers may turn on faster than they turn off, or vice versa.
// Results will include any repeats.
// The displayed numbers alternate between transmitting and idle states.

// See https://github.com/gbhug5a/SimpleIRRaw for more info


const int maxEntries = 500;
unsigned int bitTime[maxEntries];                 // store uSeconds in each state
int Adjust = 20, newAdjust;                       // receiver turn on, turn off times not the same
const byte recvPin = 8;
volatile unsigned int timerMSB, capMSB;           // third byte of 24-bit timer (ICR1 = 1st two)
volatile bool Capture;                            // a state change occurred, and time was captured
unsigned long timerLO, timerHI, newTime, prevTime;
int i, j, k;
bool activeLO;                                    // receiver output is low when receiving carrier
bool Waiting, Start, nothing;

void setup() {

  delay(1000);
  Serial.begin (115200);
  Serial.println();
  pinMode(recvPin, INPUT_PULLUP);
  activeLO = true;                                // assume no transmission happening now
  if (!digitalRead(recvPin)) activeLO = false;

  cli();
  TIMSK0 = 0;                                     // Disable Timer0 interrupts (millis)

  TCCR1A = 0;                                     // set up Timer1
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT0  = 0;                                     // clear Timer1
  TIFR1  = 0xFF;                                  // clear flags
  TIMSK1 = 0b00100001;                            // enable capture and overflow interrupt (GPS)
  TIFR1  = 0xFF;                                  // clear flags
  TCCR1A = 0b00000000;                            // Normal mode, no output, WGM #0
  sei();
  Start = true;
}

void loop() {

  if(Start) {                                     // initialize everything on start or restart
    Start = false;
    i = 0;
    j = 1;
    Waiting = true;
    Capture = false;
    nothing = false;
    cli();
    if (activeLO) TCCR1B = 0b00000010;      // falling edge capture, timer1 on, prescale /8
    else TCCR1B = 0b01000010;               // rising edge capture, timer1 on, prescale /8
    TIFR1  = 0xFF;                          // clear flags register
    sei();

    Serial.print("Adjust = "); Serial.println(Adjust);
    Serial.println("Enter new Adjust value, or [Enter]");
    while (!Serial.available());
    if((Serial.peek() == 13) || (Serial.peek() == 10)) nothing = true;
    newAdjust = Serial.parseInt(SKIP_NONE);       // Adjust can be negative
    if ((newAdjust) || !nothing) Adjust = newAdjust;
    Serial.print("Adjust = "); Serial.println(Adjust);
    while(Serial.available()) Serial.read();
    Serial.println("Waiting for IR transmission...");
  }

  if (Waiting) {                                  // keep MSB cleared
    if (timerMSB) timerMSB = 0;
  }

  if(Capture) {                                   // input has changed state
    Capture = false;
    Waiting = false;
    timerLO = ICR1;                               // read timer values
    timerHI = capMSB;
    newTime = (timerHI << 16) + timerLO;          // combine timer counts to one long value
    if (F_CPU ==16000000) newTime = (newTime +1) >> 1;
    if (i) {                                      // skip i=0
      bitTime[i] = newTime - prevTime;            // collect duration and activity state
    }
    prevTime = newTime;                           // prepare for next state change
   TCCR1B ^= 0b01000000;                          // switch to capture on opposite edge
    i++;
  }

  if ((timerMSB > 30) || (i > maxEntries)) {      // collect data for one second after first
    TCCR1B &= 0xFE;                               // stop Timer1 clock
    for (k = 1; k < i; k++) {                     // adjust on (Mark) and off (Space) times
      if (k & 1) bitTime[k] -= Adjust;            // shift Adjust from On to Off
      else bitTime[k] += Adjust;
    }
    Serial.println();                             // display output
    Serial.println("    On    Off     On    Off     On    Off     On    Off");
    Serial.println();
    while (j < i) {
      for (k = 0; k < 8; k++) {
        Serial.print(" ");
        if (bitTime[j] <10000) Serial.print(" ");
        if (bitTime[j] < 1000) Serial.print(" ");
        if (bitTime[j] < 100) Serial.print(" ");
        if (bitTime[j] < 10) Serial.print(" ");
        Serial.print (bitTime[j]);
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
