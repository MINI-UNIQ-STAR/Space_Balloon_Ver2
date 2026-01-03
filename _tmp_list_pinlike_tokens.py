import re
import pdfplumber

pdf_path = r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\Schematic_stm32_space_balloon_2025-12-30.pdf"

pin_like = re.compile(r"^[ABC]\d{1,2}/")

with pdfplumber.open(pdf_path) as pdf:
    page = pdf.pages[0]
    words = page.extract_words(x_tolerance=1.5, y_tolerance=1.5, keep_blank_chars=False, use_text_flow=True)

    # locate Weact word
    weacts=[w for w in words if w['text'].startswith('Weact')]
    if weacts:
        w=weacts[0]
        print('Weact at', w['x0'], w['top'], w['x1'], w['bottom'])
    else:
        print('Weact not found')

    pins=[]
    for w in words:
        t=w['text'].strip()
        if pin_like.match(t):
            pins.append(w)

    # Sort by x then y for readability
    pins.sort(key=lambda w:(round(w['x0'],1), round(w['top'],1)))

    print('pin-like tokens:', len(pins))
    for w in pins:
        print(f"{w['text']:12s} x0={w['x0']:.1f} top={w['top']:.1f} x1={w['x1']:.1f} bot={w['bottom']:.1f}")
