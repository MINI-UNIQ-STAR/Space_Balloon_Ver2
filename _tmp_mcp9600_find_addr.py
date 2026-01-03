from pypdf import PdfReader
import re
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\MCP9600\adafruit-mcp9600-i2c-thermocouple-amplifier.pdf"
reader=PdfReader(p)
rx=re.compile(r"I2C|0x[0-9A-Fa-f]{2}|address|Register|reg", re.IGNORECASE)
for i,page in enumerate(reader.pages, start=1):
    txt=(page.extract_text() or '')
    if rx.search(txt):
        # print only lines that mention addresses or i2c address
        lines=[ln.strip() for ln in txt.splitlines() if ln.strip()]
        hit=[ln for ln in lines if rx.search(ln)]
        if hit:
            print('\n=== page',i,'===')
            for ln in hit[:60]:
                print(ln)
