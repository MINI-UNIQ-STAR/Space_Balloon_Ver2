#!/usr/bin/env python3
"""
@file convert_flight_data.py
@brief 비행 데이터(JSON)를 C 헤더 파일로 변환하는 스크립트
@details RS41 라디오존데의 원본 데이터(JSON)를 파싱하여 SITL 시뮬레이션용
         `flight_data.h` 파일을 자동 생성함.
         - 주요 추출 필드: 위경도, 고도, 속도, 기온, 배터리, 위성 수
         - 프레임 중복 제거 및 정렬 수행
@author Hyeonsu Park
@date 2026-01-13
"""
import json
import sys
from pathlib import Path

def main():
    script_dir = Path(__file__).parent.resolve()
    input_file = script_dir.parent / "simulation_reference_data" / "V4630075.json"
    output_file = script_dir / "flight_data.h"
    
    with open(input_file, 'r') as f:
        data = json.load(f)
    
    # Deduplicate by frame number
    unique_frames = {}
    for d in data:
        frame = d.get('frame', 0)
        if frame not in unique_frames:
            unique_frames[frame] = d
    
    # Sort by frame number
    sorted_data = [unique_frames[k] for k in sorted(unique_frames.keys())]
    
    print(f"Loaded {len(sorted_data)} unique frames from {input_file.name}")
    
    # Generate C header
    with open(output_file, 'w') as f:
        f.write("// Auto-generated from V4630075.json\n")
        f.write("// RS41 Radiosonde flight data for simulation\n")
        f.write("#ifndef FLIGHT_DATA_H\n")
        f.write("#define FLIGHT_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write("typedef struct {\n")
        f.write("    int32_t lat_e7;      /**< 위도 (도 * 1e7) */\n")
        f.write("    int32_t lon_e7;      /**< 경도 (도 * 1e7) */\n")
        f.write("    float alt_m;         /**< 고도 (m) */\n")
        f.write("    float vel_v;         /**< 수직 속도 (m/s) */\n")
        f.write("    float vel_h;         /**< 수평 속도 (m/s) */\n")
        f.write("    float temp_c;        /**< 기온 (C) */\n")
        f.write("    uint16_t batt_mv;    /**< 배터리 (mV) */\n")
        f.write("    uint8_t sats;        /**< GPS 위성 수 */\n")
        f.write("    float heading_deg;   /**< 헤딩 (도) */\n")
        f.write("} flight_data_point_t;\n\n")
        
        f.write(f"#define FLIGHT_DATA_COUNT {len(sorted_data)}\n\n")
        
        f.write("static const flight_data_point_t flight_data[] = {\n")
        
        for i, d in enumerate(sorted_data):
            lat_e7 = int(d.get('lat', 0) * 1e7)
            lon_e7 = int(d.get('lon', 0) * 1e7)
            alt = d.get('alt', 0)
            vel_v = d.get('vel_v', 0)
            vel_h = d.get('vel_h', 0)
            temp = d.get('temp', -50.0) if 'temp' in d else -50.0
            batt = int(d.get('batt', 2.8) * 1000)  # V 단위를 mV로 변환
            sats = d.get('sats', 0)
            heading = d.get('heading', 0)
            
            comma = "," if i < len(sorted_data) - 1 else ""
            f.write(f"    {{ {lat_e7}, {lon_e7}, {alt:.2f}f, {vel_v:.2f}f, {vel_h:.2f}f, {temp:.1f}f, {batt}, {sats}, {heading:.2f}f }}{comma}\n")
        
        f.write("};\n\n")
        f.write("#endif // FLIGHT_DATA_H\n")
    
    print(f"Generated {output_file.name} with {len(sorted_data)} data points")
    
    # Print summary
    alts = [d.get('alt', 0) for d in sorted_data]
    print(f"Altitude range: {min(alts):.0f}m - {max(alts):.0f}m")

if __name__ == "__main__":
    main()
