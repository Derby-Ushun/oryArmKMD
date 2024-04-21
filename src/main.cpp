#include <Arduino.h>
#include <Dynamixel.h>
#include <SPIFFS.h>
#include "BluetoothSerial.h"


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define DYNAMIXEL_SERIAL Serial2

BluetoothSerial SerialBT;


// ---- S/W Version ------------------
#define VERSION_NUMBER  "Ver. 0.4.0"
// -----------------------------------



////////// pin 配置 //////////////////////////
const int led01 = 21;
const int led02 = 22;
const int led03 = 23;
const int led04 = 25;
const int led05 = 26;
const int led06 = 27;

const int sw00 = 32;
const int sw01 = 4;
const int sw02 = 5;
const int sw03 = 12;
const int sw04 = 13;
const int sw05 = 33;

const int swAudio = 14;

const int RXD1 = 19;
const int TXD1 = 18;
const int RXD2 = 16;
const int TXD2 = 17;

////////////////////////////////////////////

Dynamixel dxl(RXD2, TXD2);

const uint8_t TARGET_ID1 = 1;
const uint8_t TARGET_ID2 = 2;
const uint8_t TARGET_ID3 = 3;
const uint8_t TARGET_ID4 = 4;
const uint8_t TARGET_ID5 = 5;
const uint8_t TARGET_ID6 = 6;
const uint8_t TARGET_ID7 = 7;
const uint8_t TARGET_ID8 = 8;



bool onlyLeftArm = false; //左手のみを使用するかどうか
int exception = 1900;   //手が全開で脱力する時の閾値 左腕:1800


const uint8_t PIN_RTS = 11;
// const uint8_t DYNAMIXEL_BAUDRATE = 57600;


const int numRecords = 250;
int number = 9; //←ここの数字はID＋１とする
int values[numRecords * 9 ]; //←ここの数字はID＋１とする
char buffer[16]; // 数値の一時的な保持のためのバッファ




bool sw00State, sw01State, sw02State, sw03State, sw04State, sw05State, swAudioState = HIGH;

int targetPos01, targetPos02, targetPos03, targetPos04, targetPos05, targetPos06, targetPos07, targetPos08 = 0;

int s1, s2, s3, s4, s5, s6, s7, s8 = 0;

char receivedChar = 0;


const int s1diference = 500; //id:01モータの目標値と実測値の差分 この差分を超えるとモーションを終了する
const int s2diference = 400; //id:02モータの目標値と実測値の差分 この差分を超えるとモーションを終了する
const int s4diference = 800; //id:04モータの目標値と実測値の差分 この差分を超えるとモーションを終了する

int z = 0; //フリーの時、落下防止に動きを遅くするフラグ
int ran1, ran2, ran4 = 0;

int s08 = 0; //腕の角度

int mode = 0; //1~9:モーション録画, 11~19:モーション再生
int audioMode = 0; //0:初期値, 

int O_time = 0;
const int O_t = 100;

int past_mode = 0;


int serialnumberP = 300;
int serialnumberI = 2;
int serialnumberD = 1;


File file;

// グローバル変数としてdata配列を定義
const int readInterval = 100; // 読み取り間隔(ms)
const int totalReads = 100; // 10秒間に100回の読み取り
int data[totalReads][8]; // [読み取り回数][サーボモータの数]
int dataIndex = 0; // 現在の読み取りインデックス



void turnOnLed(int led){
  digitalWrite(led, HIGH);
}

void turnOnAllLed(){
  digitalWrite(led01, HIGH);
  digitalWrite(led02, HIGH);
  digitalWrite(led03, HIGH);
  digitalWrite(led04, HIGH);
  digitalWrite(led05, HIGH);

}

void turnOffLed(){
  digitalWrite(led01, LOW);
  digitalWrite(led02, LOW);
  digitalWrite(led03, LOW);
  digitalWrite(led04, LOW);
  digitalWrite(led05, LOW);
}

void readSwitchState(){
  sw00State = digitalRead(sw00);
  sw01State = digitalRead(sw01);
  sw02State = digitalRead(sw02);
  sw03State = digitalRead(sw03);
  sw04State = digitalRead(sw04);
  sw05State = digitalRead(sw05);
  swAudioState = digitalRead(swAudio);
}

