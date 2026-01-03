from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\MCP9600\adafruit-mcp9600-i2c-thermocouple-amplifier.pdf"
reader=PdfReader(p)
print('pages', len(reader.pages))
for i in range(min(5,len(reader.pages))):
    txt=(reader.pages[i].extract_text() or '')
    print('\n--- page', i+1, '---')
    print(txt[:1200])
