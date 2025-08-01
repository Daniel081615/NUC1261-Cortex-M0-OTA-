## 韌體自動修補（FW Patch）設計說明

本專案的 Python Host 工具具備「自動修補韌體」功能，能根據 MCU 回傳的燒錄 offset，動態調整 bin 檔案中的向量表、跳轉表與 switch-case table，確保韌體在不同記憶體位置下仍可正確執行。此功能主要由 `firmware_patcher.py` 及其 `FirmwarePatcher.adjust_vector_table` 方法實現。

### 設計動機

- MCU OTA 時，韌體實際燒錄位址（offset）可能與原始編譯位址不同。
- ARM Cortex-M 系列 MCU 的向量表（Vector Table）、程式跳轉（如 reset handler）、switch-case table 等，均與絕對位址有關。
- 若未修補，燒錄到新位址後 MCU 會因跳轉錯誤而無法執行。

### 修補流程

1. **取得 MCU 回傳 offset**  
   Host 端與 MCU 通訊，取得 MCU 建議的燒錄起始位址（offset）。

2. **解析 map 檔**  
   透過 `map_parser.py` 解析 Keil/ARMCC 產生的 map 檔，取得各 section、符號的原始位址與型態。

3. **修補 bin 檔**  
   - 依據 offset，將 bin 檔中的向量表（如 reset handler、NMI、HardFault 等）全部重寫為新位址。
   - 針對 switch-case table、跳轉表等，尋找所有與程式位址有關的資料並進行偏移修正。
   - 若有特殊 section（如 boot entry、interrupt table），也一併調整。

4. **重新計算 CRC32**  
   修補後的韌體需重新計算 CRC32，確保 MCU 驗證正確。

### 主要修補內容

- **向量表（Vector Table）**  
  ARM MCU 開機時會根據向量表跳轉，需將所有 handler 位址加上 offset。

- **程式跳轉表（Jump Table）/ Switch Table**  
  C 語言 switch-case 編譯後會產生跳轉表，需一併修正。

- **其他與絕對位址有關的 section**  
  例如 boot entry、interrupt table 等。

### 實作重點

- `FirmwarePatcher.adjust_vector_table(bin_path, map_path, original_base, new_offset)`  
  會自動：
  - 解析 map 檔取得所有需修補的位址
  - 讀取 bin 檔並進行位址偏移修正
  - 回傳修補後的 bin 檔內容與路徑

- 修補過程完全自動，無需手動修改 bin 檔或 linker script。

### 優點

- **高度自動化**：只需提供原始 bin 檔與 map 檔，即可自動完成所有修補。
- **安全性高**：避免因位址錯誤導致 MCU 無法啟動。
- **適用多種 OTA 場景**：支援雙韌體分區、動態載入等需求。

---

> 本設計可直接引用於技術文件或專案說明，協助說明 OTA Host 端自動修補韌體的原理與流程。