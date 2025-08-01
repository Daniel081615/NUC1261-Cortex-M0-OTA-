import re
from typing import List, Tuple, Set
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.arm import ARM_REG_PC
from map_parser import parse_map_file, get_executable_ranges, get_data_ranges


class FirmwarePatcher:
    DEBUG = True

    @staticmethod
    def adjust_vector_table(
        bin_path: str, map_path: str, original_addr: int, new_addr: int, table_size: int = 192
    ) -> Tuple[str, bytes]:
        """
        主入口：修補 firmware，將所有與原始位址相關的表格、跳轉、switch table 進行偏移調整。
        """
        with open(bin_path, 'rb') as f:
            bin_data = bytearray(f.read())
        if len(bin_data) < table_size:
            raise RuntimeError("bin 長度不足")

        offset_val = new_addr - original_addr
        if FirmwarePatcher.DEBUG:
            print(f"[PATCH] 偏移量 0x{offset_val:08X}")

        # 修補向量表
        patched_entries = FirmwarePatcher._patch_vector_table(bin_data, table_size, offset_val)

        # 反組譯所有指令
        md = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
        md.detail = True
        instructions = list(md.disasm(bin_data, original_addr))

        # 修補 branch 跳轉與 LDR 常數
        patched_branches = FirmwarePatcher._patch_branch_instructions(
            instructions, bin_data, original_addr, offset_val
        )

        # 解析 map 檔取得 section 資訊
        sections, _ = parse_map_file(map_path)
        if FirmwarePatcher.DEBUG:
            print(f"[MAP] Sections: {', '.join(sections.keys())}")
        exec_ranges = get_executable_ranges(sections)
        if FirmwarePatcher.DEBUG:
            print(f"[EXEC] 可執行區段: {exec_ranges}")        
        data_ranges = get_data_ranges(sections)
        if FirmwarePatcher.DEBUG:
            print(f"[DATA] 資料區段: {data_ranges}")

        # 5. 修補 LDR 絕對地址載入（只修補指向 data 區段的 LDR）
        FirmwarePatcher._patch_absolute_loads(
            instructions, bin_data, original_addr, offset_val, data_ranges
        )

        # 6. 修補 switch-case jump table
        FirmwarePatcher._patch_jump_table(
            bin_data, original_addr, offset_val,
            exec_ranges=exec_ranges,
            patched_entries=patched_entries,
            patched_branches=patched_branches
        )

        # 7. 儲存修補後的檔案
        new_path = bin_path.replace('.bin', f'_at_{hex(new_addr)}.bin')
        with open(new_path, 'wb') as f:
            f.write(bin_data)
        if FirmwarePatcher.DEBUG:
            print(f"[SAVE] patched -> {new_path}")
        return new_path, bytes(bin_data)

    @staticmethod
    def _patch_absolute_loads(
        instructions, bin_data: bytearray, original_addr: int, offset_val: int, data_ranges: List[Tuple[int, int]]
    ) -> Set[int]:
        """
        修補 LDR Rx, =0xXXXXXX 絕對地址載入指令，若其指向 data 區段。
        只針對指向 data 區段的常數進行偏移修補。
        """
        if FirmwarePatcher.DEBUG:
            print("[CONST] 掃描絕對地址載入...")
        patched_consts = set()
        def is_in_data(addr: int) -> bool:
            # 判斷位址是否在 data 區段
            return any(start <= addr < end for start, end in data_ranges)
        for ins in instructions:
            # 檢查是否為 LDR Rx, [PC, #imm] 指令
            if (ins.mnemonic == "ldr" and len(ins.operands) >= 2 and
                ins.operands[0].type == CS_OP_REG and
                ins.operands[1].type == CS_OP_MEM and
                ins.operands[1].mem.base == ARM_REG_PC):
                lit_addr = ins.address + 4 + ins.operands[1].mem.disp  # 計算常數表位址
                # 檢查常數表位址是否在 bin 範圍內
                if 0 <= lit_addr - original_addr < len(bin_data) - 4:
                    val = int.from_bytes(bin_data[lit_addr - original_addr:lit_addr - original_addr + 4], 'little')
                    # 只修補指向 data 區段且未修補過的常數
                    if is_in_data(val) and val not in patched_consts:
                        new_val = val + offset_val  # 加上偏移
                        bin_data[lit_addr - original_addr:lit_addr - original_addr + 4] = new_val.to_bytes(4, 'little')
                        patched_consts.add(val)
                        if FirmwarePatcher.DEBUG:
                            print(f" LDR const @ 0x{lit_addr:08X}: 0x{val:08X} -> 0x{new_val:08X}")
        return patched_consts

    @staticmethod
    def _patch_vector_table(bin_data: bytearray, table_size: int, offset_val: int) -> Set[int]:
        """
        修補向量表（通常在 bin 開頭），將所有非 0/0xFFFFFFFF 的 entry 加上 offset。
        """
        if FirmwarePatcher.DEBUG:
            print("[VECTOR] 開始修補向量表...")
        patched_entries = set()
        for i in range(1, table_size // 4):
            pos = i * 4
            entry = int.from_bytes(bin_data[pos:pos + 4], 'little')
            # 跳過空 entry
            if entry in (0, 0xFFFFFFFF) or entry in patched_entries:
                continue
            new_entry = entry + offset_val
            bin_data[pos:pos + 4] = new_entry.to_bytes(4, 'little')
            patched_entries.add(entry)
            if FirmwarePatcher.DEBUG:
                print(f" 向量 @ 0x{pos:02X}: 0x{entry:08X} -> 0x{new_entry:08X}")
        return patched_entries

    @staticmethod
    def _patch_branch_instructions(
        instructions, bin_data: bytearray, original_addr: int, offset_val: int
    ) -> Set[int]:
        """
        修補所有 branch (b, bl) 及 LDR PC 相對常數的跳轉目標位址。
        只修補跳轉目標在 bin 內的 branch。
        """
        if FirmwarePatcher.DEBUG:
            print("[JUMP] 掃描跳轉指令...")
        patched_branches = set()
        for ins in instructions:
            addr = ins.address
            # 處理 B/BL 跳轉
            if ins.mnemonic in ("b", "bl") and ins.operands[0].type == CS_OP_IMM:
                tgt = ins.operands[0].imm
                if original_addr <= tgt < original_addr + len(bin_data) and tgt not in patched_branches:
                    new_tgt = tgt + offset_val
                    patched_branches.add(tgt)
                    if FirmwarePatcher.DEBUG:
                        print(f" {ins.mnemonic.upper()} 0x{addr:08X} -> 0x{new_tgt:08X}")
            # 處理 LDR PC, [PC, #imm] 取常數表
            elif (ins.mnemonic == "ldr" and len(ins.operands) >= 2 and
                  ins.operands[0].type == CS_OP_REG and
                  ins.operands[1].type == CS_OP_MEM and
                  ins.operands[1].mem.base == ARM_REG_PC):
                lit_addr = ins.address + 4 + ins.operands[1].mem.disp
                if 0 <= lit_addr - original_addr < len(bin_data) - 4 and lit_addr not in patched_branches:
                    val = int.from_bytes(bin_data[lit_addr - original_addr:lit_addr - original_addr + 4], 'little')
                    if original_addr <= val < original_addr + len(bin_data):
                        bin_data[lit_addr - original_addr:lit_addr - original_addr + 4] = (val + offset_val).to_bytes(4, 'little')
                        patched_branches.add(lit_addr)
                        if FirmwarePatcher.DEBUG:
                            print(f" LDR const @ 0x{lit_addr:08X} patched")
        return patched_branches

    @staticmethod
    def _patch_jump_table(
        data: bytearray, old_base: int, offset: int,
        exec_ranges: List[Tuple[int, int]],
        patched_entries: Set[int] = None,
        patched_branches: Set[int] = None
    ):
        """
        掃描資料區段，尋找連續的可執行位址（疑似 switch-case jump table），並修補其內容。
        只修補指向 code 區段的連續位址表。
        """
        patched_entries = patched_entries or set()
        patched_branches = patched_branches or set()
        patched_tables = set()
        patched_targets = set()
        def is_in_text(addr: int) -> bool:
            # 判斷位址是否在 code 區段
            return any(start <= addr < end for start, end in exec_ranges)
        for i in range(0, len(data) - 40, 4):
            entries = []
            for j in range(10):
                entry_offset = i + j * 4
                if entry_offset + 4 > len(data):
                    break
                addr = int.from_bytes(data[entry_offset:entry_offset + 4], 'little')
                # 只收集連續指向 code 區段的 entry
                if addr in (0, 0xFFFFFFFF) or not is_in_text(addr):
                    break
                entries.append((entry_offset, addr))
            # 至少 4 筆才視為 jump table
            if len(entries) >= 4:
                for off, addr in entries:
                    if addr in patched_targets or addr in patched_entries or addr in patched_branches:
                        continue
                    new_addr = addr + offset
                    data[off:off + 4] = new_addr.to_bytes(4, 'little')
                    patched_tables.add(off)
                    patched_targets.add(addr)
                    if FirmwarePatcher.DEBUG:
                        print(f"  Entry @ 0x{off:08X}: 0x{addr:08X} -> 0x{new_addr:08X}")