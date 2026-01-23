/**
 * @file bsp.h
 * @brief 보드 지원 패키지 (Board Support Package) - 하드웨어 추상화 계층
 * @details STM32G431CBU6 기반 하드웨어 초기화 및 저수준 I/O 인터페이스
 *          I2C1 (Downside Board): LSM6DSV16X, MLX90393, GDK101
 *          I2C3 (Upside Board): MS5611, SHT31, CM1107N, MCP9600, SEN0321
 *          UART: XA1110 GPS (UART1), PMS3003 (UART2), 텔레메트리 (UART3)
 *          ADC: 배터리 전압 모니터링
 *          GPIO: 센서 전원 제어, I2C 버스 복구
 * @author SpaceBalloon Team
 * @date 2026-01-08
 * @version 1.0
 */

#ifndef BSP_H_
#define BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* MISRA C 2023: Float Type Definition */
typedef float float32_t;

/**
 * @defgroup BSP_ADDRESSES I2C 센서 주소 정의
 * @{
 */

/* I2C1 버스 (Downside Board) */
#define BSP_LSM6DSV16X_ADDR     0x6B    /**< LSM6DSV16X 6축 IMU (7비트 주소) */
#define BSP_MLX90393_ADDR       0x0C    /**< MLX90393 3축 자기계 (7비트 주소) */
#define BSP_GDK101_ADDR         0x18    /**< GDK101 방사선 센서 (7비트 주소) */

/* I2C3 버스 (Upside Board) */
#define BSP_SHT31_ADDR          0x44    /**< SHT31-D 온습도 센서 (7비트 주소, ADDR 핀 Low) */
#define BSP_MS5611_ADDR         0x77    /**< MS5611 기압계 (7비트 주소, CSB 핀 High) */
#define BSP_CM1107N_ADDR        0x31    /**< CM1107N CO2 센서 (7비트 주소) */
#define BSP_MCP9600_ADDR        0x60    /**< MCP9600 열전대 ADC (7비트 주소, 기본값 0x60) */
#define BSP_SEN0321_ADDR        0x70    /**< SEN0321 오존 센서 (7비트 주소) */

/** @} */ // end of BSP_ADDRESSES

/**
 * @defgroup BSP_INIT 초기화 함수
 * @{
 */

/**
 * @brief 보드 지원 패키지 초기화
 * @details 초기화 순서:
 *          1. GPIO 출력 초기값 설정 (센서 전원 핀, I2C 버스 복구 핀)
 *          2. ADC 캘리브레이션 (배터리 전압 측정용)
 *          3. 타이머/DMA 초기화 (HAL에서 이미 수행된 것으로 가정)
 * @note App_Init()에서 최초 1회 호출
 *       HAL_Init(), SystemClock_Config() 후 호출 필요
 */
void BSP_Init(void);

/**
 * @brief 센서 전원 공급 활성화
 * @details GPIO 제어를 통한 센서 전원 시퀀싱:
 *          1. 3.3V 레귤레이터 활성화
 *          2. 센서 리셋 핀 릴리스 (Active Low)
 *          3. 안정화 대기 (10ms)
 * @note BSP_Init() 후 센서 초기화 전 호출
 */
void BSP_Sensor_PowerOn(void);

/** @} */ // end of BSP_INIT

/**
 * @defgroup BSP_I2C1 I2C1 버스 통신 함수 (Downside Board)
 * @{
 */

/**
 * @brief I2C1 레지스터 쓰기
 * @param[in] DevAddr 디바이스 주소 (7비트, 좌측 정렬 후 HAL에서 자동 쉬프트)
 * @param[in] Reg 레지스터 주소 (8비트 또는 16비트, 센서별 상이)
 * @param[in] pData 쓰기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Mem_Write() 래퍼 함수, 타임아웃 1000ms
 * @note I2C1 핀: PB8 (SCL), PB9 (SDA), 400kHz Fast Mode
 */
int32_t BSP_I2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 레지스터 읽기
 * @param[in] DevAddr 디바이스 주소 (7비트)
 * @param[in] Reg 레지스터 주소
 * @param[out] pData 읽기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Mem_Read() 래퍼 함수, 타임아웃 1000ms
 */
int32_t BSP_I2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 Raw 쓰기 (레지스터 주소 없음)
 * @param[in] DevAddr 디바이스 주소 (7비트)
 * @param[in] pData 쓰기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Master_Transmit() 래퍼 함수
 *          사용처: 커맨드 전송 (레지스터 주소 불필요한 센서)
 */
