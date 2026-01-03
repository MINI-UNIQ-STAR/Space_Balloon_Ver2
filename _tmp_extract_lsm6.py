import re
from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
print('pages', len(reader.pages))
keys=['WHO_AM_I','OUTX_L_G','OUTX_L_A','CTRL1_XL','CTRL2_G','CTRL3','CTRL3_C','IF_INC']
found={k:[] for k in keys}
for idx, page in enumerate(reader.pages, start=1):
    txt=page.extract_text() or ''
    if not txt:
        continue
    for k in keys:
        if k in txt:
            found[k].append(idx)
for k,v in found.items():
    if v:
        print(k, 'pages', v[:10])
# dump lines containing key tokens from first few relevant pages
snip_pages=sorted(set(found['WHO_AM_I'][:5] + found['OUTX_L_G'][:5] + found['OUTX_L_A'][:5] + found['CTRL1_XL'][:5] + found['CTRL2_G'][:5]))
for pidx in snip_pages[:8]:
    lines=(reader.pages[pidx-1].extract_text() or '').splitlines()
    hits=[line for line in lines if any(tok in line for tok in ['WHO_AM_I','OUTX_L_G','OUTX_L_A','CTRL1_XL','CTRL2_G','CTRL3','CTRL3_C'])]
    print('\n--- page', pidx, '---')
    for line in hits[:120]:
        print(line)
