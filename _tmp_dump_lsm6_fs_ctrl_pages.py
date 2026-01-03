from pypdf import PdfReader
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
for pidx in [69,70,71]:
    txt=(reader.pages[pidx-1].extract_text() or '')
    print('\n===== PAGE',pidx,'=====')
    print(txt)
