/**
 * @file xcp.h
 * @brief XCP 프로토콜 인터페이스 - 실시간 캘리브레이션 및 측정
 * @details ASAM XCP (Universal Measurement and Calibration Protocol) v1.0 구현
 *          목적: PID 파라미터 실시간 튜닝, 칼만 필터 파라미터 조정, 변수 모니터링
 *          전송: UART3 (115200 baud, DMA)
 *          프로토콜 스택: XCP on UART (XCP-on-Serial)
 *          최대 패킷 크기: 64바이트
 *          지원 기능:
 *          - CONNECT/DISCONNECT: 세션 관리
 *          - UPLOAD/DOWNLOAD: 메모리 읽기/쓰기 (파라미터 캘리브레이션)
 *          - START_STOP_DAQ: 데이터 수집 시작/정지
 *          - SET_DAQ_PTR/WRITE_DAQ: 측정 변수 테이블 구성
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef XCP_H
#define XCP_H

#include <stdint.h>
#include "pid.h"
#include "kalman.h"

/**
 * @defgroup XCP_CONSTANTS XCP 프로토콜 상수
 * @{
 */

/** @brief XCP 최대 패킷 크기 (바이트) */
#define XCP_MAX_PACKET_SIZE 64

/** @} */ // end of XCP_CONSTANTS

/**
 * @defgroup XCP_STRUCTS XCP 데이터 구조체
 * @{
 */

/**
 * @brief XCP ODT (Object Descriptor Table) 엔트리 구조체
 * @details 측정 변수 테이블 엔트리
 *          DAQ (Data Acquisition) 리스트에서 전송할 변수 정의
 *          사용처: PID 제어 출력, 칼만 필터 상태 벡터, 센서 데이터 모니터링
 */
typedef struct {
    float *ptr;          /**< 측정 변수 포인터 (메모리 주소) */
    uint8_t size;        /**< 변수 크기 (바이트, 예: float=4, int16_t=2) */
    uint8_t type;        /**< 변수 타입 (0=uint8, 1=int16, 2=float 등) */
} XCP_ODT_Entry_t;

/** @} */ // end of XCP_STRUCTS

/**
 * @defgroup XCP_FUNCTIONS XCP 프로토콜 함수
 * @{
 */

/**
 * @brief XCP 프로토콜 초기화
 * @details 초기화 순서:
 *          1. XCP 세션 상태 초기화 (DISCONNECTED)
 *          2. ODT 테이블 초기화 (측정 변수 등록)
 *          3. UART3 DMA 수신 활성화 (XCP 커맨드 수신)
 *          ODT 등록 변수 예시:
 *          - hpid_bat.Kp, hpid_bat.Ki (PID 파라미터)
 *          - hkf.x[0], hkf.x[1] (칼만 필터 상태 벡터)
 *          - heater_battery_cmd, heater_board_cmd (히터 제어 출력)
 * @note App_Init()에서 1회 호출
 *       XCP 마스터 도구: CANape, INCA 등
 */
void XCP_Init(void);

/**
 * @brief XCP 커맨드 처리
 * @param[in] data XCP 커맨드 패킷 버퍼 포인터
 * @param[in] len 패킷 길이 (바이트, 최대 64)
 * @details 지원 커맨드:
 *          - 0xFF (CONNECT): 세션 연결, 응답: 리소스 정보
 *          - 0xFE (DISCONNECT): 세션 종료
 *          - 0xF0 (UPLOAD): 메모리 읽기, 파라미터 조회
 *          - 0xF1 (DOWNLOAD): 메모리 쓰기, 파라미터 변경
 *          - 0xDE (START_STOP_DAQ): DAQ 시작/정지
 *          - 0xE2 (SET_DAQ_PTR): DAQ 포인터 설정
 *          - 0xE1 (WRITE_DAQ): ODT 엔트리 추가
 *          에러 처리:
 *          - 잘못된 커맨드 → 0xFE (ERR_CMD_UNKNOWN) 응답
 *          - 권한 없음 → 0x21 (ERR_ACCESS_DENIED) 응답
 * @note UART3 DMA 인터럽트에서 호출
 *       비블로킹 처리 (응답은 UART3 DMA TX로 전송)
 */
void XCP_ProcessCommand(uint8_t *data, uint8_t len);

/**
 * @brief XCP 측정값 업데이트 (DAQ 데이터 전송)
 * @details 실행 순서:
 *          1. DAQ 상태 확인 (RUNNING인지 체크)
 *          2. ODT 테이블의 모든 엔트리 읽기
 *          3. XCP DAQ 패킷 구성 (PID=0x00, 타임스탬프, ODT 데이터)
 *          4. UART3 DMA 전송 시작
 *          전송 주기: App_Loop()에서 50Hz 호출 (20ms 간격)
 *          패킷 구조 예시:
 *          [0]: 0x00 (DAQ PID)
 *          [1-2]: Timestamp (16비트, ms)
 *          [3-6]: hpid_bat.Kp (float)
 *          [7-10]: hkf.x[0] (float)
 *          ...
 * @note App_Loop()에서 매 주기 호출 (50Hz)
 *       DAQ STOP 상태에서는 전송하지 않음
 */
void XCP_UpdateMeasurements(void);

/** @} */ // end of XCP_FUNCTIONS

/**
 * @defgroup XCP_GLOBALS 외부 전역 변수 (캘리브레이션 대상)
 * @{
 */

/** @brief 배터리 히터 PID 제어기 핸들 (Kp, Ki, Kd 튜닝 가능) */
extern PID_HandleTypeDef hpid_bat;

/** @brief 보드 히터 PID 제어기 핸들 (Kp, Ki, Kd 튜닝 가능) */
extern PID_HandleTypeDef hpid_brd;

/** @brief 칼만 필터 핸들 (Q, R, 상태 벡터 모니터링) */
extern KF_Handle_t hkf;

/** @} */ // end of XCP_GLOBALS

#endif /* XCP_H */
