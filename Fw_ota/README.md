# Nuvoton NUC1261 Fw_App OTA 雙韌體專案

本專案為 NUC1261 MCU 雙韌體 OTA 應用範例，分別提供 LED_G（綠燈）與 LED_R（紅燈）兩組獨立韌體，支援 OTA 更新、韌體自我驗證、Metadata 管理等功能。

---

## 目錄結構

- `Fw_App_OTA_Package/`：紅/綠燈韌體專案（Keil C 專案與原始碼）
- `Fw_App_Python_Host/`：紅燈韌體 Python host
- `Output/`：Keil c 編譯完成的 .bin 與 .map 檔
- `Doc/`：OTA 流程、UART 協定等說明文件
- `README.md`：本說明文件

---

## 功能說明

- **雙韌體 OTA 支援**：可於 APROM 兩區切換執行，支援 OTA 線上升級。
- **韌體自我驗證**：開機自動進行 CRC32 驗證，確保韌體完整性。
- **Metadata 管理**：每組韌體皆有獨立 Metadata，記錄版本、CRC、狀態等資訊。
- **UART OTA/ISP**：支援 UART 封包協定進行韌體與 Metadata 更新。
- **狀態標記與回報**：韌體狀態（ACTIVE、PENDING、FAIL）自動標記與回報。

---

## 快速開始

### 1. 編譯韌體

- 使用 Keil MDK 開啟 `LED_G_fw/LED_G.uvprojx` 或 `LED_R_fw/LED_R.uvprojx`。
- 編譯後於 `Objects/` 目錄取得 bin/hex 檔案。

### 2. 燒錄與測試

- 將產生的韌體 bin 檔燒錄至 NUC1261 MCU。
- MCU 開機後會自動進行自我驗證，並透過 UART 回報狀態。

### 3. OTA 更新流程

- 參考 `Doc/OTA流程說明.pdf` 及 `UART協定說明.pdf`。
- 透過 UART 傳送 OTA 封包，MCU 會自動切換分區、驗證並啟用新韌體。

---

## 主要原始碼說明

- `main.c`：主程式，負責系統初始化與主流程。
- `fw_metadata.c/h`：韌體 Metadata 結構與驗證。
- `fw_status.c/h`：韌體狀態管理與旗標寫入。
- `fw_selfcheck.c/h`：開機自我驗證、OTA 流程控制。
- `fw_ota_mark_setup.c/h`：OTA 標記與 Metadata 操作。
- `crc_util.c/h`：CRC32 計算工具。
- `uart_comm.c/h`、`uart_user.c/h`：UART 封包處理與通訊協定。

---

## UART OTA 封包格式

- **長度**：100 bytes
- **Header**：0x55，**Tail**：0x0A
- **結構**：
  | Byte  | 說明         |
  |-------|--------------|
  | 0     | Header (0x55)|
  | 1     | 保留         |
  | 2     | 命令碼       |
  | 3     | 封包編號     |
  | 4~95  | Payload      |
  | 96~97 | 保留         |
  | 98    | Checksum     |
  | 99    | Tail (0x0A)  |

- 詳細協定請參考 `Doc/UART協定說明.pdf`

---

## 注意事項

- MCU 型號：**NUC1261**
- 請勿刪除 Metadata/Status 相關原始碼，否則 OTA 流程將無法正常運作。
- 若需修改 OTA 流程或協定，請同步更新說明文件。

---

## 聯絡方式

如有問題請聯絡專案負責人或原開發者。

---

> 本專案僅供內部開發與測試，請勿外流。