from pypdf import PdfReader
import re
p=r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\LSM6DSV16x\lsm6dsv16x.pdf"
reader=PdfReader(p)
needles=[
    r"sensitivity",
    r"LSB",
    r"mdps",
    r"dps",
    r"mg",
    r"g/LSB",
    r"LSB/g",
    r"mdps/LSB",
    r"m?g/LSB",
    r"full[- ]scale",
]
rx=re.compile('|'.join(f'(?:{n})' for n in needles), re.IGNORECASE)

hits=[]
for i,page in enumerate(reader.pages, start=1):
    txt=page.extract_text() or ''
    if rx.search(txt):
        # keep only pages likely about sensitivities: must mention both accel and gyro or mention mg/LSB or mdps
        if re.search(r"mg|mdps|g/LSB|dps", txt, re.IGNORECASE):
            hits.append(i)

print('candidate_pages:', hits[:80])
print('count:', len(hits))

# dump small snippets around first few occurrences on first ~10 candidate pages
for pidx in hits[:12]:
    txt=(reader.pages[pidx-1].extract_text() or '').splitlines()
    print('\n===== PAGE',pidx,'(snippets) =====')
    for j,line in enumerate(txt):
        if rx.search(line):
            start=max(0,j-2); end=min(len(txt), j+3)
            snippet='\n'.join(txt[start:end])
            print('---'); print(snippet)