int32_t BSP_I2C1_Write(uint16_t DevAddr, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 Raw 읽기 (레지스터 주소 없음)
 * @param[in] DevAddr 디바이스 주소 (7비트)
 * @param[out] pData 읽기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Master_Receive() 래퍼 함수
 */
int32_t BSP_I2C1_Read(uint16_t DevAddr, uint8_t *pData, uint16_t Len);

/** @} */ // end of BSP_I2C1

/**
 * @defgroup BSP_I2C3 I2C3 버스 통신 함수 (Upside Board)
 * @{
 */

/**
 * @brief I2C3 레지스터 쓰기
 * @param[in] DevAddr 디바이스 주소 (7비트)
 * @param[in] Reg 레지스터 주소
 * @param[in] pData 쓰기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Mem_Write() 래퍼 함수, 타임아웃 1000ms
 * @note I2C3 핀: PC0 (SCL), PC1 (SDA), 400kHz Fast Mode
 */
int32_t BSP_I2C3_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C3 레지스터 읽기
 * @param[in] DevAddr 디바이스 주소 (7비트)
 * @param[in] Reg 레지스터 주소
 * @param[out] pData 읽기 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details HAL_I2C_Mem_Read() 래퍼 함수, 타임아웃 1000ms
 */
int32_t BSP_I2C3_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/** @} */ // end of BSP_I2C3

/**
 * @defgroup BSP_UART UART 통신 함수
 * @{
 */

/**
 * @brief UART 쓰기 (추상화된 인터페이스)
 * @param[in] pData 전송 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details 현재 구현: UART3 (텔레메트리 전송)
 *          HAL_UART_Transmit() 래퍼 함수
 * @note UART3: PA2 (TX), PA3 (RX), 115200 baud
 */
int32_t BSP_UART_Write(uint8_t *pData, uint16_t Len);

/**
 * @brief UART 읽기 (추상화된 인터페이스)
 * @param[out] pData 수신 데이터 버퍼 포인터
 * @param[in] Len 데이터 길이 (바이트)
 * @return int32_t 0=성공, 음수=HAL 에러 코드
 * @details 현재 구현: UART3 (디버그/텔레메트리 수신)
 *          HAL_UART_Receive() 래퍼 함수
 */
int32_t BSP_UART_Read(uint8_t *pData, uint16_t Len);

/** @} */ // end of BSP_UART

/**
 * @defgroup BSP_TIME 시간 관리 함수
 * @{
 */

/**
 * @brief 시스템 틱 카운트 조회
 * @return uint32_t 시스템 가동 후 경과 시간 (ms)
 * @details HAL_GetTick() 래퍼 함수
 *          SysTick 인터럽트 기반 (1ms 분해능)
 * @note 32비트 오버플로우: 약 49.7일 후 0으로 리셋
 */
uint32_t BSP_GetTick(void);

/**
 * @brief 블로킹 지연
 * @param[in] Delay 지연 시간 (ms)
 * @details HAL_Delay() 래퍼 함수
 *          SysTick 기반 폴링 대기 (인터럽트 허용)
 * @warning 긴 지연은 실시간성 저해 가능, 센서 초기화 시에만 사용 권장
 */
void BSP_Delay(uint32_t Delay);

/** @} */ // end of BSP_TIME

/**
 * @defgroup BSP_ADC ADC 센서 함수
 * @{
 */

/**
 * @brief 배터리 전압 읽기
 * @return uint16_t 배터리 전압 (mV)
 * @details 하드웨어 구성:
 *          - ADC1 채널 2 (PA1)
 *          - 분압비: 6:1 (R1=100kΩ, R2=20kΩ)
 *          - ADC 분해능: 12비트 (0-4095)
 *          - 기준 전압: 3.3V
 *          계산식: Vbat = ADC_raw × (3300mV / 4096) × 6
 * @note App_Loop()에서 50Hz 주기로 호출
 *       저전압 보호: < 2.7V 시 히터 차단
 */
uint16_t BSP_ADC_Read_Battery_mV(void);

/** @} */ // end of BSP_ADC

/**
 * @defgroup BSP_RECOVERY I2C 버스 복구 함수
 * @{
 */

/**
 * @brief I2C1 버스 복구 (9-클럭 펄스)
 * @details 복구 메커니즘:
 *          1. I2C1 주변장치 비활성화 (HAL_I2C_DeInit)
 *          2. SCL/SDA 핀을 GPIO 출력 모드로 전환 (PB8, PB9)
 *          3. SCL 핀에 9개의 클럭 펄스 생성 (100µs 주기)
 *          4. SDA 상태 확인 (High로 복귀 여부)
 *          5. I2C1 주변장치 재활성화 (HAL_I2C_Init)
 *          목적: I2C 슬레이브의 클럭 스트레칭 해제, 버스 행잉 상태 복구
 * @note FDIR_Update()에서 센서 타임아웃 발생 시 자동 호출
 *       I2C Spec (UM10204): 9-클럭 펄스로 슬레이브 리셋 보장
 */
void BSP_I2C1_Recovery(void);

/**
 * @brief I2C3 버스 복구 (9-클럭 펄스)
 * @details I2C1_Recovery와 동일한 메커니즘 (PC0, PC1 핀 사용)
 * @note FDIR_Update()에서 센서 타임아웃 발생 시 자동 호출
 */
void BSP_I2C3_Recovery(void);

/** @} */ // end of BSP_RECOVERY

#ifdef __cplusplus
}
#endif

#endif /* BSP_H_ */
