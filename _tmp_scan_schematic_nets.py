from pypdf import PdfReader
import re
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\Schematic_stm32_space_balloon_2025-12-30.pdf"
reader=PdfReader(p)
need=[
"GPS_Wake","GPS_!RST","MLX_RST","SHT_RST","MS_RST","CO2_RST","MCP_RST","SEN_RST","PMS_SET","LSM_RST","LSM_INT",
"I2C1_SCL","I2C1_SDA","I2C2_SCL","I2C2_SDA","UART1_TX","UART1_RX","UART2_TX","UART2_RX","UART3_TX","UART3_RX"
]
rx=re.compile('|'.join(re.escape(x) for x in need))
for i,page in enumerate(reader.pages, start=1):
    txt=(page.extract_text() or '')
    if rx.search(txt):
        print('\n===== PAGE',i,'=====')
        lines=txt.splitlines()
        for idx,line in enumerate(lines):
            if rx.search(line):
                start=max(0,idx-3); end=min(len(lines),idx+4)
                print('---')
                print('\n'.join(lines[start:end]))