void Pgain_on(){
  if(serialnumberI>1) {
  dxl.positionPGain(TARGET_ID1, serialnumberP);
  dxl.positionPGain(TARGET_ID2, serialnumberP);
  dxl.positionPGain(TARGET_ID3, 700);
  dxl.positionPGain(TARGET_ID4, serialnumberP);
  dxl.positionPGain(TARGET_ID6, 500);
  dxl.positionPGain(TARGET_ID7, 500);
  dxl.positionPGain(TARGET_ID8, 1200);

  // dxl.positionIGain(TARGET_ID1, serialnumberI);
  // dxl.positionIGain(TARGET_ID2, serialnumberI);
  // dxl.positionIGain(TARGET_ID3, serialnumberI);
  // dxl.positionIGain(TARGET_ID4, serialnumberI);
  // dxl.positionIGain(TARGET_ID5, serialnumberI);
  // dxl.positionIGain(TARGET_ID6, serialnumberI);
  // dxl.positionIGain(TARGET_ID7, serialnumberI);
  // dxl.positionIGain(TARGET_ID8, serialnumberI);
  }
}


void range(int a1, int a2) {
  int r = 230;
  if (a1 < a2 + r && a2 - r < a1)z = z + 1;
}

void zero() { //フリーの時、落下防止に動きを遅くするフラグをONにする
  z = 1;
  int de = 1; delay(de);
  targetPos01 = dxl.presentPosition(TARGET_ID1); delay(de);
  targetPos02 = dxl.presentPosition(TARGET_ID2); delay(de);
  targetPos04 = dxl.presentPosition(TARGET_ID4); delay(de);
  targetPos08 = dxl.presentPosition(TARGET_ID8);
  s08 = targetPos08;

  range(targetPos01, ran1);
  range(targetPos02, ran2);
  range(targetPos04, ran4);

  //range(s1,730);
  //range(s2,1000);
  //range(s4,3000);

  if (z == 4)z = 0;
  Serial.print(z);
  Serial.print(":");
}

void slow() { //条件がONの時、スローにする。ただし腕が全開の時は例外とする


  // if (s08 < exception) { //腕が全開の時の数字
  //   // digitalWrite(led01, LOW);
  //   // digitalWrite(led02, HIGH);
  //   // digitalWrite(led03, LOW);
  // }

  if (onlyLeftArm == true){
    if (z > 0 && s08 < exception) {
      dxl.positionPGain(TARGET_ID1, 1);
      dxl.positionPGain(TARGET_ID2, 1);
      dxl.positionPGain(TARGET_ID3, 1);
      dxl.positionPGain(TARGET_ID4, 1);
      dxl.positionPGain(TARGET_ID6, 1);
      dxl.positionPGain(TARGET_ID7, 1);
      dxl.positionPGain(TARGET_ID8, 1);

      int de = 1;

      dxl.torqueEnable(TARGET_ID1, false);
      dxl.torqueEnable(TARGET_ID2, false);
      dxl.torqueEnable(TARGET_ID4, false);
      dxl.torqueEnable(TARGET_ID6, false);
      delay(de);
      dxl.torqueEnable(TARGET_ID1, true);
      dxl.torqueEnable(TARGET_ID2, true);
      dxl.torqueEnable(TARGET_ID4, true);
      dxl.torqueEnable(TARGET_ID6, true);
      delay(12);

    } else {
      dxl.torqueEnable(TARGET_ID1, false);
      dxl.torqueEnable(TARGET_ID2, false);
      dxl.torqueEnable(TARGET_ID4, false);
      dxl.torqueEnable(TARGET_ID6, false);
    }
  } else {
    if (z > 0 && s08 > exception) {
      dxl.positionPGain(TARGET_ID1, 1);
      dxl.positionPGain(TARGET_ID2, 1);
      dxl.positionPGain(TARGET_ID3, 1);
      dxl.positionPGain(TARGET_ID4, 1);
      dxl.positionPGain(TARGET_ID6, 1);
      dxl.positionPGain(TARGET_ID7, 1);
      dxl.positionPGain(TARGET_ID8, 1);

      int de = 1;

      dxl.torqueEnable(TARGET_ID1, false);
      dxl.torqueEnable(TARGET_ID2, false);
      dxl.torqueEnable(TARGET_ID4, false);
      dxl.torqueEnable(TARGET_ID6, false);
      delay(de);
      dxl.torqueEnable(TARGET_ID1, true);
      dxl.torqueEnable(TARGET_ID2, true);
      dxl.torqueEnable(TARGET_ID4, true);
      dxl.torqueEnable(TARGET_ID6, true);
      delay(12);

    } else {
      dxl.torqueEnable(TARGET_ID1, false);
      dxl.torqueEnable(TARGET_ID2, false);
      dxl.torqueEnable(TARGET_ID4, false);
      dxl.torqueEnable(TARGET_ID6, false);
    }
  }
  
}




