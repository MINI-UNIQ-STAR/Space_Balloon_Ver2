from pypdf import PdfReader
import re
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\MCP9600\adafruit-mcp9600-i2c-thermocouple-amplifier.pdf"
reader=PdfReader(p)
queries=[
    r"Cold", r"Junction", r"Ambient", r"Internal", r"0x00", r"0x01", r"0x02", r"register", r"temp"
]
rx=re.compile('|'.join(queries), re.IGNORECASE)
for i,page in enumerate(reader.pages, start=1):
    txt=(page.extract_text() or '')
    if rx.search(txt):
        lines=[ln.strip() for ln in txt.splitlines() if ln.strip()]
        hits=[ln for ln in lines if rx.search(ln)]
        # print only pages with something beyond generic overview
        if i>=6:
            print('\n=== page',i,'===')
            for ln in hits[:120]:
                print(ln)
