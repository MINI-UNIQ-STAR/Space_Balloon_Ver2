# 🛠️ Code Quality & Validation Report

이 디렉토리는 성층권 풍선 프로젝트의 소프트웨어 신뢰성을 보장하기 위한 정적 분석 및 동적 테스트 결과를 포함합니다. 모든 검증 프로세스는 임베디드 핵심 표준(MISRA C:2023)을 준수하며 수행되었습니다.

---

## 📋 검증 리포트 목록

| 리포트 명 | 상세 내용 | 최종 상태 |
|:---|:---|:---:|
| **[MISRA C:2023 준수 보고서](./code_review_misra_2023.md)** | 필수/권고 규칙 준수 여부 및 주요 수정 사항 요약 | **✅ 통과** |
| **[gcov 호스트 테스트 결과](./code_review_gcov_report.md)** | PID, Kalman 알고리즘의 유닛 테스트 및 코드 커버리지 | **✅ PASS** |
| **[Cppcheck 정적 분석 요약](./cppcheck_summary.md)** | 잠재적 버그, 메모리 누수, 스타일 위반 검사 결과 | **✅ Error 0** |

---

## 🔍 검증 도구 및 환경

### 1. 정적 분석 (Static Analysis)
- **도구**: [Cppcheck 2.16.0](https://cppcheck.sourceforge.io/)
- **대상**: `Core/Src` 내 모든 소스 코드
- **기준**: --enable=all, --std=c11

### 2. 동적 테스트 (Dynamic Testing)
- **도구**: gcov, MinGW-w64 GCC
- **환경**: Host-based Testing (SITL)
- **대상**: 핵심 제어 알고리즘 (`pid.c`, `kalman.c`)

### 3. 표준 준수 (Standards Compliance)
- **기준**: [MISRA C:2023](https://www.misra.org.uk/)
- **중점**: 안전성 위반 사항 제거 및 코드 이식성 강화 (`PRIu32` 사용 등)

---

## 📂 기타 리소스
- `cppcheck_report.txt`: Cppcheck의 상세 분석 로그 원본 파일
- `SpaceBalloon_Renode/`: 하드웨어 추뮬레이션 기반의 추가 검증 가이드

---
> [!NOTE]
> **최종 검증 완료일**: 2026-01-23  
> 본 보고서의 모든 결과는 실제 비행용 펌웨어의 안정성을 수학적/논리적으로 뒷받침합니다. 🚀