void demo() {

  int de = 5;
  delay(de);
  targetPos01 = dxl.presentPosition(TARGET_ID1); delay(de);
  targetPos02 = dxl.presentPosition(TARGET_ID2); delay(de);
  targetPos03 = dxl.presentPosition(TARGET_ID3); delay(de);
  targetPos04 = dxl.presentPosition(TARGET_ID4); delay(de);
  targetPos05 = dxl.presentPosition(TARGET_ID5); delay(de);
  targetPos06 = dxl.presentPosition(TARGET_ID6); delay(de);
  targetPos07 = dxl.presentPosition(TARGET_ID7); delay(de);
  targetPos08 = dxl.presentPosition(TARGET_ID8);

  Serial.print(" demo = ");
  Serial.print(targetPos01); Serial.print(", ");
  Serial.print(targetPos02); Serial.print(", ");
  Serial.print(targetPos03); Serial.print(", ");
  Serial.print(targetPos04); Serial.print(", ");
  Serial.print(targetPos05); Serial.print(", ");
  Serial.print(targetPos06); Serial.print(", ");
  Serial.print(targetPos07); Serial.print(", ");
  Serial.println(targetPos08);

  zero();
  slow();
}


void writer(){
  
  if (mode < 10) {
    if (mode == 1) {
      // ファイルの作成とデータの書き込み
      file = SPIFFS.open("/test1.txt", FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        return;
      }
    }
    if (mode == 2) {
      // ファイルの作成とデータの書き込み
      file = SPIFFS.open("/test2.txt", FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        return;
      }
    }
    if (mode == 3) {
      // ファイルの作成とデータの書き込み
      file = SPIFFS.open("/test3.txt", FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        return;
      }
    }
    if (mode == 4) {
      // ファイルの作成とデータの書き込み
      file = SPIFFS.open("/test4.txt", FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        return;
      }
    }
    if (mode == 5) {
      // ファイルの作成とデータの書き込み
      file = SPIFFS.open("/test5.txt", FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        return;
      }
    }

    //レコード開始の合図
    digitalWrite(led01, LOW); digitalWrite(led02, LOW); digitalWrite(led03, LOW);
    delay(200);
    digitalWrite(led01, HIGH); digitalWrite(led02, HIGH); digitalWrite(led03, HIGH);
    digitalWrite(led04, HIGH); digitalWrite(led05, HIGH);

    //全脱力
    dxl.torqueEnable(TARGET_ID1, false);
    dxl.torqueEnable(TARGET_ID2, false);
    dxl.torqueEnable(TARGET_ID3, false);
    dxl.torqueEnable(TARGET_ID4, false);
    dxl.torqueEnable(TARGET_ID5, false);
    dxl.torqueEnable(TARGET_ID6, false);
    dxl.torqueEnable(TARGET_ID7, false);
    dxl.torqueEnable(TARGET_ID8, false);

    for (int i = 0; i < numRecords; i++) {

      //カウントダウン機能
      if (i > numRecords - int(numRecords * 4 / 5))digitalWrite(led05, LOW);
      if (i > numRecords - int(numRecords * 3 / 5))digitalWrite(led04, LOW);
      if (i > numRecords - int(numRecords * 2 / 5))digitalWrite(led03, LOW);
      if (i > numRecords - int(numRecords / 5))digitalWrite(led02, LOW);

      int ii = numRecords - i;

      if (ii > 30 && ii < 33)digitalWrite(led01, LOW);
      if (ii > 27 && ii < 30)digitalWrite(led01, HIGH);
      if (ii > 24 && ii < 27)digitalWrite(led01, LOW);
      if (ii > 19 && ii < 24)digitalWrite(led01, HIGH);
      if (ii > 16 && ii < 19)digitalWrite(led01, LOW);
      if (ii > 13 && ii < 16)digitalWrite(led01, HIGH);
      if (ii > 10 && ii < 13)digitalWrite(led01, LOW);

      if (ii < 10) {
        digitalWrite(led01, HIGH); digitalWrite(led02, HIGH); digitalWrite(led03, HIGH);
        digitalWrite(led04, HIGH); digitalWrite(led05, HIGH);
      }
      if (ii < 2) {
        digitalWrite(led01, LOW); digitalWrite(led02, LOW); digitalWrite(led03, LOW);
        digitalWrite(led04, LOW); digitalWrite(led05, LOW);
      }

      int de = 6; delay(de);
      targetPos01 = dxl.presentPosition(TARGET_ID1); delay(de);
      targetPos02 = dxl.presentPosition(TARGET_ID2); delay(de);
      targetPos03 = dxl.presentPosition(TARGET_ID3); delay(de);
      targetPos04 = dxl.presentPosition(TARGET_ID4); delay(de);
      targetPos05 = dxl.presentPosition(TARGET_ID5); delay(de);
      targetPos06 = dxl.presentPosition(TARGET_ID6); delay(de);
      targetPos07 = dxl.presentPosition(TARGET_ID7); delay(de);
      targetPos08 = dxl.presentPosition(TARGET_ID8);

      //Serial.print("timer = ");  Serial.println(t);

      Serial1.write(0);

      Serial.print("レコード中 = ");
      Serial.print(i); Serial.print(" : ");
      Serial.print(targetPos01); Serial.print(", ");
      Serial.print(targetPos02); Serial.print(", ");
      Serial.print(targetPos03); Serial.print(", ");
      Serial.print(targetPos04); Serial.print(", ");
      Serial.print(targetPos05); Serial.print(", ");
      Serial.print(targetPos06); Serial.print(", ");
      Serial.print(targetPos07); Serial.print(", ");
      Serial.println(targetPos08);

      //レコード書き出し
      file.print(targetPos01);
      file.print(",");
      file.print(targetPos02);
      file.print(",");
      file.print(targetPos03);
      file.print(",");
      file.print(targetPos04);
      file.print(",");
      file.print(targetPos05);
      file.print(",");
      file.print(targetPos06);
      file.print(",");
      file.print(targetPos07);
      file.print(",");
      file.print(targetPos08);
      file.print(",");
      file.println(0);

    }
    file.close();
  }
}

