#include <Arduino.h>
#include <M5Unified.h>
#include <GxEPD2_4C.h> // ★ 4色用ヘッダーに変更
#include <SPI.h>

// --- ピン定義 ---
#define EPD_BUSY 36  // Pin 1: BUSY (G36)
#define EPD_RST  -1  // Pin 2: RES  (3.3Vプルアップ固定)
#define EPD_DC    0  // Pin 3: D/C  (G0)
#define EPD_CS   26  // Pin 4: CS   (G26)
#define EPD_SCL  33  // Pin 5: SCL  (G33 - Grove)
#define EPD_SDA  32  // Pin 6: SDA  (G32 - Grove)

// 1.54インチ 4色パネル(4C) 用クラス定義
GxEPD2_4C<GxEPD2_154c_GDEM0154F51H, GxEPD2_154c_GDEM0154F51H::HEIGHT> display(
    GxEPD2_154c_GDEM0154F51H(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);



void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setRotation(3);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Init 4C EPD...", 10, 10);

    // SPI通信の初期化
    SPI.begin(EPD_SCL, -1, EPD_SDA, EPD_CS);

    // 電子ペーパー初期化
    display.init(115200, true, 2, false);
    display.setRotation(2);

    // 描画実行
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // 外枠
        display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

        // テキスト表示
        display.setTextColor(GxEPD_BLACK);
        display.setTextSize(2);
        display.setCursor(15, 20);
        display.print("M5StickC Plus");

        display.setCursor(15, 50);
        display.print("4-Color EPD!");

        // 4色のテスト描画（黒・赤・黄）
        display.setCursor(15, 90);
        display.setTextColor(GxEPD_RED);
        display.print("RED COLOR");

        display.setCursor(15, 120);
        display.setTextColor(GxEPD_YELLOW);
        display.print("YELLOW COLOR");

        // 図形テスト
        display.fillRect(15, 150, 40, 30, GxEPD_BLACK);
        display.fillRect(65, 150, 40, 30, GxEPD_RED);
        display.fillRect(115, 150, 40, 30, GxEPD_YELLOW);

    } while (display.nextPage());

    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(GREEN);
    M5.Display.drawString("Done!", 10, 10);
}

void loop() {
    M5.update();
    delay(100);
}
