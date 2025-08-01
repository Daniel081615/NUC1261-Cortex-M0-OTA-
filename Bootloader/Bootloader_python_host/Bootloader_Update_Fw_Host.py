import os
import sys
import time
import serial
import struct
from typing import Tuple
from firmware_patcher import FirmwarePatcher

"""
UART ISP Host Tool
==================
自動完成 MCU OTA 流程：
 1. 連線 (CMD_CONNECT)
 2. 第一次傳送 Metadata 取得 MCU 回傳燒錄位址 (update_offset)
 3. 依 MCU offset 補丁 .bin 檔 (向量表 + 跳轉) 並重新計算 CRC32
 4. 第二次傳送修補後 Metadata
 5. 以 CMD_UPDATE_APROM + CMD_WRITE_FW 分段傳送修補後韌體
"""

# --------------------- ISP 指令定義 ---------------------
CMD_CONNECT         = 0xAE  # 建立連線指令
CMD_SWITCH_FW       = 0xAD  # 切換韌體 Bank
CMD_UPDATE_APROM    = 0xA0  # 燒錄 APROM 首包
CMD_WRITE_FW        = 0x00  # 後續資料分包
CMD_RESEND_PACKET   = 0xFF  # MCU 要求重傳
CMD_UPDATE_METADATA = 0xA5  # 更新 Metadata

# --------------------- 全域設定 ---------------------
SERIAL_PORT = "COM3"  # 串口名稱
BAUDRATE    = 115200  # 串口鮑率
PACKET_SIZE = 100     # 封包大小
BIN_FILE    = r"C:\Users\barry\OneDrive\Desktop\Bootloader_OTA_Package\Fw_ota\Project\Objects\Fw_ota.bin"
MAP_FILE    = r"C:\Users\barry\OneDrive\Desktop\Bootloader_OTA_Package\Fw_ota\Project\Listings\Fw_ota.map"
ORIGINAL_FW_BASE = 0x00000000  # 編譯時假設的載入位址

# --------------------- CRC Utility ---------------------
class CRCUtil:
    """CRC32 計算工具類別"""
    POLY = 0x04C11DB7         # CRC32 多項式
    SEED = 0xFFFFFFFF         # 初始值

    @staticmethod
    def reverse_byte(b: int) -> int:
        """反轉 8-bit 整數"""
        return int('{:08b}'.format(b)[::-1], 2)

    @staticmethod
    def reverse_uint32(n: int) -> int:
        """反轉 32-bit 整數"""
        return int('{:032b}'.format(n)[::-1], 2)

    @staticmethod
    def calculate_crc32(data: bytes, input_rev: bool = True, output_rev: bool = True) -> int:
        """
        計算 CRC32 值：
         - 使用初始種子 0xFFFFFFFF
         - 若 input_rev 為 True，先反轉每個 byte 的位元順序
         - 按照標準逐位計算後，再與 0xFFFFFFFF XOR
         - 若 output_rev 為 True，則將結果反轉
        """
        pad_len = (4 - len(data) % 4) % 4
        if pad_len:
            data += bytes([0xFF] * pad_len)
        if input_rev:
            data = bytes(CRCUtil.reverse_byte(b) for b in data)
        crc = CRCUtil.SEED
        for byte in data:
            crc ^= (byte << 24)
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ CRCUtil.POLY) & 0xFFFFFFFF
                else:
                    crc = (crc << 1) & 0xFFFFFFFF
        result = crc ^ 0xFFFFFFFF
        if output_rev:
            result = CRCUtil.reverse_uint32(result)
        return result

# --------------------- Packet Builder ---------------------
class PacketBuilder:
    """封包產生器，負責產生 100-byte 封包格式"""
    @staticmethod
    def calc_checksum(buf: bytes) -> int:
        """計算前 98 bytes 的 checksum"""
        return sum(buf[:98]) & 0xFF

    @staticmethod
    def make_packet(centerid: int, cmd: int, packno: int, payload: bytes = b"") -> bytes:
        """
        組成 100-byte 封包
        [0]=0x55, [1]=centerid, [2]=cmd, [3]=packno, [4:98]=payload, [98]=checksum, [99]=0x0A
        """
        payload = payload[:94].ljust(94, b"\xFF")
        buf = bytearray(100)
        buf[0] = 0x55
        buf[1] = centerid & 0xFF
        buf[2] = cmd & 0xFF
        buf[3] = packno & 0xFF
        buf[4:98] = payload
        buf[98] = PacketBuilder.calc_checksum(buf)
        buf[99] = 0x0A
        print(f"[MAKE] cmd=0x{cmd:02X} no={packno} chksum=0x{buf[98]:02X}")
        return bytes(buf)