void reader(){

  if (mode > 10){

    // ファイルの読み取り
    Serial.print("mode = " + mode);

    if (mode == 11) {
      digitalWrite(led01, HIGH);
      file = SPIFFS.open("/test1.txt");
      if (!file) {
        Serial.println("ファイルの読み取りに失敗しました");
        return;
      }
    }
    if (mode == 12) {
      digitalWrite(led02, HIGH);
      file = SPIFFS.open("/test2.txt");
      if (!file) {
        Serial.println("ファイルの読み取りに失敗しました");
        return;
      }
    }
    if (mode == 13) {
      digitalWrite(led03, HIGH);
      file = SPIFFS.open("/test3.txt");
      if (!file) {
        Serial.println("ファイルの読み取りに失敗しました");
        return;
      }
    }
    if (mode == 14) {
      digitalWrite(led04, HIGH);
      file = SPIFFS.open("/test4.txt");
      if (!file) {
        Serial.println("ファイルの読み取りに失敗しました");
        return;
      }
    }
    if (mode == 15) {
      digitalWrite(led05, HIGH);
      file = SPIFFS.open("/test5.txt");
      if (!file) {
        Serial.println("ファイルの読み取りに失敗しました");
        return;
      }
    }

    // ファイルの内容を格納する配列
    int index = 0;
    while (file.available()) {
      int pos = 0;
      while (true) {
        char c = file.read();
        if (c == ',' || c == '\n' || c == -1) {
          buffer[pos] = '\0'; // 終端文字を追加
          values[index++] = atoi(buffer); // char配列を整数に変換して配列に格納
          pos = 0; // バッファの位置をリセット

          if (c == '\n' || c == -1) {
            break; // 行の終わりまたはファイルの終わり
          }
        } else {
          buffer[pos++] = c; // バッファに文字を追加
        }
      }
    }
    file.close();

    mode = 255;
    Serial.print(z);
    if (z == 0)mode = 256;
    if (mode == 255 )slow();
    if (mode == 256) {

      dxl.torqueEnable(TARGET_ID1, true);
      dxl.torqueEnable(TARGET_ID2, true);
      dxl.torqueEnable(TARGET_ID3, true);
      dxl.torqueEnable(TARGET_ID4, true);
      dxl.torqueEnable(TARGET_ID5, true);
      dxl.torqueEnable(TARGET_ID6, true);
      dxl.torqueEnable(TARGET_ID7, true);
      dxl.torqueEnable(TARGET_ID8, true);

      // シリアルモニタにデータを表示&モータ実行
      for (int i = 0; i < numRecords; i++) {

        int ss1 = values[i * number];
        int ss2 = values[i * number + 1];
        int ss3 = values[i * number + 2];
        int ss4 = values[i * number + 3];
        int ss5 = values[i * number + 4];
        int ss6 = values[i * number + 5];
        int ss7 = values[i * number + 6];
        int ss8 = values[i * number + 7];

        // delay(5);
        // Serial.print(z);
        Serial.print("フレーム ");
        Serial.print(i + 1);
        Serial.println(": ");
        // Serial.print("目標 = ");
        // Serial.print(ss1); Serial.print(", ");
        // Serial.print(ss2); Serial.print(", ");
        // Serial.print(ss3); Serial.print(", ");
        // Serial.print(ss4); Serial.print(", ");
        // Serial.print(ss5); Serial.print(", ");
        // Serial.print(ss6); Serial.print(", ");
        // Serial.print(ss7); Serial.print(", ");
        // Serial.print(ss8); Serial.print(" :");
        Serial.println(mode);

        //実測値計測
        int de = 2; delay(de);
        s1 = dxl.presentPosition(TARGET_ID1); delay(de);
        s2 = dxl.presentPosition(TARGET_ID2); delay(de);
        s3 = dxl.presentPosition(TARGET_ID3); delay(de);
        s4 = dxl.presentPosition(TARGET_ID4); delay(de);
        s5 = dxl.presentPosition(TARGET_ID5); delay(de);
        s6 = dxl.presentPosition(TARGET_ID6); delay(de);
        s7 = dxl.presentPosition(TARGET_ID7); delay(de);
        s8 = dxl.presentPosition(TARGET_ID8);

        // Serial.print("実測 = ");
        // Serial.print(s1); Serial.print(", ");
        // Serial.print(s2); Serial.print(", ");
        // Serial.print(s3); Serial.print(", ");
        // Serial.print(s4); Serial.print(", ");
        // Serial.print(s5); Serial.print(", ");
        // Serial.print(s6); Serial.print(", ");
        // Serial.print(s7); Serial.print(", ");
        // Serial.println(s8);

        Serial.print("差分 = ");
        Serial.print(ss1 - s1); Serial.print(", ");
        Serial.print(ss2 - s2); Serial.print(", ");
        Serial.print(ss3 - s3); Serial.print(", ");
        Serial.print(ss4 - s4); Serial.print(", ");
        Serial.print(ss5 - s5); Serial.print(", ");
        Serial.print(ss6 - s6); Serial.print(", ");
        Serial.print(ss7 - s7); Serial.print(", ");
        Serial.println(ss8 - s8);

        
        
        sw00State = digitalRead(sw00);
        if (abs(ss1 - s1) > s1diference || abs(ss2 - s2) > s2diference || abs(ss4 - s4) > s4diference || sw00 == LOW) {
          digitalWrite(led01, HIGH); digitalWrite(led02, LOW); digitalWrite(led03, LOW);
          digitalWrite(led04, LOW); digitalWrite(led05, HIGH);
          delay(200);
          digitalWrite(led01, LOW); digitalWrite(led05, LOW);
          mode = 0;
          break;
        }

        range(s1, ran1);
        range(s2, ran2);
        range(s4, ran4);

        //モータへ送信
        dxl.goalPosition(TARGET_ID1, ss1);
        dxl.goalPosition(TARGET_ID2, ss2);
        dxl.goalPosition(TARGET_ID3, ss3);
        dxl.goalPosition(TARGET_ID4, ss4);
        dxl.goalPosition(TARGET_ID5, ss5);
        dxl.goalPosition(TARGET_ID6, ss6);
        dxl.goalPosition(TARGET_ID7, ss7);
        dxl.goalPosition(TARGET_ID8, ss8 + 15);

      }

      //dxl.torqueEnable(TARGET_ID1, false);
      //dxl.torqueEnable(TARGET_ID2, false);
      dxl.torqueEnable(TARGET_ID3, false);
      //dxl.torqueEnable(TARGET_ID4, false);
      dxl.torqueEnable(TARGET_ID5, false);
      dxl.torqueEnable(TARGET_ID6, false);
      dxl.torqueEnable(TARGET_ID7, false);
      dxl.torqueEnable(TARGET_ID8, false);
      mode = 0;
      zero();
      slow();
    }

    turnOffLed();

  }

}



