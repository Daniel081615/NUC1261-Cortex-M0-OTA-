# Fw_App_Python_Host

## 概述
此專案提供了一個 Python 應用程式，用於通過 UART 與微控制器進行通信。它支持 OTA 更新、固件切換和進入 Bootloader 模式的功能。

## 需求
- Python 3.x
- pyserial 庫

## 安裝
1. 確保已安裝 Python 3.x。
2. 安裝所需的庫：
   ```
   pip install pyserial
   ```

## 使用方法
1. 連接微控制器到指定的串口（例如 COM3）。
2. 執行應用程式：
   ```
   python FW_Host_Uart_Cmd.py
   ```
3. 根據提示選擇操作：
   - 查詢狀態
   - 重啟進入 OTA 模式
   - 進入 Bootloader
   - 切換 Firmware
   - 離開

## 功能
- **查詢狀態**: 獲取當前固件狀態和元數據。
- **進入 OTA 模式**: 開始 OTA 更新過程。
- **進入 Bootloader**: 切換到 Bootloader 模式。
- **切換 Firmware**: 切換到另一個固件版本。

## 參考
- [pyserial Documentation](https://pyserial.readthedocs.io/en/latest/)

## 貢獻
歡迎任何形式的貢獻！請提交問題或拉取請求。

## 授權
此專案使用 MIT 授權。