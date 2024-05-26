#ifndef TFT_HPP
#define TFT_HPP

#include <TFT_eSPI.h>      // ハードウェア固有のライブラリ
#include <SPI.h>

extern int mode; // グローバル変数としてmodeを宣言

TFT_eSPI tft = TFT_eSPI(); // カスタムライブラリの呼び出し
#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false

#define KEY_X 40 // キーの中央
#define KEY_Y 90
#define KEY_W 74 // 幅と高さ
#define KEY_H 45
#define KEY_SPACING_X 6 // XとYのギャップ
#define KEY_SPACING_Y 5
#define KEY_TEXTSIZE 0   // フォントサイズの倍率

// 数字は太字が良いので2つのフォントを使用
#define LABEL1_FONT &FreeSansOblique12pt7b // キーラベルフォント1
#define LABEL2_FONT &FreeSansBold12pt7b    // キーラベルフォント2

// 数値表示ボックスのサイズと位置
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

#define STATUS_X 120 // ここに中心
#define STATUS_Y 65

char keyLabel[15][5] = {"RUN", "MODE", "REC", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#" };
uint16_t keyColor[15] = {TFT_RED, TFT_DARKGREY, TFT_DARKGREEN,
                        TFT_BLUE, TFT_BLUE, TFT_BLUE,
                        TFT_BLUE, TFT_BLUE, TFT_BLUE,
                        TFT_BLUE, TFT_BLUE, TFT_BLUE,
                        TFT_BLUE, TFT_BLUE, TFT_BLUE
                        };

TFT_eSPI_Button key[15];

void touch_calibrate()
{
    uint16_t calData[5];
    uint8_t calDataOK = 0;

    // ファイルシステムの存在を確認
    if (!SPIFFS.begin()) {
        Serial.println("ファイルシステムをフォーマットしています");
        SPIFFS.format();
        SPIFFS.begin();
    }

    // キャリブレーションファイルが存在し、サイズが正しいか確認
    if (SPIFFS.exists(CALIBRATION_FILE)) {
        if (REPEAT_CAL)
        {
            // 再キャリブレーションする場合は削除
            SPIFFS.remove(CALIBRATION_FILE);
        }
        else
        {
            File f = SPIFFS.open(CALIBRATION_FILE, "r");
            if (f) {
                if (f.readBytes((char *)calData, 14) == 14)
                    calDataOK = 1;
                f.close();
            }
        }
    }

    if (calDataOK && !REPEAT_CAL) {
        // キャリブレーションデータが有効
        tft.setTouch(calData);
    } else {
        // データが無効なので再キャリブレーション
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.println("指示された角をタッチしてください");

        tft.setTextFont(1);
        tft.println();

        if (REPEAT_CAL) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("REPEAT_CALをfalseに設定して、再実行を停止してください！");
        }

        tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println("キャリブレーション完了！");

        // データを保存
        File f = SPIFFS.open(CALIBRATION_FILE, "w");
        if (f) {
            f.write((const unsigned char *)calData, 14);
            f.close();
        }
    }
}

//------------------------------------------------------------------------------------------

// ミニステータスバーにメッセージを表示
void status(const char *msg) {
    tft.setTextPadding(240);
    //tft.setCursor(STATUS_X, STATUS_Y);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.setTextFont(0);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(1);
    tft.drawString(msg, STATUS_X, STATUS_Y);
}
//------------------------------------------------------------------------------------------

void drawKeypad()
{
    // キーを描画
    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            uint8_t b = col + row * 3;

            if (b < 3) tft.setFreeFont(LABEL1_FONT);
            else tft.setFreeFont(LABEL2_FONT);

            if (mode > 9) {
                keyColor[4] = TFT_DARKGREEN;
                keyColor[5] = TFT_DARKGREEN;
                keyColor[6] = TFT_DARKGREEN;
                keyColor[7] = TFT_DARKGREEN;
                keyColor[8] = TFT_DARKGREEN;
                keyColor[9] = TFT_DARKGREEN;
            } else {
                keyColor[4] = TFT_WHITE;
                keyColor[5] = TFT_WHITE;
                keyColor[6] = TFT_WHITE;
                keyColor[7] = TFT_WHITE;
                keyColor[8] = TFT_WHITE;
                keyColor[9] = TFT_WHITE;
            }
            key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                                KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
                                KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                                keyLabel[b], KEY_TEXTSIZE);
            key[b].drawButton();
        }
    }
}

#endif // TFT_HPP