//オーディオインタフェースモード
void audioLoop(){
  delay(10);
  O_time++;

  
  

  // Serial.print("audioMode = ");
  // Serial.println(audioMode);
  
  if (audioMode == 1){
    digitalWrite(led01, HIGH);
    digitalWrite(led02, HIGH);
    digitalWrite(led03, HIGH);
    delay(1000);
    audioMode = 2;
  }
  
  swAudioState = digitalRead(swAudio);  //場所を変えない

  if (audioMode == 2){
    digitalWrite(led01, HIGH);
    digitalWrite(led02, LOW);
    digitalWrite(led03, LOW);
    if (swAudioState == LOW){
      mode = 11;
      audioMode = -1;
      O_time = 0;
    }
  }

  if (audioMode == 3){
    digitalWrite(led01, LOW);
    digitalWrite(led02, HIGH);
    digitalWrite(led03, LOW);
      if (swAudioState == LOW){
      mode = 12;
      audioMode = -1;
      O_time = 0;
    }
  }

  if (audioMode == 4){
    digitalWrite(led01, LOW);
    digitalWrite(led02, LOW);
    digitalWrite(led03, HIGH);
    if(swAudioState == LOW){
      mode = 13;
      audioMode = -1;
      O_time = 0;
    }
  }

  if(audioMode == 5){
    audioMode = 0;
    turnOffLed();
  }

  // if (audioMode == -1){
  //   if (swAudioState == HIGH){
  //     audioMode = 0;
  //     O_time = 0;
  //   }
  // }
  
  if (audioMode > 1 && O_time > O_t){
    audioMode++;
    O_time = 0;
  }

  if (mode > 10){
    reader();
  }

}



