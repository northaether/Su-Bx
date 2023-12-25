/*
  ATmega4809 Timer Test
  For SuB102B1

  UPDATE    VER   AUTHOR  DESCRIPTION
  20210826  0001  Yo.     New.

  c. f.
  Tom Almy's Blog
  https://tomalmy.com/infrared-barrier-modulating-a-38khz-carrier-at-1khz-with-an-arduino-nano-every/
*/

#define BAUD_RATE 1200

int pinTCB0 = 38; // PF4
int pinTCB3 = 13; // PB5
int pinTXD1 = 14; // PC0
int pinTXD2 = 34; // PF0


void setupTCB() {

  //38kHz two ch

  /* TCB0 */
  TCB0.CTRLA = 0; // Turn off channel for configuring

  TCB0.CTRLB |= 0x07; // CNTMODE[2:0] = b111
  TCB0.CTRLB |= 0x10; // CCMPEN = b1000

  TCB0.CCMPL = 8000000L / 38000L - 1; // output goes high
  TCB0.CCMPH = 8000000L / 38000L / 2; // output goes low

  TCB0.CTRLA |= TCB_ENABLE_bm; // Enable
  TCB0.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2

  /* TCB1 */
  /*
    TCB1.CTRLA = 0; // Turn off channel for configuring

    TCB1.CTRLB |= 0x07; // CNTMODE[2:0] = b111
    TCB1.CTRLB |= 0x10; // CCMPEN = b1000

    TCB1.CCMPL = 8000000L / 38000L - 1; // output goes high
    TCB1.CCMPH = 8000000L / 38000L / 2; // output goes low

    TCB1.CTRLA |= TCB_ENABLE_bm; // Enable
    TCB1.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2
  */

  /* TCB3 */
  TCB3.CTRLA = 0; // Turn off channel for configuring

  TCB3.CTRLB |= 0x07; // CNTMODE[2:0] = b111
  TCB3.CTRLB |= 0x10; // CCMPEN = b1000

  TCB3.CCMPL = 8000000L / 38000L - 1; // output goes high
  TCB3.CCMPH = 8000000L / 38000L / 2; // output goes low

  TCB3.CTRLA |= TCB_ENABLE_bm; // Enable
  TCB3.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2

}


void setup() {

  Serial1.begin(BAUD_RATE);
  Serial2.begin(BAUD_RATE);
  Serial3.begin(115200);

  pinMode(pinTCB0, OUTPUT);
  pinMode(pinTCB3, OUTPUT);

  setupTCB();

  attachInterrupt(pinTXD1, B0_toggle, CHANGE);
  attachInterrupt(pinTXD2, B3_toggle, CHANGE);

}


void B0_On() {
  TCB0.CTRLB |= 0x10;
}

void B0_Off() {
  TCB0.CTRLB &= ~0x10;
}

void B0_toggle() {

  if (digitalRead(pinTXD1)) {
    B0_Off();

  }
  else {
    digitalRead(pinTXD1);
    B0_On();
  }
}




void B3_On() {
  TCB3.CTRLB |= 0x10;
}

void B3_Off() {
  TCB3.CTRLB &= ~0x10;
}

void B3_toggle() {

  if (digitalRead(pinTXD2)) {
    B3_Off();

  }
  else {
    digitalRead(pinTXD2);
    B3_On();
  }
}







ISR(TCB1_INT_vect) {
  //  TCB0.CTRLB ^= 0x10; // Toggle the TCB0 output enable bit (CCMPEN)
  //  TCB1.INTFLAGS = 1; // Clear the interrupt
}

void loop() {

  Serial1.print("1");
  delay(500);

  Serial2.print("2");
  delay(500);

//  
//  if (Serial1.available() > 0) {
//    Serial3.print("U1>");
//    Serial3.write(Serial1.read());
//    Serial3.println("");
//  }
//
//  if (Serial2.available() > 0) {
//    Serial3.print("U2>");
//    Serial3.write(Serial2.read());
//    Serial3.println("");
//  }
}
