# Renode Test Runner & Scenarios 코드 리뷰

| 항목 | 내용 |
|------|------|
| **위치** | `RENODE_TEST(HIL)/` |
| **핵심 파일** | `run_fdir_tests.py`, `fdir_cases.py` |
| **역할** | Renode 시뮬레이션 제어 및 자동화된 FDIR 테스트 수행 |

## 1. run_fdir_tests.py (Test Runner)

| 기능 | 설명 |
|------|------|
| **상대 경로 지원** | `BASE_DIR` 기반으로 어떤 환경에서도 실행 가능 (Portable) |
| **텔넷 자동화** | Renode 텔넷 포트로 명령어 주입 및 로그 모니터링 |
| **시간 관리** | 테스트별 `duration`에 맞춘 스마트 대기 및 타임아웃 처리 |
| **결과 기록** | 각 테스트 단계별 로그 저장 및 최종 CSV 리포트 생성 |

### 주요 로직
```python
# BASE_DIR 기반 동적 경로 설정
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RES_FILE = os.path.join(BASE_DIR, "renode", "scripts", "test_firmware.resc")
```

## 2. fdir_cases.py (Test Scenarios)

| 항목 | 통계 |
|------|------|
| **총 테스트 케이스** | 22개 |
| **I2C 센서 테스트** | S-01 ~ S-11 (전종 포함) |
| **UART 센서 테스트** | U-01 ~ U-04 (GPS, PMS) |
| **GPIO/시스템 테스트** | G-01(DS18B20), P-01~P-06(PPS, ADC, Timers) |

### 시나리오 구성
```python
{
    "id": "S-01",
    "name": "LSM6DSV16X Disconnect",
    "setup_cmd": "sysbus.i2c1.lsm6dsv16x Unregister",
    "verification_log": "FDIR: S-01 [WARNING]",
    "duration": 5.0
}
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **자동화 수준** | ⭐⭐⭐⭐⭐ | 버튼 하나로 22종 전수 검사 |
| **유지보수성** | ⭐⭐⭐⭐⭐ | `BASE_DIR` 도입으로 환경 의존성 제거 |
| **검증 정밀도** | ⭐⭐⭐⭐⭐ | 로그 기반 키워드 매칭 검증 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**완성도 높은 시뮬레이션 자동화 프레임워크. CI/CD 통합이 즉시 가능한 수준.**