void setup(){
  DYNAMIXEL_SERIAL.begin(115200);
  dxl.attach(Serial2, 115200);
  Serial.begin(115200);
  SerialBT.begin("CartOryArm"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  pinMode(led01, OUTPUT);
  pinMode(led02, OUTPUT);
  pinMode(led03, OUTPUT);
  pinMode(led04, OUTPUT);
  pinMode(led05, OUTPUT);
  pinMode(led06, OUTPUT);

  pinMode(sw00, INPUT_PULLUP);
  pinMode(sw01, INPUT_PULLUP);
  pinMode(sw02, INPUT_PULLUP);
  pinMode(sw03, INPUT_PULLUP);
  pinMode(sw04, INPUT_PULLUP);
  pinMode(sw05, INPUT_PULLUP);

  digitalWrite(led01, LOW);
  digitalWrite(led02, LOW);
  digitalWrite(led03, LOW);
  digitalWrite(led04, LOW);
  digitalWrite(led05, LOW);
  
  digitalWrite(led01, HIGH); delay(150);
  digitalWrite(led02, HIGH); delay(150);
  digitalWrite(led03, HIGH); delay(150);
  digitalWrite(led04, HIGH); delay(150);
  digitalWrite(led05, HIGH); delay(300);

  digitalWrite(led01, LOW);
  digitalWrite(led02, LOW);
  digitalWrite(led03, LOW);
  digitalWrite(led04, LOW);
  digitalWrite(led05, LOW);

  dxl.addModel<DxlModel::X>(TARGET_ID1);
  dxl.addModel<DxlModel::X>(TARGET_ID2);
  dxl.addModel<DxlModel::X>(TARGET_ID3);
  dxl.addModel<DxlModel::X>(TARGET_ID4);
  dxl.addModel<DxlModel::X>(TARGET_ID5);
  dxl.addModel<DxlModel::X>(TARGET_ID6);
  dxl.addModel<DxlModel::X>(TARGET_ID7);
  dxl.addModel<DxlModel::X>(TARGET_ID8);

  Serial.println(VERSION_NUMBER);

  delay(2000);

  dxl.torqueEnable(TARGET_ID1, false);
  dxl.torqueEnable(TARGET_ID2, false);
  dxl.torqueEnable(TARGET_ID3, false);
  dxl.torqueEnable(TARGET_ID4, false);
  dxl.torqueEnable(TARGET_ID5, false);
  dxl.torqueEnable(TARGET_ID6, false);
  dxl.torqueEnable(TARGET_ID7, false);
  dxl.torqueEnable(TARGET_ID8, false);

  Pgain_on();


  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFSの初期化に失敗しました");
    digitalWrite(led01, HIGH);
    digitalWrite(led02, LOW);
    digitalWrite(led03, HIGH);
    return;
  }

  Serial.println("SPIFFSの初期化に成功しました");
  digitalWrite(led01, LOW);
  digitalWrite(led02, HIGH);
  digitalWrite(led03, LOW);


  ran1 = dxl.presentPosition(TARGET_ID1); delay(5);
  ran2 = dxl.presentPosition(TARGET_ID2); delay(5);
  ran4 = dxl.presentPosition(TARGET_ID4); delay(5);

  Serial.print(ran1);
  Serial.print(",");
  Serial.print(ran2);
  Serial.print(",");
  Serial.println(ran4);

  Serial.println("setup done");

  turnOffLed();
  mode = 10;
}




