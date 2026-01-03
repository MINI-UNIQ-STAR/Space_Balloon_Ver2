import re
from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
pat=re.compile(r'FS[_ ]?(XL|G)|FULL[_ ]?SCALE|dps|mg/LSB|mdps/LSB', re.IGNORECASE)
page_hits=[]
for idx,page in enumerate(reader.pages, start=1):
    txt=page.extract_text() or ''
    if pat.search(txt):
        page_hits.append(idx)
print('pages with FS/full-scale keywords (first 30):', page_hits[:30])
# dump lines around FS mentions for first few pages
for pidx in page_hits[:8]:
    txt=(reader.pages[pidx-1].extract_text() or '')
    lines=txt.splitlines()
    hits=[ln for ln in lines if pat.search(ln)]
    print('\n--- page',pidx,'hit lines',len(hits),'---')
    for ln in hits[:80]:
        print(ln)
