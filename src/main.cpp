#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_4C.h>
#include <SPI.h>

// --- SoftAP (Wi-Fi親機) 設定 ---
const char* AP_SSID = "M5StickC-EPaper";
const char* AP_PASS = "12345678"; // 8文字以上

// --- ピン定義 ---
#define EPD_BUSY 36  // Pin 1: BUSY (G36)
#define EPD_RST  -1  // Pin 2: RES  (3.3Vプルアップ固定)
#define EPD_DC    0  // Pin 3: D/C  (G0)
#define EPD_CS   26  // Pin 4: CS   (G26)
#define EPD_SCL  33  // Pin 5: SCL  (G33 - Grove)
#define EPD_SDA  32  // Pin 6: SDA  (G32 - Grove)

// 1.54インチ 4色パネル(4C) 用ドライバ定義
GxEPD2_4C<GxEPD2_154c_GDEM0154F51H, GxEPD2_154c_GDEM0154F51H::HEIGHT> display(
    GxEPD2_154c_GDEM0154F51H(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

WebServer server(80);

// --- WebUI HTML ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>4C E-Paper Display</title>
    <style>
        body { font-family: sans-serif; text-align: center; padding: 20px; background: #f4f4f9; }
        .card { background: white; padding: 20px; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; }
        input[type="text"] { width: 80%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 6px; }
        input[type="submit"] { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 6px; cursor: pointer; }
    </style>
</head>
<body>
    <div class="card">
        <h2>E-Paper Text Sender</h2>
        <form action="/msg" method="POST">
            <input type="text" name="text" placeholder="Enter text to display..." maxlength="30"><br>
            <input type="submit" value="Send to E-Paper">
        </form>
    </div>
</body>
</html>
)rawliteral";

// トップページアクセス時
void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

// テキスト受信時の表示更新処理
void handleMessage() {
    if (server.hasArg("text")) {
        String msg = server.arg("text");

        // Web画面へ応答レスポンスを返す
        server.send(200, "text/html", "<html><body><h2>Updating Display...</h2><a href='/'>Back</a></body></html>");

        // M5本体の液晶状態更新
        M5.Display.fillScreen(BLACK);
        M5.Display.setTextColor(YELLOW);
        M5.Display.drawString("Drawing...", 10, 10);

        // 電子ペーパーへ描画
        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

            display.setTextColor(GxEPD_BLACK);
            display.setTextSize(2);
            display.setCursor(15, 20);
            display.print("Web Message:");

            // 受信した文字を赤色で表示
            display.setTextColor(GxEPD_RED);
            display.setCursor(15, 60);
            display.print(msg.c_str());

            // 4色の装飾ライン
            display.fillRect(15, 120, 170, 10, GxEPD_BLACK);
            display.fillRect(15, 140, 170, 10, GxEPD_RED);
            display.fillRect(15, 160, 170, 10, GxEPD_YELLOW);

        } while (display.nextPage());

        M5.Display.fillScreen(BLACK);
        M5.Display.setTextColor(GREEN);
        M5.Display.drawString("Updated!", 10, 10);
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setRotation(3);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1);

    // 1. SoftAP（親機）の開始
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress IP = WiFi.softAPIP();

    // 2. M5StickC Plus 画面に接続情報を表示
    M5.Display.drawString("SSID: " + String(AP_SSID), 5, 5);
    M5.Display.drawString("PASS: " + String(AP_PASS), 5, 20);
    M5.Display.drawString("IP  : " + IP.toString(), 5, 35);

    // 3. SPI & 電子ペーパー初期化
    SPI.begin(EPD_SCL, -1, EPD_SDA, EPD_CS);
    display.init(115200, true, 2, false);
    display.setRotation(2);

    // 4. Webサーバーのルーティング開始
    server.on("/", HTTP_GET, handleRoot);
    server.on("/msg", HTTP_POST, handleMessage);
    server.begin();
}

void loop() {
    M5.update();
    // Webクライアントからのアクセスを監視
    server.handleClient();
}
