from pptx import Presentation
from pptx.util import Inches, Pt, Cm
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN
from pptx.enum.shapes import MSO_SHAPE

# -----------------------------------------------------------------------------
# 설정 및 헬퍼 함수
# -----------------------------------------------------------------------------

def set_font(paragraph, font_name='맑은 고딕', font_size=Pt(18), bold=False, color=None):
    """폰트 스타일을 일괄 적용하는 함수"""
    for run in paragraph.runs:
        run.font.name = font_name
        run.font.size = font_size
        run.font.bold = bold
        if color:
            run.font.color.rgb = color

def add_slide(prs, layout_index, title_text):
    """슬라이드 추가 및 제목 설정 함수"""
    slide = prs.slides.add_slide(prs.slide_layouts[layout_index])
    title = slide.shapes.title
    title.text = title_text
    
    # 제목 스타일 (검정색, 굵게)
    for paragraph in title.text_frame.paragraphs:
        paragraph.font.name = '맑은 고딕'
        paragraph.font.size = Pt(32)
        paragraph.font.bold = True
        paragraph.font.color.rgb = RGBColor(0, 0, 0)
        
    return slide

def create_table(slide, data, col_widths, top=Inches(2.0)):
    """슬라이드에 표를 생성하는 함수"""
    rows = len(data)
    cols = len(data[0])
    left = Inches(0.5)
    width = Inches(9.0) # 전체 너비
    
    table = slide.shapes.add_table(rows, cols, left, top, width, Inches(0.8)).table

    # 컬럼 너비 설정
    for i, w in enumerate(col_widths):
        table.columns[i].width = w

    # 데이터 입력 및 스타일링
    for r in range(rows):
        for c in range(cols):
            cell = table.cell(r, c)
            cell.text = str(data[r][c])
            
            # 텍스트 포맷팅
            paragraph = cell.text_frame.paragraphs[0]
            paragraph.font.name = '맑은 고딕'
            paragraph.font.size = Pt(10) # 표 내용 폰트 크기
            
            # 헤더 행 스타일 (파란색 배경, 흰색 글씨)
            if r == 0:
                cell.fill.solid()
                cell.fill.fore_color.rgb = RGBColor(44, 62, 80) # Dark Blue
                paragraph.font.color.rgb = RGBColor(255, 255, 255)
                paragraph.font.bold = True
                paragraph.alignment = PP_ALIGN.CENTER
            else:
                # 일반 셀 스타일
                paragraph.font.color.rgb = RGBColor(0, 0, 0)
                # 숫자나 짧은 데이터는 중앙 정렬
                if c >= 3: 
                    paragraph.alignment = PP_ALIGN.CENTER

# -----------------------------------------------------------------------------
# 메인 로직
# -----------------------------------------------------------------------------

