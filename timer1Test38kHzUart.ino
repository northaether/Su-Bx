/*
  for ProMini 8MHz
  OC1A(PB1/D9) 38kHz
*/

int pin38kHzTx = 9;
int pin38kHzRef = 10;
int pinTxIn = 2; // int0

void initTimer1() {

  // ************ 38KHz ***************
  // Xtal  8 MHz
  // PreScaler 1 ^-1
  // T 0.125 μs
  // count 105 -> OCR1A = 105 - 1
  // Freq  38.10 kHz

  OC1A_Low();
  OC1B_High();

  // Set Timer/Counter1 to CTC mode (4)
  TCCR1B &= ~(1 << WGM13);    // 0
  TCCR1B |=  (1 << WGM12);    // 1
  TCCR1A &= ~(1 << WGM11);    // 0
  TCCR1A &= ~(1 << WGM10);    // 0

  // Set CS bits to No Prescaling (001)
  TCCR1B &= ~(1 << CS12);     // 0
  TCCR1B &= ~(1 << CS11);     // 0
  TCCR1B |=  (1 << CS10);     // 1

  OCR1A = 104;

}

/* OC1A (PB1/D9) */
void OC1A_Disconnect() {
  TCCR1A &= ~(1 << COM1A1);   // 0
  TCCR1A &= ~(1 << COM1A0);   // 0
}
void OC1A_toggle() {
  TCCR1A &= ~(1 << COM1A1);   // 0
  TCCR1A |=  (1 << COM1A0);   // 1
}
void OC1A_Low() {
  TCCR1A |=  (1 << COM1A1);   // 1
  TCCR1A &= ~(1 << COM1A0);   // 0
}
void OC1A_High() {
  TCCR1A |=  (1 << COM1A1);   // 1
  TCCR1A |=  (1 << COM1A0);   // 1
}

/* OC1B (PB2/D10) */
void OC1B_Disconnect() {
  TCCR1A &= ~(1 << COM1B1);   // 0
  TCCR1A &= ~(1 << COM1B0);   // 0
}
void OC1B_toggle() {
  TCCR1A &= ~(1 << COM1B1);   // 0
  TCCR1A |=  (1 << COM1B0);   // 1
}
void OC1B_Low() {
  TCCR1A |=  (1 << COM1B1);   // 1
  TCCR1A &= ~(1 << COM1B0);   // 0
}
void OC1B_High() {
  TCCR1A |=  (1 << COM1B1);   // 1
  TCCR1A |=  (1 << COM1B0);   // 1
}

void modUartTx () {
  if (!digitalRead(pinTxIn)) {
    OC1A_toggle();
  } else {
    OC1A_Low();
  }
}

void initPins() {
  pinMode(pin38kHzTx, OUTPUT);
  pinMode(pin38kHzRef, OUTPUT);
  pinMode(pinTxIn, INPUT_PULLUP);
}

void setup() {
  initPins();
  initTimer1();
  Serial.begin(2400);
  attachInterrupt(0, modUartTx, CHANGE);
}

void loop() {
  OC1B_toggle();
  Serial.println("YI_LAB");
  delay(10);
  OC1B_Low();
  delay(40);
}
