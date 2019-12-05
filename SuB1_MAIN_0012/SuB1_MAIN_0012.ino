/*
  SuB1_MAIN
  Ver   Author  Description
  0002  Yo.     タイマー利用38kHz版とマージ。動作未確認()
  0010  Yo.     LedMatrixで表示できるようにマージ。
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#define BAUDRATE 2400
#define PIN_SW 8

boolean DBG = false;
boolean isOrigin = true;
boolean isSending = false;

String mode = " ";
String modes = " 0123456789+-x=";
int modeNo = 0;
String serialIncoming;
boolean isCmdComplete;

long irRecieved = 0;

#define RECIEVED_SEC 10;
#define ANS_SEC 5;

long dataReceived = 0;
long oneSecTimePrevious = 0;
int dataReceivedSec = RECIEVED_SEC;
int ansSec = 5;



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
/*
  自身のUART Tx 出力を INT0(D2)ピンに入力
  INT0 CHANGE で割込み発生
  TCCR1AレジスタにてOC1Aの出力を下記のように設定できる
    (00: 切断, 01: トグル, 10: LOW固定, 11: HIGH固定)
    INT0割込み時 LOW (UART ON) ならばTCCR1Aレジスタをトグル設定で38kHz出力
    INT0割込み時 HIGH (UART OFF) ならばTCCR1AレジスタをLOW固定にするとLOW出力
  OC1A(pin38kHzTx/PB1/D9) ピンより38kHz / LOW 出力
*/

int pin38kHzTx = 9;
int pin38kHzRef = 10;
int pinTxIn = 2; // int0

void initTimer1()
{

  // ************ 38KHz ***************
  // Xtal  8 MHz
  // PreScaler 1 ^-1
  // T( 8MHz) = 0.125 μs
  // T(38kHz) = 26.316us
  // 1/2T(38kHz) = 13.158us
  // 1/2T(38kHz)毎にH, Lを切り替えると38kHz出力
  // 13.158us / 0.125us = 105.2
  // T(8MHz)が105回でオーバーフロー出力
  // 1 / (0.125us * 105回) = 38.095kHz
  //
  // OCR1A = 105 - 1 = 104

  OC1A_Low();
  OC1B_High();

  // Set Timer/Counter1 to CTC mode (4)
  TCCR1B &= ~(1 << WGM13); // 0
  TCCR1B |= (1 << WGM12);  // 1
  TCCR1A &= ~(1 << WGM11); // 0
  TCCR1A &= ~(1 << WGM10); // 0

  // Set CS bits to No Prescaling (001)
  TCCR1B &= ~(1 << CS12); // 0
  TCCR1B &= ~(1 << CS11); // 0
  TCCR1B |= (1 << CS10);  // 1

  OCR1A = 104;
}

/* OC1A (PB1/D9) */
void OC1A_Disconnect()
{
  TCCR1A &= ~(1 << COM1A1); // 0
  TCCR1A &= ~(1 << COM1A0); // 0
}
void OC1A_toggle()
{
  TCCR1A &= ~(1 << COM1A1); // 0
  TCCR1A |= (1 << COM1A0);  // 1
}
void OC1A_Low()
{
  TCCR1A |= (1 << COM1A1);  // 1
  TCCR1A &= ~(1 << COM1A0); // 0
}
void OC1A_High()
{
  TCCR1A |= (1 << COM1A1); // 1
  TCCR1A |= (1 << COM1A0); // 1
}

/* OC1B (PB2/D10) */
void OC1B_Disconnect()
{
  TCCR1A &= ~(1 << COM1B1); // 0
  TCCR1A &= ~(1 << COM1B0); // 0
}
void OC1B_toggle()
{
  TCCR1A &= ~(1 << COM1B1); // 0
  TCCR1A |= (1 << COM1B0);  // 1
}
void OC1B_Low()
{
  TCCR1A |= (1 << COM1B1);  // 1
  TCCR1A &= ~(1 << COM1B0); // 0
}
void OC1B_High()
{
  TCCR1A |= (1 << COM1B1); // 1
  TCCR1A |= (1 << COM1B0); // 1
}

