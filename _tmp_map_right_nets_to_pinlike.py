import re
import pdfplumber

pdf_path = r"C:\Users\hyuns\Desktop\stm32_spaceballoon\reference\Schematic_stm32_space_balloon_2025-12-30.pdf"

right_nets = [
    "GPS_INT","VBAT","GPS_Wake","GPS_!RST","MLX_RST","DS18B20","GPS_PPS",
    "I2C2_SDA","I2C2_SCL","SHT_RST","I2C1_SCL","UART3_TX","UART3_RX","LSM_INT",
    "MLX90393","UART1_TX","UART1_RX","I2C1_SDA","PMS_SET","LSM_RST","CO2_RST",
    "SEN_RST","minibulb_PWM","MS_RST","kapton_PWM","UART2_RX","BAT_measure","MCP_RST",
    "UART2_TX"
]

pin_like = re.compile(r"^[ABC]\d{1,2}/")

def ycenter(w):
    return (w['top'] + w['bottom'])/2.0

def xcenter(w):
    return (w['x0'] + w['x1'])/2.0

with pdfplumber.open(pdf_path) as pdf:
    page = pdf.pages[0]
    words = page.extract_words(x_tolerance=1.5, y_tolerance=1.5, keep_blank_chars=False, use_text_flow=True)

    pins=[w for w in words if pin_like.match(w['text'].strip())]

    # pick nets that sit in the right-side list area (x0 > 1000 is a decent heuristic)
    nets=[]
    for w in words:
        t=w['text'].strip()
        if t in right_nets and w['x0'] > 1000:
            nets.append(w)

    pins.sort(key=lambda w:ycenter(w))
    nets.sort(key=lambda w:ycenter(w))

    print('pins:', len(pins), 'nets(right area):', len(nets))

    for nw in nets:
        ny=ycenter(nw)
        best=None
        for pw in pins:
            dy=abs(ycenter(pw)-ny)
            if best is None or dy<best[0]:
                best=(dy,pw)
        dy,pw=best
        print(f"{nw['text']:12s}  y={ny:7.1f} x={xcenter(nw):7.1f}  ->  {pw['text']:12s} (dy={dy:5.1f})")