void loop(){
  
  readSwitchState();

  if (audioMode > 0){
    audioLoop();
  } else {

    // Serial.print("audioMode = ");
    // Serial.println(audioMode);

    Pgain_on();
    delay(10);
    demo();
    delay(10);

    if (swAudioState == LOW){
      audioMode = 1;
    }

    if (SerialBT.available()) {
      receivedChar = SerialBT.read();
    }

    if (sw00State == LOW && mode == 0){
      mode = 10;
      turnOffLed();
      delay(500);
    } else if (sw00State == LOW && mode == 10){
      mode = 0;
      turnOnAllLed();
      delay(500);
    }



    if (sw01State == LOW && mode == 0){
      mode = 1;
      turnOnLed(led01);
    } else if (sw02State == LOW && mode == 0){
      mode = 2;
      turnOnLed(led02);
    } else if (sw03State == LOW && mode == 0){
      mode = 3;
      turnOnLed(led03);
    } else if (sw04State == LOW && mode == 0){
      mode = 4;
      turnOnLed(led04);
    } else if (sw05State == LOW && mode == 0){
      mode = 5;
      turnOnLed(led05);
    }
    Serial.print(mode);
    writer();
    turnOffLed();
    mode = 10;




    // if (mode == 0){
    //   if (sw01State == LOW){
    //     mode = 1;
    //     turnOnLed(led01);
    //   } else if (sw02State == LOW){
    //     mode = 2;
    //     turnOnLed(led02);
    //   } else if (sw03State == LOW){
    //     mode = 3;
    //     turnOnLed(led03);
    //   } else if (sw04State == LOW){
    //     mode = 4;
    //     turnOnLed(led04);
    //   } else if (sw05State == LOW){
    //     mode = 5;
    //     turnOnLed(led05);
    //   }

    //   Serial.print(mode);
    //   writer();
    //   turnOffLed();
    //   mode = 10;
      
    // }





    if ((sw01State == LOW && mode == 10) || receivedChar == 49){  //ASCII 1
      mode = 11;
    } else if ((sw02State == LOW && mode == 10) || receivedChar == 50){ //ASCII 2
      mode = 12;
    }else if ((sw03State == LOW && mode == 10) || receivedChar == 51){ //ASCII 3
      mode = 13;
    } else if ((sw04State == LOW && mode == 10) || receivedChar == 52){ //ASCII 4
      mode = 14;
    } else if ((sw05State == LOW && mode == 10) || receivedChar == 53){ //ASCII 5
      mode = 15;
    }
    turnOffLed();
    reader();
    mode = 10;
    receivedChar = 0;

    dxl.torqueEnable(TARGET_ID1, true);
    dxl.torqueEnable(TARGET_ID2, true);
    dxl.torqueEnable(TARGET_ID3, true);
    dxl.torqueEnable(TARGET_ID4, true);
    dxl.torqueEnable(TARGET_ID5, true);
    dxl.torqueEnable(TARGET_ID6, true);
    dxl.torqueEnable(TARGET_ID7, true);
    dxl.torqueEnable(TARGET_ID8, true);

  }
}