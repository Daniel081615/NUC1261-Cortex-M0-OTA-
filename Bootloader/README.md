# Nuvoton NUC1261 UART OTA Bootloader

本專案實現 NUC1261 MCU 透過 UART 進行 OTA 韌體更新，包含 MCU 端 Bootloader 及 PC 端 Python Host 工具。

## 目錄結構

- `Bootloader_IAP/`：MCU 端 Bootloader 原始碼（C 語言）
- `Bootloader_python_host/`：PC 端 OTA 工具（Python）
- `Library/`：MCU 驅動程式庫
- `Doc/`：相關說明文件

## MCU 端功能

- UART ISP/OTA 韌體更新
- 韌體驗證與 Metadata 管理
- 支援雙韌體分區與 OTA 切換

## PC 端工具

- 以 Python 撰寫，支援自動修補 bin 檔向量表
- 透過 UART 傳送韌體與 Metadata
- 支援 CRC32 驗證與分段傳輸

## 快速開始

### MCU 端

1. 使用 Keil MDK 編譯 `Bootloader_IAP` 專案
2. 將產生的 bin 檔燒錄至 NUC1261 MCU

### PC 端

1. 安裝 Python 3.7+  
   `pip install pyserial capstone`
2. 編輯 `Bootloader_python_host/Bootloader_Update_Fw_Host.py`，設定 UART port 與 bin/map 檔路徑
3. 執行  
   `python Bootloader_Update_Fw_Host.py`

## UART 封包格式

- 固定 100 bytes
- Header: 0x55，Tail: 0x0A
- 內含命令碼、Payload、Checksum

## 注意事項

- MCU 與 PC 端 UART 請設定為 115200 8N1
- 詳細流程與命令格式請參考 `Doc/` 及原始碼註解

---