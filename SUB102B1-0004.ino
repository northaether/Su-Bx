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
#define PIN_SW 26

int pinTCB0 = 38; // PF4
int pinTCB3 = 13; // PB5
int pinTXD1 = 14; // PC0
int pinTXD2 = 34; // PF0

boolean DBG = true;
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
    0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x00, 0x00  // 15 UArrow
};

/* TIMER ----------------------------------------------------- */

void setupTCB()
{

  // 38kHz two ch

  /* TCB0 */
  TCB0.CTRLA = 0; // Turn off channel for configuring

  TCB0.CTRLB |= 0x07; // CNTMODE[2:0] = b111
  TCB0.CTRLB |= 0x10; // CCMPEN = b1000

  TCB0.CCMPL = 8000000L / 38000L - 1; // output goes high
  TCB0.CCMPH = 8000000L / 38000L / 2; // output goes low

  TCB0.CTRLA |= TCB_ENABLE_bm;         // Enable
  TCB0.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2

  /* TCB3 */
  TCB3.CTRLA = 0; // Turn off channel for configuring

  TCB3.CTRLB |= 0x07; // CNTMODE[2:0] = b111
  TCB3.CTRLB |= 0x10; // CCMPEN = b1000

  TCB3.CCMPL = 8000000L / 38000L - 1; // output goes high
  TCB3.CCMPH = 8000000L / 38000L / 2; // output goes low

  TCB3.CTRLA |= TCB_ENABLE_bm;         // Enable
  TCB3.CTRLA |= TCB_CLKSEL_CLKDIV2_gc; // use system clock/2
}

void B0_On()
{
  TCB0.CTRLB |= 0x10;
}

void B0_Off()
{
  TCB0.CTRLB &= ~0x10;
}

void B0_toggle()
{

  if (digitalRead(pinTXD1))
  {
    B0_Off();
  }
  else
  {
    digitalRead(pinTXD1);
    B0_On();
  }
}

void B3_On()
{
  TCB3.CTRLB |= 0x10;
}

void B3_Off()
{
  TCB3.CTRLB &= ~0x10;
}

void B3_toggle()
{

  if (digitalRead(pinTXD2))
  {
    B3_Off();
  }
  else
  {
    digitalRead(pinTXD2);
    B3_On();
  }
}

ISR(TCB1_INT_vect)
{
  //  TCB0.CTRLB ^= 0x10; // Toggle the TCB0 output enable bit (CCMPEN)
  //  TCB1.INTFLAGS = 1; // Clear the interrupt
}

/* LED MATRIX ----------------------------------------------------- */

void disp(int no)
{
  for (int i = 0; i < 8; i++)
  {
    // matrix.displaybuffer[i] =  swapbit(font[no * 8 + i]);//
    matrix.displaybuffer[i] = font[no * 8 + i];
  }
  matrix.writeDisplay();
  // sendIr(mode);
}

void dispNow()
{
  int i = modes.indexOf(mode);
  modeNo = i;
  disp(modes.indexOf(mode));
  // sendIr(mode);
}

void dispAns(String text)
{
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
  Serial1.begin(BAUD_RATE);
  // Serial2.begin(BAUD_RATE);
  Serial3.begin(115200);

  serialIncoming.reserve(200);
}

void serialCheck()
{

  //  if (isSending) {
  //    //Serial3.println("S");
  //    return;
  //  }

  serialIncomingCheck();
  serialComplete();
}

void serialIncomingCheck()
{

  if (isSending)
  {
    while (Serial1.available())
    {
      Serial1.read();
    }
    return;
  }

  while (Serial1.available())
  {
    char inChar = (char)Serial1.read();
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
      if (serialIncoming.length() >= 200)
      {
        serialIncoming = "";
      }
      serialIncoming += inChar;
      // Serial3.println("o>" + inChar);
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
  Serial3.println(">" + command);
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

  // dbg("incoming = " + n);

  /* debug */
  if (false)
  {
    // デバグ用コマンド
    if (command.length() > 1)
    {
      if (command.charAt(0) == '#')
      {
        String n = String(command.charAt(1));
        // dbg(">" + command);

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
      if (mode == " ")
      {
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
  pinMode(pinTCB0, OUTPUT);
  pinMode(pinTCB3, OUTPUT);

  setupTCB();

  attachInterrupt(pinTXD1, B0_toggle, CHANGE);
  attachInterrupt(pinTXD2, B3_toggle, CHANGE);

  matrix.begin(0x70); // pass in the address
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
void checkMode()
{
  // Serial.println(digitalRead(PIN_SW));
  //  1 = OFF, 0 = ON

  if (digitalRead(PIN_SW) == 0)
  {
    if (!isPushed)
    {
      isPushed = true;
      incrementChar();
    }
    delay(10);
  }
  else
  {
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

  // 一桁なら
  if (text.length() == 1)
  {
    dispAns(text.substring(0, 1));
  }
  // 二桁以上なら ↑を表示
  else if (text.length() > 1)
  {
    disp(15);
  }
}

void exe()
{

  //
}

void sendIr(String text)
{
  isSending = true;

  delay(10);
  Serial1.println(text);
  Serial3.println("<" + text);
  while (Serial1.available())
  {
    Serial1.read();
  }

  serialIncoming = "";
  delay(10);

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
    Serial3.println("##" + text);
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
  // initPins();
  // initTimer1();
  serialInit();
  // attachInterrupt(0, modUartTx, CHANGE);

  Init();
  for (int i = 0; i < 16; i++)
  {
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

void oneSecTimer()
{
  long now = millis();
  if (oneSecTimePrevious + 1000 < now)
  {
    oneSec();
    oneSecTimePrevious = millis();
  }
}

void oneSec()
{

  if (ansSec > 0)
  {
    ansSec--;
  }

  if (ansSec == 0)
  {
    dispNow();
  }

  if (dataReceivedSec > 0)
  {
    dataReceivedSec--;
  }

  if (mode == " ")
    return;
  if (mode == "+")
    return;
  if (mode == "-")
    return;
  if (mode == "x")
    return;
  if (mode == "=")
    return;

  if (dataReceivedSec == 0)
  {
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

  // Serial.println("hello world!");
}