# --------------------- ISP Client ---------------------
class ISPClient:
    """ISP 串口通訊類別，負責與 MCU 進行封包收發"""
    def __init__(self, port: str, baudrate: int, packet_size: int = 100, timeout: float = 0.05):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
        except Exception as e:
            print(f"[ERROR] 開啟串口失敗: {e}")
            sys.exit(1)
        self.packet_size = packet_size

    def txrx(self, packet: bytes, timeout: float = 3.0) -> bytes:
        """傳送封包並等待回應"""
        self.ser.write(packet)
        print(f"[SEND] {packet.hex()}")
        resp = b""
        start = time.time()
        while len(resp) < self.packet_size and time.time() - start < timeout:
            part = self.ser.read(self.packet_size - len(resp))
            if part:
                resp += part
        #print(f"[RECV] {resp.hex()} (len={len(resp)})")
        return resp

    def close(self):
        """關閉串口"""
        self.ser.close()

# --------------------- FirmwareUploader ---------------------
class FirmwareUploader:
    """韌體上傳流程控制類別"""
    def __init__(self, isp: ISPClient, centerid: int = 1):
        self.isp = isp
        self.centerid = centerid

    def connect(self):
        """與 MCU 建立連線"""
        pkt = PacketBuilder.make_packet(self.centerid, CMD_CONNECT, 0)
        resp = self.isp.txrx(pkt)
        if len(resp) < 3 or resp[2] != CMD_CONNECT:
            print("[ERROR] MCU 未回應 CONNECT")
            sys.exit(1)
        print("[INFO] MCU 連線成功")

    def send_update_metadata(self, packno: int, fw_version: int, fw_crc: int, fw_size: int) -> Tuple[int, int]:
        """
        傳送 Metadata 給 MCU，取得 MCU 回傳的燒錄 offset
        回傳 (status, update_addr)
        """
        payload = struct.pack('<III', fw_version, fw_crc, fw_size).ljust(94, b'\xFF')
        pkt = PacketBuilder.make_packet(self.centerid, CMD_UPDATE_METADATA, packno, payload)
        resp = self.isp.txrx(pkt)
        if len(resp) < self.isp.packet_size:
            print("[ERROR] Metadata 回應長度不足")
            return -1, 0
        update_addr = struct.unpack('<I', resp[8:12])[0]  # MCU 定義：offset 8~11
        status = resp[8:12][0]  # 假設狀態碼在第 8 byte
        print(f"[INFO] MCU offset=0x{update_addr:08X} status=0x{status:02X}")
        return status, update_addr

    def send_firmware(self, packno_start: int, fw_data: bytes):
        """
        分段傳送韌體資料給 MCU
        """
        total_len = len(fw_data)
        offset = 0
        packno = packno_start

        # 第一包 CMD_UPDATE_APROM
        first_chunk = fw_data[:92]
        pkt = PacketBuilder.make_packet(self.centerid, CMD_UPDATE_APROM, packno, first_chunk)
        self.isp.txrx(pkt)
        offset += len(first_chunk)
        packno += 1

        # 後續包 CMD_WRITE_FW
        while offset < total_len:
            chunk = fw_data[offset:offset+92]
            pkt = PacketBuilder.make_packet(self.centerid, CMD_WRITE_FW, packno, chunk)
            resp = self.isp.txrx(pkt)

            # 若 MCU 要求重傳，則不前進 offset
            if resp and resp[2] == CMD_RESEND_PACKET:
                print(f"[WARN] MCU 要求重傳 packno={packno}")
                continue  # 不前進 offset，重傳本包

            offset += len(chunk)
            packno += 1

# --------------------- main ---------------------
# --------------------- main ---------------------
if __name__ == "__main__":
    # 建立 uploader 實例
    uploader = FirmwareUploader(ISPClient(SERIAL_PORT, BAUDRATE))
    uploader.connect()

    # 第一次傳送 Metadata，取得 MCU offset
    status, update_offset = uploader.send_update_metadata(
        packno=1, fw_version=0x01020304, fw_crc=0, fw_size=0)

    if update_offset == 0:
        print("[ERROR] MCU 回傳 offset 無效")
        sys.exit(1)

    # 修補 bin 檔（加入 map 檔路徑）
    patched_bin_path, patched_data = FirmwarePatcher.adjust_vector_table(
        BIN_FILE, MAP_FILE, ORIGINAL_FW_BASE, update_offset)

    # 重新計算 CRC
    patched_size = len(patched_data)
    fw_crc32 = CRCUtil.calculate_crc32(patched_data[:patched_size])
    print(f"[INFO] 修補後韌體 CRC32: 0x{fw_crc32:08X}")
    patched_bin = patched_data[:patched_size]

    # 計算修補後韌體的 CRC32 與 Size
    fw_crc = CRCUtil.calculate_crc32(patched_data)
    fw_size = len(patched_data)

    # 第二次傳送 Metadata（帶正確 CRC 與 Size）
    uploader.send_update_metadata(
        packno=2, fw_version=0x01020304, fw_crc=fw_crc, fw_size=fw_size)

    # 傳送修補後韌體
    uploader.send_firmware(packno_start=3, fw_data=patched_data)

    uploader.isp.close()
    print("[DONE] 韌體更新完成")
# --------------------- End of main ---------------------