import serial
import struct
import time

SERIAL_PORT = "COM3"
BAUDRATE = 115200
CENTER_ID = 0x01

# 指令定義
CMD_OTA_UPDATE    = 0xA7
CMD_TO_BOOTLOADER = 0xAE
CMD_SWITCH_FW     = 0xAD
CMD_REPORT_STATUS = 0xAF

FWSTATUS_STRUCT = "<3I"
FWMETA_STRUCT   = "<8I"

# OTA flag 定義（FWStatus）
OTA_UPDATE_FLAG  = 0xDDCCBBAA
SWITCH_FW_FLAG   = 0xA5A5BEEF
OTA_FAILED_FLAG  = 0xDEADDEAD

# FW flag 定義（FWMetadata.flags，bit flag）
FW_FLAG_INVALID = (1 << 0)
FW_FLAG_VALID   = (1 << 1)
FW_FLAG_PENDING = (1 << 2)
FW_FLAG_ACTIVE  = (1 << 3)

def build_packet(center_id: int, cmd: int, payload: bytes = b"") -> bytes:
    buf = bytearray(100)
    buf[0] = 0x55
    buf[1] = center_id & 0xFF
    buf[2] = cmd & 0xFF
    buf[3] = 0x00
    payload = payload.ljust(94, b'\xFF')
    buf[4:98] = payload
    buf[98] = 0
    for i in range(98):
        buf[98] ^= buf[i]
    buf[99] = 0x0A
    return bytes(buf)

def parse_fwstatus(data: bytes) -> dict:
    vals = struct.unpack(FWSTATUS_STRUCT, data)
    return {
        "FW_Addr":      vals[0],
        "FW_meta_Addr": vals[1],
        "status":       vals[2]
    }

def parse_fwmetadata(data: bytes) -> dict:
    vals = struct.unpack(FWMETA_STRUCT, data)
    return {
        "flags":         vals[0],
        "fw_crc32":      vals[1],
        "fw_version":    vals[2],
        "fw_start_addr": vals[3],
        "fw_size":       vals[4],
        "trial_counter": vals[5],
        "reserved":      vals[6],
        "meta_crc":      vals[7]
    }

def interpret_ota_flag(status):
    if status == OTA_UPDATE_FLAG:
        return "OTA Update"
    elif status == SWITCH_FW_FLAG:
        return "Switch Firmware"
    elif status == OTA_FAILED_FLAG:
        return "OTA Failed"
    else:
        return "Unknown"

def interpret_fw_flags(flags):
    desc = []
    if flags & FW_FLAG_INVALID:
        desc.append("INVALID")
    if flags & FW_FLAG_VALID:
        desc.append("VALID")
    if flags & FW_FLAG_PENDING:
        desc.append("PENDING")
    if flags & FW_FLAG_ACTIVE:
        desc.append("ACTIVE")
    if not desc:
        desc.append("None")
    return "|".join(desc)

def print_fwstatus(status: dict):
    print(f"FW_Addr      : 0x{status['FW_Addr']:08X}")
    print(f"FW_meta_Addr : 0x{status['FW_meta_Addr']:08X}")
    print(f"status       : 0x{status['status']:08X} ({interpret_ota_flag(status['status'])})")

def print_fwmetadata(meta: dict, idx=1):
    flag_desc = interpret_fw_flags(meta['flags'])
    print(f"FWMetadata{idx}:")
    print(f"  flags         : 0x{meta['flags']:08X} ({flag_desc})")
    print(f"  fw_crc32      : 0x{meta['fw_crc32']:08X}")
    print(f"  fw_version    : 0x{meta['fw_version']:08X}")
    print(f"  fw_start_addr : 0x{meta['fw_start_addr']:08X}")
    print(f"  fw_size       : {meta['fw_size']:08X} bytes")
    print(f"  trial_counter : {meta['trial_counter']}")
    print(f"  reserved      : 0x{meta['reserved']:08X}")
    print(f"  meta_crc      : 0x{meta['meta_crc']:08X}")