void modUartTx()
{
  // pinTxInがON(Low)の時、OC1A(PB1/D9)に38.1kHzを出力
  if (!digitalRead(pinTxIn))
  {
    OC1A_toggle();
  }
  else
  {
    OC1A_Low();
  }
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


//unsigned swapbit(uint8_t x) {
//  uint8_t r = 0;
//  for (int i = 0; i < 5; i++) {
//    r |= ((x >> i & 1) << (4 - i));
//  }
//  return r;
//}
//

void dispNow() {
  int i = modes.indexOf(mode);
  modeNo = i;
  disp(modes.indexOf(mode));
  //sendIr(mode);
}

void dispAns(String text) {

  disp(modes.indexOf(text));



}


/*
   _|_|_|  _|_|_|_|  _|_|_|    _|_|_|    _|_|    _|
  _|        _|       _|    _|    _|    _|    _|  _|          _|_|_|
   _|_|    _|_|_|    _|_|_|      _|    _|_|_|_|  _|        _|_|
       _|  _|        _|    _|    _|    _|    _|  _|            _|_|
  _|_|_|    _|_|_|_|  _|    _|  _|_|_|  _|    _|  _|_|_|_|  _|_|_|

*/


void serialInit()
{
  Serial.begin(2400);
  //  serialIncoming.reserve(200);
}

void serialCheck()
{

  if (isSending) {
    Serial.println("S");
    return;
  }

  serialIncomingCheck();
  serialComplete();
}

void serialIncomingCheck()
{

  if (isSending) {
    while (Serial.available()) {
      Serial.read();
    }
    return;
  }

  while (Serial.available())
  {
    char inChar = (char)Serial.read();

    if (inChar == '\n')
    {
      isCmdComplete = true;
    }
    else if (inChar == '\r')
    {
      // Ignore CR (do nothing)
    }
    else
    {
      serialIncoming += inChar;
    }
  }
}

void serialComplete()
{
  // Serial
  if (isCmdComplete)
  {
    serialCommandRecieve(serialIncoming);
    serialIncoming = "";
    isCmdComplete = false;
  }
}

void serialCommandRecieve(String command)
{


  //  String n = String(command.charAt(0));
  //
  //  if (n == "#")
  //    return;
  //  else if (n == "0")
  //    mode = "0";
  //  else if (n == "1")
  //    mode = "1";
  //  else if (n == "2")
  //    mode = "2";
  //  else if (n == "3")
  //    mode = "3";
  //  else if (n == "4")
  //    mode = "4";
  //  else if (n == "5")
  //    mode = "5";
  //  else if (n == "6")
  //    mode = "6";
  //  else if (n == "7")
  //    mode = "7";
  //  else if (n == "8")
  //    mode = "8";
  //  else if (n == "9")
  //    mode = "9";
  //  else if (n == "+")
  //    mode = "+";
  //  else if (n == "-")
  //    mode = "-";
  //  else if (n == "x")
  //    mode = "x";
  //  else if (n == "=")
  //    mode = "=";
  //  else if (n == " ")
  //    mode = " ";
  //
  //
  //  dispNow();

  //dbg("incoming = " + n);

  /* debug */
  if (false) {
    // デバグ用コマンド
    if (command.length() > 1)
    {
      if (command.charAt(0) == '#')
      {
        String n = String(command.charAt(1));
        dbg(">" + command);

        if (n == "#")
          return;
        else if (n == "0")
          mode = "0";
        else if (n == "1")
          mode = "1";
        else if (n == "2")
          mode = "2";
        else if (n == "3")
          mode = "3";
        else if (n == "4")
          mode = "4";
        else if (n == "5")
          mode = "5";
        else if (n == "6")
          mode = "6";
        else if (n == "7")
          mode = "7";
        else if (n == "8")
          mode = "8";
        else if (n == "9")
          mode = "9";
        else if (n == "+")
          mode = "+";
        else if (n == "-")
          mode = "-";
        else if (n == "x")
          mode = "x";
        else if (n == "=")
          mode = "=";
        else if (n == " ")
          mode = " ";

        else if (n == "U")
        {
          incrementChar();
          return;
        }
        else if (n == "D")
        {
          decrementChar();
          return;
        }
        else
          mode = " ";

        dbg("mode = " + mode);
      }
    }
  } // end if


  // コマンドが1文字以下ならば終了
  if (command.length() < 1)
    return;
  {
    // 受信タイマーをリセット
    dataReceivedSec = RECIEVED_SEC;
    if (command.charAt(0) == 'L')
    {
      // 今自分が=ならば
      if (mode == "=")
      {
        exeEquals(command.substring(1));
        return;
      }
      // 左辺を実行
      exeLeftSide(command.substring(1));
      return;
    }

    if (command.charAt(0) == 'R')
    {
      // 右辺を実行
      if (mode == " ") {
        exeRightSide(command.substring(1));
      }
      return;
    }
  }
}

/*
  _|      _|  _|_|_|_|  _|_|_|_|_|  _|    _|    _|_|    _|_|_|
  _|_|  _|_|  _|            _|      _|    _|  _|    _|  _|    _|    _|_|_|
  _|  _|  _|  _|_|_|        _|      _|_|_|_|  _|    _|  _|    _|  _|_|
  _|      _|  _|            _|      _|    _|  _|    _|  _|    _|      _|_|
  _|      _|  _|_|_|_|      _|      _|    _|    _|_|    _|_|_|    _|_|_|
*/

void Init()
{
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.begin(BAUDRATE);
  matrix.begin(0x70);  // pass in the address
}



void incrementChar()
{
  int i = modes.indexOf(mode);
  if (i == modes.length() - 1)
  {
    i = 0;
  }
  else
  {
    i++;
  }
  disp(i);
  mode = String(modes.charAt(i));
  dbg("mode = " + mode);
}

void decrementChar()
{
  int i = modes.indexOf(mode);
  if (i == 0)
  {
    i = modes.length() - 1;
  }
  else
  {
    i--;
  }

  mode = String(modes.charAt(i));
  dbg("mode = " + mode);
}

boolean isPushed = false;
void checkMode() {
  //Serial.println(digitalRead(PIN_SW));
  // 1 = OFF, 0 = ON

  if (digitalRead(PIN_SW) == 0) {
    if (!isPushed) {
      isPushed = true;
      incrementChar();
    }
    delay(10);
  } else {
    isPushed = false;
  }
}

void exeLeftSide(String text)
{
  // 自分が" "なら何もしない
  if (mode == " ")
  {
    return;
  }
  delay(100);
  sendIr("L" + text + mode);
}

void exeEquals(String text)
{
  String op = "+-x";
  int opNo = -1;
  int opIndex = -1;
  int idx = -1;

  // 演算子のインデックスを探す
  for (int no = 0; no < op.length(); no++)
  {
    idx = text.indexOf(op[no]);

    // 演算子があれば、forを出る
    if (idx > 0)
    {
      opNo = no;
      opIndex = idx;
      break;
    }
  }

  // 演算子がない場合
  if (idx < 0)
  {
    sendIr("R" + text);
    return;
  }

  // 演算子の左側
  int a = text.substring(0, opIndex).toInt();

  // 演算子の右側
  int b = text.substring(opIndex + 1).toInt();

  // 計算
  int ans = 0;
  switch (opNo)
  {
    case 0:
      sendIr("R" + String(plus(a, b)));
      break;
    case 1:
      sendIr("R" + String(minus(a, b)));
      break;
    case 2:
      sendIr("R" + String(mult(a, b)));
      break;
    default:
      sendIr("R?");
      break;
  }
}

void exeRightSide(String text)
{
  ansSec = ANS_SEC;

  // Serial.print("###");
  // Serial.println(text.substring(0,1));


  dispAns(text.substring(0, 1));

}

void exe()
{

  //
}



void sendIr(String text)
{
  isSending = true;

  delay(10);
  Serial.println(text);
  Serial.flush();
  delay(10);
  while (Serial.available()) {
    Serial.read();
  }

  serialIncoming = "";
  isSending = false;
}

/* misc */

int plus(int a, int b)
{
  return a + b;
}
int minus(int a, int b)
{
  return a - b;
}
int mult(int a, int b)
{
  return a * b;
}

void dbg(String text)
{
  if (DBG)
    Serial.println("##" + text);
}

/* Pins */

void initPins()
{
  pinMode(pin38kHzTx, OUTPUT);
  pinMode(pin38kHzRef, OUTPUT);
  pinMode(pinTxIn, INPUT_PULLUP);
}

/*
  _|_|_|  _|_|_|_|  _|_|_|_|_|  _|    _|  _|_|_|
  _|        _|            _|      _|    _|  _|    _|
  _|_|    _|_|_|        _|      _|    _|  _|_|_|
      _|  _|            _|      _|    _|  _|
  _|_|_|    _|_|_|_|      _|        _|_|    _|
*/
void setup()
{
  initPins();
  initTimer1();
  serialInit();
  attachInterrupt(0, modUartTx, CHANGE);

  Init();
  for (int i = 0; i < 16; i++) {
    disp(i);
    delay(100);
  }
  mode = " ";
  dispNow();
  // disp(modeNo);

}

/*
  _|          _|_|      _|_|    _|_|_|
  _|        _|    _|  _|    _|  _|    _|
  _|        _|    _|  _|    _|  _|_|_|
  _|        _|    _|  _|    _|  _|
  _|_|_|_|    _|_|      _|_|    _|
*/




void oneSecTimer() {
  long now = millis();
  if (oneSecTimePrevious + 1000 < now) {
    oneSec();
    oneSecTimePrevious = millis();
  }
}




void oneSec() {


  if (ansSec > 0) {
    ansSec--;
  }

  if (ansSec == 0) {
    dispNow();
  }

  if (dataReceivedSec > 0) {
    dataReceivedSec--;
  }

  if (mode == " ") return;
  if (mode == "+") return;
  if (mode == "-") return;
  if (mode == "x") return;
  if (mode == "=") return;

  if (dataReceivedSec == 0) {
    delay(100);
    sendIr("L" + mode);
  }
}


void loop()
{
  serialCheck();
  checkMode();

  //  if (mode != "=")

  oneSecTimer();



  //Serial.println("hello world!");
}
