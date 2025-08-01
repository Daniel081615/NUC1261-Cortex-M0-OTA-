import re
from typing import Dict, List, Tuple

def parse_map_file(map_file_path: str) -> Tuple[Dict[str, dict], list]:
    """
    解析 ARMCC/Keil map 檔，取得每個 section 的起始位址、大小與型態。
    回傳 sections dict 及 symbols list（symbols 可視需求擴充）。
    """
    sections = {}
    symbols = []
    section_line_re = re.compile(
        r'^\s*(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f\-]+)\s+(0x[0-9A-Fa-f]+)\s+(\w+)\s+\w+\s+\d+\s+\S+\s+([.\w$]+)'
    )
    with open(map_file_path, 'r') as f:
        for line in f:
            m = section_line_re.match(line)
            if m:
                addr = int(m.group(1), 16)
                size = int(m.group(3), 16)
                typ = m.group(4)
                name = m.group(5)
                sections[name] = {
                    'start_address': addr,
                    'size': size,
                    'type': typ
                }
    return sections, symbols

def get_executable_ranges(sections: dict) -> List[Tuple[int, int]]:
    """
    取得所有可執行區段的位址範圍（type=Code）。
    """
    return [
        (info['start_address'], info['start_address'] + info['size'])
        for info in sections.values()
        if info.get('type') == 'Code'
    ]

def get_data_ranges(sections: dict) -> List[Tuple[int, int]]:
    """
    取得所有資料區段的位址範圍（type=Data 或 Zero）。
    """
    return [
        (info['start_address'], info['start_address'] + info['size'])
        for info in sections.values()
        if info.get('type') in ('Data', 'Zero')
    ]
def is_bx_lr_valid(return_addr: int, exec_ranges: List[Tuple[int, int]]) -> bool:
    """
    判斷 BX LR 返回位址是否落在有效 code 區段。
    """
    return any(start <= return_addr < end for start, end in exec_ranges)
