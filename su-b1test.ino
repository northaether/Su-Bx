

#include <Wire.h>
#define LDaddrs1 (0x70)  //LEDドライバーHT16K33　アドレス

byte LedDot_YI[8] = { //"YI"という文字をドット化
  B00000000,
  B00010101,
  B00010101,
  B00010010,
  B00010010,
  B00010010,
  B00000000,
  B00000000
};
byte LedDot_ALL[8] = { //てんとう
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B00000000
};
byte LedDot0[8] = {
                  0,0,0,0,0,0,0,0
                  };

void setup() {
  // put your setup code here, to run once:

  Wire.begin(); // initialise the connection
  Wire.setClock(400000L); //HT16K33のクロックはMax 400kHz
  LED_Driver_Setup( LDaddrs1, 1); //HT16K33システムオシレータ　ＯＮ
  LED_Driver_Blink( LDaddrs1, 1, 0); //blink_Hz=0 点滅off, 1は2Hz, 2は1Hz, 3は0.5Hz, on_off=0は消灯、1は点灯
  LED_Driver_Brightness( LDaddrs1, 15 ); // brightness= 0～15
  LED_Driver_DisplayInt( LDaddrs1 ); //Display Black OUT
  delay(1000);

}

void loop() {
  // put your main code here, to run repeatedly:

  LED_Driver_DisplayOutput(LDaddrs1, LedDot_ALL);
  delay(1000);


}
//**********************ＬＥＤドライバ　HT16K33　セットアップ*******************
void LED_Driver_Setup(byte LD_addrs, byte on_off)
{
  //HT16K33は8X8マトリックスLEDを２台まで制御できる。
  //ドライバＩＣ#1 アドレスは0x70から設定する。
  Wire.beginTransmission(LD_addrs);
  Wire.write(0x20 | on_off);  //システムオシレータをONにする
  Wire.endTransmission();
}
//**********************ＬＥＤドライバ　HT16K33　点滅周期設定*******************
void LED_Driver_Blink(byte LD_addrs, byte on_off, byte blink_Hz)
{
  //blink_Hz=0 点滅off, 1は2Hz, 2は1Hz, 3は0.5Hz, on_off=0は消灯、1は点灯　
  Wire.beginTransmission(LD_addrs);
  Wire.write(0x80 | (blink_Hz << 1) | on_off);
  Wire.endTransmission();
}
//**********************ＬＥＤドライバ　HT16K33　明るさ設定*********************
void LED_Driver_Brightness(byte LD_addrs, byte brightness)
{
  // brightness= 0～15
  Wire.beginTransmission(LD_addrs);
  Wire.write(0xE0 | brightness);
  Wire.endTransmission();
}
//**********************ＬＥＤドライバ　HT16K33　画面初期化*********************
void LED_Driver_DisplayInt(byte LD_addrs)
{
  Wire.beginTransmission(LD_addrs);
  Wire.write(0x00);
  for (int i = 0; i < 8; i++) {
    Wire.write(B00000000);      //8x8LED初期化
    Wire.write(B00000000); //
  }
  Wire.endTransmission();
}

void LED_Driver_DisplayOutput(byte LD_addrs, byte* Dot)
{
  int i,j;
    Wire.beginTransmission(LD_addrs);
    Wire.write(B00000000); //これは必要
    for(i = 0; i<8; i++){
      Wire.write(Dot[i])   ; //Row0-7 use
      Wire.write(B00000000); //ROW8-15 not use
    }
    Wire.endTransmission();
}
