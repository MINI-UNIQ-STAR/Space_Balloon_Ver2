# main.c 진입점 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Entry Point |
| **코드 라인** | 205줄 |
| **생성** | STM32CubeMX |
| **최종 업데이트** | 2026-01-18 |

## 구조

| 함수 | 설명 |
|------|------|
| `main()` | HAL/Peripheral 초기화, App_Loop() 호출 |
| `SystemClock_Config()` | 170MHz PLL 설정 |
| `HAL_TIM_PeriodElapsedCallback()` | TIM3 SysTick |
| `Error_Handler()` | 무한 루프 |

## 초기화 순서
```
HAL_Init() → SystemClock_Config() → MX_GPIO/I2C/UART/... → App_Init() → while(App_Loop())
```

## 클럭
- **HSI** + **PLL** → 170MHz
- **LSI** → RTC/IWDG

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **CubeMX 준수** | ⭐⭐⭐⭐⭐ | 표준 구조 |
| **App 분리** | ⭐⭐⭐⭐⭐ | `App_Init/Loop` |
| **IWDG** | ⭐⭐⭐⭐⭐ | 워치독 지원 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**STM32CubeMX 표준 구조를 준수하며, `App_Init/Loop` 호출을 통해 비즈니스 로직과 하드웨어 초기화 코드를 명확히 분리함.**
