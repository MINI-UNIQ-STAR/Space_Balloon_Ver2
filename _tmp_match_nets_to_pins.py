import re
import math
import pdfplumber

pdf_path = r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\Schematic_stm32_space_balloon_2025-12-30.pdf"

nets = [
    "GPS_Wake","GPS_!RST","MLX_RST","SHT_RST","MS_RST","CO2_RST","MCP_RST","SEN_RST","PMS_SET","LSM_RST","LSM_INT",
    "UART3_TX","UART3_RX","UART1_TX","UART1_RX","UART2_TX","UART2_RX",
    "I2C1_SCL","I2C1_SDA","I2C2_SCL","I2C2_SDA",
    "GPS_INT","GPS_PPS","BAT_measure",
]

# Candidate pin labels on the WeAct symbol look like:
#  - "B11/RX3" or "C10/TX4" or "A15" etc.
# We'll search for text that starts with A/B/C and a number.
# Some labels include extra like "/TX3".
pin_rx = re.compile(r"\b([ABC])(\d{1,2})\b")
pin_rx2 = re.compile(r"\b([ABC]\d{1,2})(?:/[^\s]+)?\b")

# Extract words with bbox (x0, top, x1, bottom)
# pdfplumber uses top-origin coordinates.

def center(word):
    return ((word["x0"] + word["x1"]) / 2.0, (word["top"] + word["bottom"]) / 2.0)

with pdfplumber.open(pdf_path) as pdf:
    page = pdf.pages[0]
    words = page.extract_words(x_tolerance=1.5, y_tolerance=1.5, keep_blank_chars=False, use_text_flow=True)

    net_words = []
    pin_words = []

    for w in words:
        t = w["text"].strip()
        if t in nets:
            net_words.append(w)
        # capture tokens that look like pin labels
        if pin_rx.search(t) or ("/" in t and pin_rx2.search(t)):
            pin_words.append(w)

    # Normalize pin tokens: prefer something like A4 or B10 if present in token.
    def pin_key(w):
        t = w["text"].strip()
        m = pin_rx2.search(t)
        if not m:
            return None
        return m.group(1)

    pin_items = [(pin_key(w), w) for w in pin_words]
    pin_items = [(k, w) for (k, w) in pin_items if k]

    def dist(a, b):
        ax, ay = center(a)
        bx, by = center(b)
        return math.hypot(ax - bx, ay - by)

    # For each net label, find closest pin label within a reasonable radius.
    results = []
    for nw in net_words:
        best = None
        for pk, pw in pin_items:
            d = dist(nw, pw)
            if best is None or d < best[0]:
                best = (d, pk, pw)
        if best:
            d, pk, pw = best
            results.append((nw["text"], pk, d, center(nw), pw["text"], center(pw)))

    results.sort(key=lambda x: x[2])

    print("Found nets:", len(net_words), "pins:", len(pin_items))
    for net, pk, d, nxy, ptxt, pxy in results:
        print(f"{net:12s} -> {pk:4s}   d={d:7.1f}   net@{nxy}  pin_token='{ptxt}' @ {pxy}")
