from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\Schematic_stm32_space_balloon_2025-12-30.pdf"
reader=PdfReader(p)
page=reader.pages[0]
lines=(page.extract_text() or '').splitlines()
# find first occurrence of 'Weact STM32G431CBU6'
key='Weact STM32G431CBU6'
for i,l in enumerate(lines):
    if key in l:
        start=max(0,i-80); end=min(len(lines),i+200)
        for j in range(start,end):
            print(f"{j:04d}: {lines[j]}")
        break
else:
    print('not found')