def send_cmd(ser, cmd, retry=1, timeout=0.2):  # 原本 timeout=3.0，改為 0.2
    pkt = build_packet(CENTER_ID, cmd)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    for attempt in range(retry):
        try:
            ser.write(pkt)
            start = time.time()
            resp = b''
            while len(resp) < 100 and (time.time() - start) < timeout:
                resp += ser.read(100 - len(resp))
            if len(resp) != 100:
                print(f"回應長度錯誤 (收到 {len(resp)} bytes)")
                continue
            if resp[0] != 0x55 or resp[99] != 0x0A:
                print("回應格式錯誤")
                continue
            checksum = 0
            for i in range(98):
                checksum ^= resp[i]
            if resp[98] != checksum:
                print("回應 checksum 錯誤")
                continue
            return resp
        except serial.SerialException as e:
            print(f"Serial 錯誤: {e}")
            time.sleep(0.2)
    print("多次重試後仍失敗")
    return None

def send_cmd_until_response(ser, cmd, timeout=0.5):
    pkt = build_packet(CENTER_ID, cmd)
    ser.reset_input_buffer()
    ser.write(pkt)
    resp = b''
    t0 = time.time()
    while len(resp) < 100 and (time.time() - t0) < timeout:
        resp += ser.read(100 - len(resp))
    if len(resp) == 100:
        if resp[0] == 0x55 and resp[99] == 0x0A:
            checksum = 0
            for i in range(98):
                checksum ^= resp[i]
            if resp[98] == checksum:
                return resp
            else:
                print("回應 checksum 錯誤")
        else:
            print("回應格式錯誤")
    else:
        print(f"回應長度錯誤 (收到 {len(resp)} bytes)")
    return None

def menu():
    print("\n==== MCU UART 控制選單 ====")
    print("1. 查詢狀態 (CMD_REPORT_STATUS)")
    print("2. 進入 OTA 模式 (CMD_OTA_UPDATE)")
    print("3. 進入 Bootloader (CMD_TO_BOOTLOADER)")
    print("4. 切換 Firmware (CMD_SWITCH_FW)")
    print("0. 離開")
    return input("請輸入選項: ")

def main():
    try:
        with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=0.2) as ser:
            while True:
                choice = menu()
                if choice == "1":
                    resp = send_cmd_until_response(ser, CMD_REPORT_STATUS)
                    if resp:
                        fwstatus = parse_fwstatus(resp[4:16])
                        print_fwstatus(fwstatus)
                        meta1 = parse_fwmetadata(resp[20:52])
                        print_fwmetadata(meta1, 1)
                        meta2 = parse_fwmetadata(resp[52:84])
                        print_fwmetadata(meta2, 2)
                elif choice == "2":
                    resp = send_cmd_until_response(ser, CMD_OTA_UPDATE)
                    if resp:
                        fwstatus = parse_fwstatus(resp[4:16])
                        print_fwstatus(fwstatus)
                elif choice == "3":
                    resp = send_cmd_until_response(ser, CMD_TO_BOOTLOADER)
                    if resp:
                        fwstatus = parse_fwstatus(resp[4:16])
                        print_fwstatus(fwstatus)
                        meta = parse_fwmetadata(resp[20:52])
                        print_fwmetadata(meta, 1)
                elif choice == "4":
                    resp = send_cmd_until_response(ser, CMD_SWITCH_FW)
                    if resp:
                        fwstatus = parse_fwstatus(resp[4:16])
                        print_fwstatus(fwstatus)
                        meta1 = parse_fwmetadata(resp[20:52])
                        print_fwmetadata(meta1, 1)
                        meta2 = parse_fwmetadata(resp[52:84])
                        print_fwmetadata(meta2, 2)
                elif choice == "0":
                    print("Bye!")
                    break
                else:
                    print("無效選項，請重新輸入。")
    except serial.SerialException as e:
        print(f"無法開啟序列埠: {e}")

if __name__ == "__main__":
    main()