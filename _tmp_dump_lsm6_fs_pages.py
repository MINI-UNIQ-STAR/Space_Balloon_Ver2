import re
from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
pages=[32,39,40,69,71,104,107,108,115,116,119,120]
for pidx in pages:
    txt=(reader.pages[pidx-1].extract_text() or '')
    if not txt:
        continue
    lines=txt.splitlines()
    pat=re.compile(r'FS_|full scale|CTRL8|CTRL7|CTRL6|CTRL4|CTRL5|CTRL_EIS|UI_CTRL2_OIS|CTRL9|CTRL10|CTRL11|CTRL12|CTRL13|CTRL14|CTRL15', re.IGNORECASE)
    hits=[ln for ln in lines if pat.search(ln)]
    print('\n--- page',pidx,'hits',len(hits),'---')
    for ln in hits[:140]:
        print(ln)
