# Project Documentation

성층권 풍선 프로젝트의 시스템 사양, 설계 가이드라인, 분석 보고서 및 최종 결과 리포트를 포함하는 문서 저장소입니다.

## 주요 문서 (Main Documents)

- **[최종 보고서 (FINAL_REPORT.md)](./FINAL_REPORT.md)**: 프로젝트의 전체 개요, 구현 현황 및 최종 검증 결과 요약 (Rev 5.0)
- **[시스템 사양서 (Specification)](./STM32_SpaceBalloon_Specification.md)**: 하드웨어 및 소프트웨어의 상세 기술 사양
- **[구현 현황 (Implementation Status)](./IMPLEMENTATION_STATUS.md)**: 각 모듈별 기능 구현 진척도 상세

## 신뢰성 및 안전성 (Safety & Reliability)

- **[FDIR 설계 가이드](./FDIR.md)**: 결함 감지 및 복구 로직 상세 설계
- **[FMEA 고장 모드 분석](./FMEA.md)**: 시스템 잠재적 고장 분석 및 완화 조치 계획

## 검증 보고서 (Validation Reports)

- **[품질 검증 리포트 저장소](./cppcheck%20&%20gcov%20report/README.md)**:
    - [MISRA C:2023 준수 보고서](./cppcheck%20&%20gcov%20report/code_review_misra_2023.md)
    - [gcov 호스트 테스트 결과](./cppcheck%20&%20gcov%20report/code_review_gcov_report.md)
    - [Cppcheck 정적 분석 요약](./cppcheck%20&%20gcov%20report/cppcheck_summary.md)

## 시뮬레이션 및 메모리
- **[memory/](./memory/)**: 시스템 메모리 맵 및 섹션 배치 가이드
- **[code_review/](./code_review/)**: 개별 센서 드라이버 및 모듈별 심층 코드 리뷰 로그

---
> [!TIP]
> 최신 검증 결과는 **최종 보고서(FINAL_REPORT.md)**의 "4. 검증 결과" 섹션에서 확인할 수 있습니다.
