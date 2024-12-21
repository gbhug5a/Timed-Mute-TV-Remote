// Dump the signature block from an Uno, Nano or Pro Mini
// to help detect counterfeit Atmega328P chips.
// See https://www.youtube.com/watch?v=eeDC1m7ANJI
// for how to interpret the output.
// Thanks to Kevin Darrah for this code and the video.

#include <avr/boot.h>
#define SIGRD 5
void setup() {
  Serial.begin(9600);
  Serial.println("");
  Serial.println("boot sig dump");
  int newLineIndex = 0;
  for (uint8_t i = 0; i <= 0x1F; i += 1) {
    Serial.print(boot_signature_byte_get(i), HEX);
    Serial.print("\t");
    newLineIndex++;
    if (newLineIndex == 8) {
      Serial.println("");
      newLineIndex = 0;
    }
  }
  Serial.println();
}

void loop() {
}
