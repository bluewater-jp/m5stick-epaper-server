# m5stick-epd-frame

M5StickC Plus と 1.5インチ電子ペーパー（E-Paper）を組み合わせ、スマホやPCからWeb経由で画像をアップロード・表示させるプロトタイププロジェクトです。

## 概要

* **制御マイコン**: M5StickC Plus (ESP32)
* **ディスプレイ**: 1.5インチ E-Paper モジュール (SPI接続)
* **開発環境**: VS Code + PlatformIO (Arduino framework)
* **主な機能**: 
  * Wi-Fi Webサーバー機能（画像のアップロード受付）
  * 画像データの2値化（ディザリング）処理と電子ペーパーへの描画
  * M5StickC Plus 内蔵液晶へのステータス表示（IPアドレス等）

---

## ハードウェア構成 & 配線

M5StickC Plus と電子ペーパー（1.5インチ SPIモジュール）を以下のように接続します。

|電子ペーパーPin No.| 電子ペーパー表記 | M5StickC Plus 側 | 接続箇所 | 役割・備考 |
| ---! | :--- | :--- | :--- | :--- |
| 1| **BUSY** | **G36** | 上部ピンヘッダー | ビジー確認 (入力専用ピン活用) |
| 2| **RES** | **3.3V** | 上部 3.3V(プルアップ) | リセット (GPIO節約のため固定*) |
| 3| **D/C** | **G0** | 上部ピンヘッダー | Data / Command 切替 |
| 4| **CS** | **G26** | 上部ピンヘッダー | SPI Chip Select |
| 5| **SCL** | **G33** | 底面 Grove (HY2.0) | SPIクロック (SCK) |
| 6| **SDA** | **G32** | 底面 Grove (HY2.0) | SPIデータ (MOSI) |
| 7| **GND** | **GND** | 底面 Grove (HY2.0) | グランド |
| 8| **VCC** | **Vout** | 底面 Grove (HY2.0) | 電源 (5V) |

> **配線のポイント**:
> * **G36ピン (入力専用)** を電子ペーパーの `BUSY` 信号に割り当てています。
> * **RES (Reset) ピン** はGPIOが足りないため 3.3V にプルアップ接続（固定）します。プログラム側ではソフトウェアリセットを使用します。
> * 底面Groveからの接続には「Grove ⇄ ジャンパメス変換ケーブル」を使用します。

---

## 開発環境のセットアップ

1. **VS Code** と **PlatformIO IDE** エクステンションをインストールします。
2. このリポジトリをクローンします。
   ```bash
   git clone [https://github.com/your-username/m5stick-epd-frame.git](https://github.com/your-username/m5stick-epd-frame.git)
   cd m5stick-epd-frame
