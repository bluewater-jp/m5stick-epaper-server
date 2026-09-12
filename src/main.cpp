#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_4C.h>
#include <SPI.h>

// --- SoftAP 設定 ---
const char* AP_SSID = "M5StickC-EPaper";
const char* AP_PASS = "12345678";

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

// 受信用画像バッファ (200x200px, 1ピクセル=1バイト: 0=WHITE, 1=BLACK, 2=RED, 3=YELLOW)
uint8_t imageBuffer[200 * 200];

// --- WebUI (HTML + JavaScript) ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>4C Photo Uploader</title>
    <style>
        body { font-family: sans-serif; text-align: center; padding: 15px; background: #f0f2f5; }
        .card { background: white; padding: 20px; border-radius: 12px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 350px; margin: 0 auto; }
        input[type="file"] { margin: 15px 0; }
        canvas { border: 1px solid #ccc; margin-top: 10px; width: 200px; height: 200px; image-rendering: pixelated; }
        button { background: #007bff; color: white; border: none; padding: 12px 24px; border-radius: 6px; font-size: 16px; cursor: pointer; margin-top: 15px; }
        button:disabled { background: #ccc; }
    </style>
</head>
<body>
    <div class="card">
        <h2>Photo to E-Paper</h2>
        <input type="file" id="fileInput" accept="image/*"><br>
        <canvas id="previewCanvas" width="200" height="200"></canvas><br>
        <button id="sendBtn" onclick="uploadImage()" disabled>Send to E-Paper</button>
        <p id="status"></p>
    </div>

    <script>
        const fileInput = document.getElementById('fileInput');
        const canvas = document.getElementById('previewCanvas');
        const ctx = canvas.getContext('2d');
        const sendBtn = document.getElementById('sendBtn');
        const status = document.getElementById('status');
        
        let convertedData = new Uint8Array(200 * 200);

        // パレット定義 [R, G, B] -> EPD Color Index (0:WHITE, 1:BLACK, 2:RED, 3:YELLOW)
        const PALETTE = [
            { c: 0, r: 255, g: 255, b: 255 }, // WHITE
            { c: 1, r: 0,   g: 0,   b: 0   }, // BLACK
            { c: 2, r: 255, g: 0,   b: 0   }, // RED
            { c: 3, r: 255, g: 255, b: 0   }  // YELLOW
        ];

        fileInput.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (!file) return;

            const img = new Image();
            img.onload = () => {
                // 200x200 にリサイズ描画
                ctx.drawImage(img, 0, 0, 200, 200);
                const imgData = ctx.getImageData(0, 0, 200, 200);
                const data = imgData.data;

                // 4色への減色処理 (最近傍色マッチング)
                for (let i = 0; i < data.length; i += 4) {
                    const r = data[i], g = data[i+1], b = data[i+2];
                    
                    let minDistance = Infinity;
                    let bestMatch = PALETTE[0];

                    for (let p of PALETTE) {
                        const dist = Math.pow(r - p.r, 2) + Math.pow(g - p.g, 2) + Math.pow(b - p.b, 2);
                        if (dist < minDistance) {
                            minDistance = dist;
                            bestMatch = p;
                        }
                    }

                    // プレビュー表示用キャンバスを書き換え
                    data[i]     = bestMatch.r;
                    data[i+1]   = bestMatch.g;
                    data[i+2]   = bestMatch.b;

                    // M5送信用データバッファに格納
                    convertedData[i / 4] = bestMatch.c;
                }

                ctx.putImageData(imgData, 0, 0);
                sendBtn.disabled = false;
                status.innerText = "Image converted to 4-colors!";
            };
            img.src = URL.createObjectURL(file);
        });

        function uploadImage() {
            sendBtn.disabled = true;
            status.innerText = "Sending image to M5...";

            fetch('/upload_img', {
                method: 'POST',
                headers: { 'Content-Type': 'application/octet-stream' },
                body: convertedData
            })
            .then(res => res.text())
            .then(text => {
                status.innerText = "Updating E-Paper display...";
            })
            .catch(err => {
                status.innerText = "Error: " + err;
                sendBtn.disabled = false;
            });
        }
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

// バイナリ画像データの受信処理
void handleImageUpload() {
    if (server.hasArg("plain") == false) {
        // stream受信処理
        HTTPUpload& upload = server.upload();
    }
    
    // リクエストボディからバイナリを取得
    String body = server.arg("plain");
    int len = server.arg("plain").length();

    if (len >= 200 * 200) {
        memcpy(imageBuffer, server.arg("plain").c_str(), 200 * 200);
        server.send(200, "text/plain", "OK");

        // 本体液晶にステータス表示
        M5.Display.fillScreen(BLACK);
        M5.Display.setTextColor(YELLOW);
        M5.Display.drawString("Drawing Photo...", 5, 10);

        // 電子ペーパーへ描画
        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            for (int y = 0; y < 200; y++) {
                for (int x = 0; x < 200; x++) {
                    uint8_t colorIdx = imageBuffer[y * 200 + x];
                    uint16_t epdColor = GxEPD_WHITE;

                    if (colorIdx == 1) epdColor = GxEPD_BLACK;
                    else if (colorIdx == 2) epdColor = GxEPD_RED;
                    else if (colorIdx == 3) epdColor = GxEPD_YELLOW;

                    if (epdColor != GxEPD_WHITE) {
                        display.drawPixel(x, y, epdColor);
                    }
                }
            }
        } while (display.nextPage());

        M5.Display.fillScreen(BLACK);
        M5.Display.setTextColor(GREEN);
        M5.Display.drawString("Photo Done!", 5, 10);
    } else {
        server.send(400, "text/plain", "Invalid Data Size");
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setRotation(3);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1);
    // 1. SoftAP 開始
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress IP = WiFi.softAPIP();

    M5.Display.drawString("SSID: " + String(AP_SSID), 5, 5);
    M5.Display.drawString("PASS: " + String(AP_PASS), 5, 20);
    M5.Display.drawString("IP  : " + IP.toString(), 5, 35);

    // 2. SPI & 電子ペーパー初期化
    SPI.begin(EPD_SCL, -1, EPD_SDA, EPD_CS);
    display.init(115200, true, 2, false);
    display.setRotation(2);

    // 3. ルーティング設定
    server.on("/", HTTP_GET, handleRoot);
    server.on("/upload_img", HTTP_POST, handleImageUpload);
    server.begin();
}

void loop() {
    M5.update();
    server.handleClient();
}
