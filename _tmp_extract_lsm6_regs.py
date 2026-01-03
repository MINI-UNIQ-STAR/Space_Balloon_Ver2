from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
for pidx in [52,53,54,65,66,67,181,187,188]:
    txt=(reader.pages[pidx-1].extract_text() or '')
    if not txt:
        continue
    lines=txt.splitlines()
    hits=[ln for ln in lines if any(tok in ln for tok in ['CTRL1','CTRL2','CTRL3','ODR','FS','XL','GYRO'])]
    print('\n--- page',pidx,'hits',len(hits),'---')
    for ln in hits[:200]:
        print(ln)