def generate_presentation():
    prs = Presentation()
    
    # PPT 화면 비율 16:9 설정
    prs.slide_width = Inches(13.333)
    prs.slide_height = Inches(7.5)

    # --- 1. 타이틀 슬라이드 ---
    slide = prs.slides.add_slide(prs.slide_layouts[0]) # Title Slide
    title = slide.shapes.title
    subtitle = slide.placeholders[1]
    
    title.text = "성층권 고가용성 센서 플랫폼\n상세 개발 보고서"
    subtitle.text = "STM32 Stratospheric Balloon Project\nDate: 2026-01-09 | Team: Antigravity AI"
    
    # 타이틀 스타일 조정
    title.text_frame.paragraphs[0].font.name = "맑은 고딕"
    title.text_frame.paragraphs[0].font.bold = True
    subtitle.text_frame.paragraphs[0].font.name = "맑은 고딕"

    # --- 2. 시스템 아키텍처 ---
    slide = add_slide(prs, 1, "1. 시스템 아키텍처 상세")
    body = slide.placeholders[1]
    tf = body.text_frame
    tf.text = "Software Architecture"
    
    p = tf.add_paragraph()
    p.text = "• Model: Bare-metal Super Loop (RTOS Free)"
    p.level = 1
    p = tf.add_paragraph()
    p.text = "• Frequency: 50Hz (20ms fixed period)"
    p.level = 1
    p = tf.add_paragraph()
    p.text = "• Determinism: Jitter < 10µs (Hard Real-time)"
    p.level = 1
    p = tf.add_paragraph()
    p.text = "• Sync: GPS 1PPS Interrupt Based"
    
    # 하드웨어 토폴로지 표 추가
    data_hw = [
        ["Bus", "Role", "Connected Devices"],
        ["I2C1", "Flight Dynamics", "LSM6DSV16X, MLX90393, GDK101"],
        ["I2C3", "Environment", "MS5611, SHT31-D, CM1107N, MCP9600"],
        ["UART", "Independent", "UART1(GPS), UART2(LoRa), UART3(Dust)"]
    ]
    create_table(slide, data_hw, [Inches(2), Inches(3), Inches(7)], top=Inches(4.5))

    # --- 3. 회로도 상세 (Upside) ---
    slide = add_slide(prs, 5, "2.1 회로도 상세: Upside Boards") # Title Only Layout
    
    # Upside Board #1 Table
    data_up1 = [
        ["Ref", "Component", "Function"],
        ["U1", "MS5611", "Barometer (10-1200mbar)"],
        ["U2", "SHT31-D", "Temp/Humidity (Heater On)"],
        ["U3", "CM1107N", "CO2 Sensor (NDIR)"],
        ["U4", "MCP9600", "Thermocouple Interface"],
        ["U5", "SEN0321", "Ozone Sensor"]
    ]
    
    # Upside Board #2 Text Box
    txBox = slide.shapes.add_textbox(Inches(0.5), Inches(1.5), Inches(5), Inches(2))
    tf = txBox.text_frame
    tf.text = "Upside Board #1 (Sensor Array)"
    tf.paragraphs[0].font.bold = True
    tf.paragraphs[0].font.size = Pt(16)
    
    create_table(slide, data_up1, [Inches(1), Inches(2), Inches(3)], top=Inches(2.0))
    
    # 위치 조정하여 두 번째 표 추가
    txBox2 = slide.shapes.add_textbox(Inches(7), Inches(1.5), Inches(5), Inches(2))
    tf2 = txBox2.text_frame
    tf2.text = "Upside Board #2 (Main Controller)"
    tf2.paragraphs[0].font.bold = True
    tf2.paragraphs[0].font.size = Pt(16)
    
    p = tf2.add_paragraph()
    p.text = "• MCU: STM32G431CBU6 (170MHz)\n• Power: 1S Li-ion Input -> 3.3V LDO\n• Thermal: PWM MOSFET Drivers"
    p.font.size = Pt(12)

    # --- 4. 회로도 상세 (Downside) ---
    slide = add_slide(prs, 5, "2.2 회로도 상세: Downside Board")
    
    data_down = [
        ["Ref", "Component", "Interface", "Protection"],
        ["U6", "LSM6DSV16X", "I2C1", "P-MOS (PB13)"],
        ["U7", "MLX90393", "I2C1", "P-MOS (PB14)"],
        ["U8", "GDK101", "I2C1", "P-MOS (PB2)"]
    ]
    create_table(slide, data_down, [Inches(1), Inches(2.5), Inches(1.5), Inches(3)])
    
    # 설명 텍스트
    txBox = slide.shapes.add_textbox(Inches(0.5), Inches(4.5), Inches(10), Inches(2))
    tf = txBox.text_frame
    tf.text = "Active Protection (Anti-Latchup):"
    tf.paragraphs[0].font.bold = True
    p = tf.add_paragraph()
    p.text = "우주 방사선(SEU)에 의한 Single Event Latch-up 발생 시, MCU가 해당 라인의 전원을 물리적으로 차단하고 재인가(Power Cycle)할 수 있도록 설계됨."

    # --- 5. FMEA: 센서 (Full Table) ---
    slide = add_slide(prs, 5, "3.1 FMEA: 센서 서브시스템 (전체)")
    
    # 데이터가 많으므로 폰트를 작게 조정해야 함 (create_table 함수 내에서 10pt로 설정됨)
    data_sensor = [
        ["ID", "Mode", "Cause & Effect", "S", "O", "D", "RPN", "Mitigation"],
        ["S-01", "IMU 드리프트", "진동/온도 -> 바이어스 오차", 3, 3, 3, 27, "Kalman Filter, 온도 보상"],
        ["S-02", "IMU 락업", "SEU 방사선 -> Latch-up", 3, 2, 3, 18, "WDT, P-MOS 전원 사이클"],
        ["S-03", "IMU 노이즈", "기계적 진동 -> 데이터 오염", 2, 4, 2, 16, "LPF, 방진 댐퍼"],
        ["S-04", "GPS No Fix", "안테나/자세 -> 위치 상실", 4, 3, 2, 24, "Barometer 백업 고도 사용"],
        ["S-05", "GPS 데이터오류", "UART 노이즈 -> CRC 에러", 2, 3, 1, 6, "NMEA Checksum 검증"],
        ["S-06", "GPS 파싱실패", "버퍼 오버플로우", 2, 2, 2, 8, "Circular Buffer, DMA"],
        ["S-07", "Baro 스파이크", "기류 불안정 -> 튀는 값", 2, 3, 2, 12, "이동 평균 필터"],
        ["S-08", "Baro 오프셋", "온도 변화 -> 영점 이동", 2, 2, 2, 8, "GPS 고도와 상호 검증"],
        ["S-09", "Baro 멈춤", "I2C 버스 에러", 3, 1, 3, 9, "FDIR 타임아웃 리셋"],
        ["S-10", "SHT31 동결", "성층권 결로 -> 센서 마비", 3, 5, 1, 15, "내장 히터 활성화"],
        ["S-11", "SHT31 결로", "급격한 하강 -> 습기 응결", 2, 4, 2, 16, "히터 가동 프로파일"],
        ["S-12", "GDK101 노이즈", "전원 노이즈 유입", 2, 4, 2, 16, "이동 평균, 필터"],
        ["S-13", "GDK101 고장", "물리적 충격", 2, 1, 3, 6, "값 0 지속 시 경고"]
    ]
    # S,O,D 컬럼 폭을 좁게 설정
    cols = [Inches(0.7), Inches(1.5), Inches(2.5), Inches(0.5), Inches(0.5), Inches(0.5), Inches(0.7), Inches(3)]
    create_table(slide, data_sensor, cols, top=Inches(1.5))

    # --- 6. FMEA: 전력 (High Priority) ---
    slide = add_slide(prs, 5, "3.2 FMEA: 전력 서브시스템 (High Priority)")
    
    data_power = [
        ["ID", "Mode", "Cause & Effect", "S", "O", "D", "RPN", "Mitigation"],
        ["P-01", "배터리 저온", "-60C 저항 급증 -> Voltage Sag", 4, 4, 2, 32, "필름 히터(PID), 단열재"],
        ["P-02", "과방전", "미션 초과 -> 셧다운", 5, 1, 5, 25, "저전압 부하 차단 로직"],
        ["P-03", "셀 불균형", "충전 불량 -> 용량 감소", 3, 2, 3, 18, "발사 전 밸런싱 체크"],
        ["P-04", "LDO 과열", "부하 급증 -> 열적 차단", 4, 2, 2, 16, "방열 설계, PCB 최적화"],
        ["P-05", "LDO 리플", "입력 변동 -> 센서 노이즈", 3, 3, 2, 18, "입출력 커패시터 보강"]
    ]
    create_table(slide, data_power, cols)

    # --- 7. FMEA: 통신 & 열제어 ---
    slide = add_slide(prs, 5, "3.3 FMEA: 통신 및 열제어")
    
    data_comm_thermal = [
        ["ID", "Mode", "Cause & Effect", "S", "O", "D", "RPN", "Mitigation"],
        ["C-01", "LoRa 링크손실", "거리/지형 -> TM 두절", 4, 3, 2, 24, "SD카드 로깅(Blackbox)"],
        ["C-02", "프레임 손상", "노이즈 -> CRC 에러", 2, 3, 2, 12, "CRC16 체크섬, 재전송"],
        ["C-03", "주파수 편차", "저온 -> 오실레이터 편차", 4, 2, 2, 16, "TCXO 사용, 대역폭 여유"],
        ["T-01", "히터 단선", "충격 -> 단선 (보온실패)", 4, 1, 5, 20, "커넥터 고정, 이중화"],
        ["T-02", "과열", "제어 오류 -> 소자 파손", 3, 1, 5, 15, "SW 안전 차단 (80C Limit)"],
        ["T-03", "PWM 고장", "타이머 오류 -> 제어 불가", 3, 1, 3, 9, "타이머 상태 모니터링"],
        ["T-04", "단열재 파손", "충격 -> 하우징 크랙", 3, 1, 4, 12, "테이핑 보강, 충격 흡수재"]
    ]
    create_table(slide, data_comm_thermal, cols)

    # --- 8. FMEA: 소프트웨어 ---
    slide = add_slide(prs, 5, "3.4 FMEA: 소프트웨어")
    
    data_sw = [
        ["ID", "Mode", "Cause & Effect", "S", "O", "D", "RPN", "Mitigation"],
        ["W-01", "스택 오버플로", "재귀/변수 과다 -> 멈춤", 5, 1, 4, 20, "정적 분석, 스택 모니터링"],
        ["W-02", "무한 루프", "I2C 타임아웃 -> 멈춤", 4, 2, 2, 16, "Watchdog Timer (WDT)"],
        ["W-03", "메모리 누수", "동적 할당 미해제", 4, 1, 4, 16, "정적 할당(No Malloc) 원칙"],
        ["W-04", "ISR 지연", "인터럽트 과부하", 3, 3, 3, 27, "실행 시간 계측, 우선순위"],
        ["W-05", "Kalman 발산", "입력 오류 -> 값 발산", 3, 2, 2, 12, "입력값 검증, 리셋 로직"],
        ["W-06", "Float 오류", "NaN/Inf 발생", 3, 1, 2, 6, "isnan() 검사 후 처리"]
    ]
    create_table(slide, data_sw, cols)

    # --- 9. FDIR Architecture ---
    slide = add_slide(prs, 1, "4. FDIR 아키텍처 및 복구 전략")
    body = slide.placeholders[1]
    
    tf = body.text_frame
    p = tf.add_paragraph()
    p.text = "Detection & Isolation"
    p.font.bold = True
    
    p = tf.add_paragraph()
    p.text = "1. Timeout: IMU(100ms), GPS(5s), Env(3s)"
    p.level = 1
    p = tf.add_paragraph()
    p.text = "2. Range: 기압(1k~110kPa), 온도(-80~80C)"
    p.level = 1
    p = tf.add_paragraph()
    p.text = "3. Continuity: 고도 점프 500m 감지"
    p.level = 1
    
    data_recovery = [
        ["Lev", "Action", "Detail"],
        ["L1", "Retry", "단순 재시도 (Transient Error)"],
        ["L2", "Soft Reset", "드라이버 재초기화 + I2C 9-Clock Pulse"],
        ["L3", "Hard Reset", "P-MOS Power Cycle (Latch-up 해제)"],
        ["L4", "Isolation", "영구 비활성화 (대체값 사용)"]
    ]
    # 테이블을 텍스트 아래에 추가하기 위해 별도 처리
    create_table(slide, data_recovery, [Inches(1), Inches(2), Inches(6)], top=Inches(4.5))

    # --- 10. Sensor FDIR Details ---
    slide = add_slide(prs, 5, "5. 센서별 FDIR 상세 설정")
    
    data_sensor_fdir = [
        ["Sensor", "Interface", "Timeout", "Reset Pin", "Action"],
        ["LSM6DSV16X", "I2C1", "100 ms", "PB13", "Power Cycle"],
        ["MLX90393", "I2C1", "500 ms", "PB14", "Power Cycle"],
        ["GDK101", "I2C1", "3000 ms", "PB2", "Power Cycle"],
        ["MS5611", "I2C3", "1000 ms", "PA5", "Reset"],
        ["SHT31", "I2C3", "3000 ms", "PB11", "Power Cycle"],
        ["MCP9600", "I2C3", "3000 ms", "PA4", "Reset"],
        ["CM1107N", "I2C3", "5000 ms", "PB0", "Reset"],
        ["GPS", "UART1", "5000 ms", "PA9", "RST Low"],
        ["PMS3003", "UART2", "5000 ms", "PB10", "Sleep/Wake"]
    ]
    create_table(slide, data_sensor_fdir, [Inches(2), Inches(1.5), Inches(1.5), Inches(1.5), Inches(2)])

    # --- 11. Implementation Status ---
    slide = add_slide(prs, 1, "6. 구현 및 검증 현황 (2026-01-09)")
    body = slide.placeholders[1]
    
    tf = body.text_frame
    p = tf.add_paragraph()
    p.text = "Priority 0 & 1 Complete"
    p.font.bold = True
    
    items = [
        "Sensors_Reset(): 110 LOC (9개 센서 통합 복구)",
        "I2C Recovery: 104 LOC (BSP_I2C_Recovery)",
        "Telemetry: 132 Byte Packet Send Verified",
        "GPS NMEA CRC: minmea_check() 구현 완료",
        "Low Voltage: 2.7V/2.9V Hysteresis 적용",
        "1PPS Sync: 50Hz Slot Allocation (100 LOC)"
    ]
    for item in items:
        p = tf.add_paragraph()
        p.text = "• " + item
        p.level = 1

    # Unit Test Result Table
    data_test = [
        ["Module", "Cases", "Result"],
        ["FDIR Logic", "4", "PASS"],
        ["Kalman Filter", "4", "PASS"],
        ["PID Control", "5", "PASS"],
        ["Low Voltage", "5", "PASS"],
        ["GPS CRC", "9", "PASS"]
    ]
    create_table(slide, data_test, [Inches(3), Inches(2), Inches(2)], top=Inches(4.5))

    # --- 12. Future Plan ---
    slide = add_slide(prs, 1, "7. 향후 계획 (Next Steps)")
    body = slide.placeholders[1]
    tf = body.text_frame
    
    steps = [
        ("Priority 1.5: HW 통합 테스트 (1주)", ["P-MOS 스위칭 동작 및 Inrush 전류 측정", "GPIO 타이밍 검증", "I2C 복구 파형 분석"]),
        ("Priority 2: HITL / SITL (3-5일)", ["ESP32 Mock System (2674 LOC) 활용", "고장 주입(Fault Injection)", "RS41 실측 데이터 기반 SITL"]),
        ("Final: FM 전환 (2주)", ["환경 챔버 (-40~60C)", "FIT (3시간 연속 구동)", "최종 비행 모델 승인"])
    ]
    
    for title, subitems in steps:
        p = tf.add_paragraph()
        p.text = title
        p.font.bold = True
        p.font.size = Pt(20)
        for item in subitems:
            sub_p = tf.add_paragraph()
            sub_p.text = "  - " + item
            sub_p.level = 1
            sub_p.font.size = Pt(16)

    # 파일 저장
    prs.save('STM32_Full_Report.pptx')
    print("STM32_Full_Report.pptx 생성 완료!")

if __name__ == "__main__":
    generate_presentation()