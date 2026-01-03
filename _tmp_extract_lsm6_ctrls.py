import re
from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
patterns=[r'CTRL1_XL', r'CTRL2_G', r'CTRL3_C', r'CTRL3\s*\(', r'\bCTRL1\b', r'\bCTRL2\b']
# find pages where patterns occur
hits={pat:[] for pat in patterns}
for idx,page in enumerate(reader.pages, start=1):
    txt=page.extract_text() or ''
    for pat in patterns:
        if re.search(pat, txt):
            hits[pat].append(idx)
for pat, pages in hits.items():
    if pages:
        print(pat, pages[:15])
# dump some lines from pages that mention CTRL1_XL/CTRL2_G specifically
for pat in ['CTRL1_XL','CTRL2_G','CTRL3_C']:
    pages=[]
    for idx,page in enumerate(reader.pages, start=1):
        txt=page.extract_text() or ''
        if pat in txt:
            pages.append(idx)
    pages=pages[:5]
    print('\n===',pat,'pages',pages,'===')
    for pidx in pages:
        lines=(reader.pages[pidx-1].extract_text() or '').splitlines()
        for line in lines:
            if pat in line:
                print('p',pidx,':',line)
