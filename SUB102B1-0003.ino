/*
  ATmega4809 Timer Test
  For SuB102B1

  UPDATE    VER   AUTHOR  DESCRIPTION
  20210826  0001  Yo.     New.

  c. f.
  Tom Almy's Blog
  https://tomalmy.com/infrared-barrier-modulating-a-38khz-carrier-at-1khz-with-an-arduino-nano-every/
*/


#include <Adafruit_LEDBackpack.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#define BAUD_RATE 2400

int pinTCB0 = 38; // PF4 Send Right
int pinTCB3 = 13; // PB5 Send Left
int pinTXD1 = 14; // PC0 Send Right
int pinTXD2 = 34; // PF0 Send Left

Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();
static const uint8_t font[] = {
  // Takashi font
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //  0 SP
  0x0E, 0x19, 0x15, 0x13, 0x11, 0x11, 0x0E, 0x00, //  1 0
  0x08, 0x0C, 0x0A, 0x08, 0x08, 0x08, 0x08, 0x00, //  2 1
  0x0E, 0x11, 0x10, 0x08, 0x04, 0x02, 0x1F, 0x00, //  3 2
  0x0E, 0x11, 0x10, 0x0C, 0x10, 0x11, 0x0E, 0x00, //  4 3
  0x08, 0x0C, 0x0A, 0x09, 0x1F, 0x08, 0x08, 0x00, //  5 4
  0x1F, 0x01, 0x0F, 0x10, 0x10, 0x11, 0x0E, 0x00, //  6 5
  0x0E, 0x11, 0x01, 0x0F, 0x11, 0x11, 0x0E, 0x00, //  7 6
  0x1F, 0x10, 0x08, 0x04, 0x02, 0x02, 0x02, 0x00, //  8 7
  0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00, //  9 8
  0x0E, 0x11, 0x11, 0x1E, 0x10, 0x11, 0x0E, 0x00, // 10 9
  0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00, 0x00, // 11 +
  0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, // 12 -
  0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00, 0x00, // 13 x
  0x00, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x00, 0x00, // 14 =
  0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00, 0x00 // 15 UArrow
};













/* TIMER ----------------------------------------------------- */



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

  /* TCB3 */
  TCB3.CTRLA = 0; // Turn off channel for configuring

  TCB3.CTRLB |= 0x07; // CNTMODE[2:0] = b111
  TCB3.CTRLB |= 0x10; // CCMPEN = b1000

  TCB3.CCMPL = 8000000L / 38000L - 1; // output goes high
  TCB3.CCMPH = 8000000L / 38000L / 2; // output goes low

  TCB3.CTRLA |= TCB_ENABLE_bm; // Enable
  TCB3.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2
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



/* LED MATRIX ----------------------------------------------------- */

void disp(int no) {
  for (int i = 0; i < 8; i++) {
    //matrix.displaybuffer[i] =  swapbit(font[no * 8 + i]);//
    matrix.displaybuffer[i] =  font[no * 8 + i];
  }
  matrix.writeDisplay();
  //sendIr(mode);
}


//void dispNow() {
//  int i = modes.indexOf(mode);
//  modeNo = i;
//  disp(modes.indexOf(mode));
//  //sendIr(mode);
//}

//void dispAns(String text) {
//  disp(modes.indexOf(text));
//}





void Init()
{
  //pinMode(PIN_SW, INPUT_PULLUP);

  Serial1.begin(BAUD_RATE);
  //Serial2.begin(BAUD_RATE);
  Serial3.begin(115200);

  pinMode(pinTCB0, OUTPUT);
  pinMode(pinTCB3, OUTPUT);

  setupTCB();
  
  attachInterrupt(pinTXD1, B0_toggle, CHANGE);
  //attachInterrupt(pinTXD2, B3_toggle, CHANGE);
  
  matrix.begin(0x70);  // pass in the address
}




void setup() {

  Init();

}

void loop() {

  for (int i = 0; i < 16; i++)
  {
    disp(i);
    delay(500);
  }
}
