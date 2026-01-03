from pypdf import PdfReader
import re
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)

keys=[('CTRL1 (10h)', re.compile(r'CTRL1\s*\(10h\)', re.IGNORECASE)),
      ('CTRL2 (11h)', re.compile(r'CTRL2\s*\(11h\)', re.IGNORECASE)),
      ('CTRL3 (12h)', re.compile(r'CTRL3\s*\(12h\)', re.IGNORECASE)),
]

found={k:[] for k,_ in keys}
for i,page in enumerate(reader.pages, start=1):
    txt=page.extract_text() or ''
    for name,rx in keys:
        if rx.search(txt):
            found[name].append(i)

print(found)
# Dump first occurrence pages for CTRL1/2/3
for name,pages in found.items():
    if pages:
        pidx=pages[0]
        txt=reader.pages[pidx-1].extract_text() or ''
        print('\n===== %s @ PAGE %d =====' % (name,pidx))
        print(txt)
