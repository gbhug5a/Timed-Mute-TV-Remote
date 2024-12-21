/*

This sketch converts the bit order of a hex value you type in.
It can convert MSB-first to LSB-first, or vice versa.
Set Serial Monitor line ending to Newline.

*/

unsigned long pointer, output, inverse;
char buff [11];
char d;
int buffSize, i, j;
byte c;

void setup () {
  Serial.begin (9600);
  Serial.println ("Convert between MSB and LSB, both HEX.");
  Serial.println ("Enter up to 8 HEX digits:");
}
void loop() {

  if (Serial.available()) {              // process input from Serial Monitor

    d = Serial.read();                   // set end-line option to Newline or CR
    if ((d == 13) || (d == 10)) {
      buff[buffSize] = 0;
      parse_cmd();
      buffSize = 0;
      buff[0] = 0;
      delay(200);
      if (Serial.available()) Serial.read();  // dump second line end char if present
    }
    else {
      buff[buffSize] = d;
      buffSize++;
    }
  }
}

void parse_cmd() {
  output = 0;
  for (i = 0; i < buffSize; i++) {
    c = buff[i];
    if (c > 0x39) c -= 7;
    c &= 0xF;
    output <<= 4;
    output |= c;
  }
  i = buffSize << 2;

  pointer = 1;
  inverse = 0;

  for (j = 0; j < i; j++) {
    inverse <<= 1;
    if (output & pointer) inverse |= 1;
    pointer <<= 1;
  }

  Serial.print (output,HEX);
  Serial.print ("  ");
  Serial.println (inverse,HEX);
  Serial.println();
}
