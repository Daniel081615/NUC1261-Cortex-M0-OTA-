# Nuvoton NUC1261 UART ISP 韌體更新說明

## 功能簡介
本模組實現 NUC1261 系列 MCU 透過 UART 介面進行 ISP（In-System Programming）韌體更新。支援 OTA、韌體驗證、Metadata 管理等功能，適合產品現場升級與維護。

## 主要檔案
- `uart_transfer.c/h`：UART 傳輸協定與中斷處理
- `isp_user.c/h`：ISP 指令解析、韌體燒錄與驗證流程
- `crc_user.c/h`：CRC32 計算（韌體與資料驗證）
- `main.c`：主程式，負責初始化與主迴圈
- `targetdev.c/h`：目標裝置定義與設定

## UART ISP 流程說明
1. **UART 初始化**  
   於 `main.c` 及 `uart_transfer.c` 內呼叫 `UART_Init()`，設定 UART0 為 115200 8N1，並啟用中斷。

2. **資料接收與封包處理**  
   - UART RX 中斷將資料存入 `uart_rcvbuf`。
   - 當接收滿 100 bytes（`MAX_PKT_SIZE`）時，觸發 `bUartDataReady`。
   - 主迴圈偵測 `bUartDataReady`，呼叫 `ParseCmd()` 解析封包並執行對應命令。

3. **ISP 指令集**  
   主要命令定義於 `isp_user.h`，如下表所示：

   | 指令名稱             | 代碼(hex) | 說明                     |
   |----------------------|-----------|--------------------------|
   | CMD_UPDATE_APROM     | 0xA0      | 更新 APROM 韌體          |
   | CMD_UPDATE_CONFIG    | 0xA1      | 更新 Config              |
   | CMD_READ_CONFIG      | 0xA2      | 讀取 Config              |
   | CMD_ERASE_ALL        | 0xA3      | 全晶片清除               |
   | CMD_SYNC_PACKNO      | 0xA4      | 同步封包編號             |
   | CMD_UPDATE_METADATA  | 0xA5      | 更新 Metadata            |
   | CMD_GET_FWVER        | 0xA6      | 取得韌體版本             |
   | CMD_SEL_FW           | 0xA7      | 切換韌體                 |
   | CMD_GET_META         | 0xA8      | 取得 Metadata            |
   | CMD_GET_CRC32        | 0xA9      | 取得指定區段 CRC32       |
   | CMD_RUN_APROM        | 0xAB      | 跳轉執行 APROM           |
   | CMD_RUN_LDROM        | 0xAC      | 跳轉執行 LDROM           |
   | CMD_RESET            | 0xAD      | 軟體重啟                 |
   | CMD_CONNECT          | 0xAE      | 連線握手                 |
   | CMD_DISCONNECT       | 0xAF      | 斷線                     |
   | CMD_GET_DEVICEID     | 0xB1      | 取得裝置 ID              |
   | CMD_GET_FLASHMODE    | 0xB2      | 取得 Flash 模式          |
   | CMD_RESEND_PACKET    | 0xFF      | 請求重送上一個封包       |

   > 實際支援指令請依 `isp_user.h` 及原始碼為準，可依需求擴充。

4. **回應封包**  
   - 執行命令後，回應資料存於 `response_buff`。
   - 透過 `PutString()` 以 UART 傳回主機。

## UART 封包格式
- **長度**：100 bytes
- **Header**：0x55
- **Tail**：0x0A
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

## 使用方式
1. 使用 Keil MDK 開啟專案並編譯。
2. 將產生的韌體（如 `NUC1261_ISP_UART0.bin`）燒錄至 MCU。
3. 透過 UART0（115200 8N1）連接 PC，使用對應 ISP 工具傳送封包進行韌體更新。

## 注意事項
- 請確認 MCU 型號與 UART 腳位設定正確。
- 若需自訂命令或流程，請參考 `isp_user.c` 及 `uart_transfer.c` 進行擴充。
- OTA/ISP 測試建議搭配官方或自製 PC 工具。

---