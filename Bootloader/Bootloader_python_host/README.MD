# Keil C Bootloader ISP Host Python 工具

本專案為 Keil C Bootloader ISP (In-System Programming) 的 Host 端 Python 工具，支援 MCU OTA（Over-The-Air）韌體升級流程。  
可自動修補 firmware（二進位檔）向量表、跳轉表、switch-case table 等，並透過 UART 與目標 MCU 進行通訊與燒錄。

---

## 目錄結構

- `Bootloader_Update_Fw_Host.py`：主程式，負責 OTA 流程控制與與 MCU 通訊
- [`firmware_patcher.py`](firmware_patcher.py)：韌體修補邏輯（向量表、跳轉、switch table 等）
- [`map_parser.py`](map_parser.py)：Keil/ARMCC map 檔解析工具
- `.dist/`：可放置打包後的執行檔
- 其他：說明文件（docx）

---

## 使用方式

### 1. 安裝必要套件

請先安裝 Python 3.7 以上版本，並安裝必要套件：

```sh
pip install pyserial capstone
```

### 2. 準備檔案

- MCU 端需燒錄對應的 Bootloader 韌體
- 準備好要 OTA 的 bin 檔與 map 檔（Keil 編譯產生）

### 3. 設定參數

編輯 [`Bootloader_Update_Fw_Host.py`](Bootloader_Update_Fw_Host.py) 內的下列參數：

- `SERIAL_PORT`：指定 UART COM port（如 `COM3`）
- `BIN_FILE`：待燒錄的韌體 bin 檔路徑
- `MAP_FILE`：Keil 產生的 map 檔路徑
- `ORIGINAL_FW_BASE`：原始 firmware 編譯時的載入位址（通常為 0）

### 4. 執行主程式

```sh
python Bootloader_Update_Fw_Host.py
```

---

## 流程說明

1. **與 MCU 建立連線**  
   Host 端發送 `CMD_CONNECT` 指令，MCU 回應確認。

2. **第一次傳送 Metadata**  
   傳送韌體版本、CRC、大小等資訊，MCU 回傳建議的燒錄 offset。

3. **修補韌體**  
   依 MCU 回傳 offset，利用 [`FirmwarePatcher.adjust_vector_table`](firmware_patcher.py) 修補 bin 檔（向量表、跳轉、switch table 等），確保正確執行。

4. **重新計算 CRC32**  
   使用 [`CRCUtil`](Bootloader_Update_Fw_Host.py) 計算修補後韌體的 CRC32。

5. **第二次傳送 Metadata**  
   傳送正確的韌體版本、CRC、大小，MCU 驗證資訊。

6. **分段傳送韌體**  
   以 100-byte 封包格式，先送 `CMD_UPDATE_APROM`，後續用 `CMD_WRITE_FW`，MCU 若要求重傳則重送該包。

7. **完成**  
   MCU 燒錄完成後，Host 端關閉連線。

---

## 主要程式說明

- [`FirmwarePatcher`](firmware_patcher.py)：修補韌體，調整所有與位址相關的表格與跳轉
- [`parse_map_file`](map_parser.py)：解析 Keil map 檔，取得各 section 的位址與型態
- `CRCUtil`：計算 CRC32
- `PacketBuilder`：產生 100-byte 封包
- `ISPClient`：UART 封包收發
- `FirmwareUploader`：OTA 流程控制

---

## 注意事項

- MCU 端需配合正確的 ISP/OTA Bootloader 韌體
- 若 MCU 回傳 offset 為 0，請檢查 MCU 韌體支援度
- 若遇到串口開啟失敗，請確認 COM port 是否正確、已安裝驅動

---