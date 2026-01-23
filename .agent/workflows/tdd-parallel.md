---
description: TDD 방식으로 테스트와 구현을 병렬 진행하는 워크플로우
---

# TDD 병렬 개발 워크플로우

이 워크플로우는 테스트 주도 개발을 병렬로 수행합니다.

---

## ⚠️ 필수 사전 단계 (반드시 수행)

### Step 0: 정보 확인 및 질문

**시작 전 반드시 확인해야 할 정보:**

| 필수 정보 | 확인 여부 | 부족 시 질문 예시 |
|-----------|----------|------------------|
| 개발할 기능 명세 | [ ] | "어떤 기능을 개발해야 하나요?" |
| 입력/출력 스펙 | [ ] | "함수의 입력과 예상 출력을 알려주세요" |
| 에러 케이스 | [ ] | "어떤 에러 상황을 처리해야 하나요?" |
| 테스트 프레임워크 | [ ] | "사용할 테스트 프레임워크가 있나요? (Unity, Criterion, etc.)" |
| 커버리지 목표 | [ ] | "테스트 커버리지 목표가 있나요?" |
| 경계값 조건 | [ ] | "입력값의 유효 범위가 어떻게 되나요?" |

> [!IMPORTANT]
> **정보가 부족하면 절대 추측하지 말고 사용자에게 질문하세요!**
> TDD에서 테스트 케이스 설계는 명확한 스펙이 있어야만 가능합니다.

**질문 템플릿:**
```
TDD 개발을 시작하기 전에 몇 가지 확인이 필요합니다:

1. [누락된 정보 1]?
2. [누락된 정보 2]?
3. [누락된 정보 3]?

위 정보를 제공해 주시면 테스트 케이스와 구현 계획을 수립하겠습니다.
```

---

### Step 1: 계획 수립 (implementation_plan.md 작성)

> [!CAUTION]
> **계획 없이 코딩을 시작하지 마세요!**
> 반드시 `implementation_plan.md`를 작성하고 사용자 승인을 받은 후 진행합니다.

**계획서에 포함할 내용:**

```markdown
# TDD 개발 계획

## 1. 개요
- 기능: [개발할 기능 설명]
- 목적: [기능의 목적]

## 2. 기능 스펙

### 입력
| 파라미터 | 타입 | 유효 범위 | 설명 |
|---------|------|----------|------|
| input_a | i32 | 0-100 | ... |

### 출력
| 반환값 | 타입 | 설명 |
|--------|------|------|
| result | Result<T, E> | ... |

### 에러 케이스
| 에러 | 조건 | 처리 |
|------|------|------|
| InvalidInput | input < 0 | Err 반환 |

## 3. 테스트 케이스 목록

| TC ID | 분류 | 설명 | 우선순위 |
|-------|------|------|---------|
| TC-001 | Happy Path | 정상 입력 처리 | P1 |
| TC-002 | Boundary | 최소값 경계 | P1 |
| TC-003 | Error | 잘못된 입력 | P1 |

## 4. 구현 계획
1. 테스트 케이스 TC-001~003 작성 (RED)
2. 최소 구현 (GREEN)
3. 리팩토링 (REFACTOR)
4. 추가 테스트 케이스 작성
5. 반복

## 5. 커버리지 목표
- 라인 커버리지: 80% 이상
- 브랜치 커버리지: 70% 이상
```

**사용자 승인 요청:**
```
계획서를 검토해 주세요. 테스트 케이스가 적절한지 확인 후 승인해 주시면 개발을 시작하겠습니다.
```

---

## 역할 분담

| 역할 | 담당 작업 |
|------|----------|
| **테스트 에이전트** | 테스트 케이스 작성, 검증 |
| **구현 에이전트** | 테스트 통과하는 코드 구현 |

---

## TDD 사이클

```
┌─────────────────────────────────────────────┐
│                   RED                       │
│   테스트 에이전트: 실패하는 테스트 작성      │
└─────────────────────┬───────────────────────┘
                      ▼
┌─────────────────────────────────────────────┐
│                  GREEN                      │
│   구현 에이전트: 테스트 통과하는 코드 작성   │
└─────────────────────┬───────────────────────┘
                      ▼
┌─────────────────────────────────────────────┐
│                REFACTOR                     │
│   양쪽 협력: 코드 개선 및 정리              │
└─────────────────────┬───────────────────────┘
                      │
                      └──────────> 반복
```

---

## 실행 단계 (계획 승인 후)

### Step 2: 테스트 케이스 설계
테스트 에이전트가 테스트 케이스를 설계합니다:

**테스트 우선순위**:
1. **Happy Path**: 정상 동작 케이스
2. **Boundary**: 경계값 테스트
3. **Error Cases**: 에러 핸들링
4. **Edge Cases**: 엣지 케이스
5. **Performance**: 성능 테스트 (임베디드)

### Step 3: RED - 테스트 작성
// turbo
테스트 에이전트가 실패하는 테스트를 먼저 작성합니다:

```rust
// Rust 예시
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sensor_read_valid_range() {
        let sensor = Sensor::new();
        let value = sensor.read();
        assert!(value >= 0.0 && value <= 100.0);
    }

    #[test]
    fn test_sensor_error_on_disconnect() {
        let sensor = Sensor::new();
        sensor.disconnect();
        assert!(sensor.read().is_err());
    }
}
```

### Step 4: GREEN - 구현
구현 에이전트가 테스트를 통과하는 최소한의 코드를 작성합니다.

```rust
impl Sensor {
    pub fn read(&self) -> Result<f32, SensorError> {
        if !self.connected {
            return Err(SensorError::Disconnected);
        }
        // 최소 구현
        Ok(self.raw_read().clamp(0.0, 100.0))
    }
}
```

### Step 5: 테스트 실행
// turbo
```bash
# Rust
cargo test

# C/C++ with Unity
make test

# Python
pytest -v
```

### Step 6: REFACTOR - 리팩토링
테스트가 통과하면 코드를 개선합니다:
- 중복 제거
- 가독성 향상
- 성능 최적화

### Step 7: 다음 기능으로 반복
모든 테스트가 통과하면 다음 기능으로 이동합니다.

---

## 테스트 케이스 템플릿

```markdown
## 테스트 케이스: [기능명]

### TC-001: [테스트 이름]
- **설명**: [테스트 목적]
- **입력**: [입력값]
- **예상 출력**: [예상 결과]
- **우선순위**: High/Medium/Low

### TC-002: [테스트 이름]
...
```

---

## 임베디드 특화 테스트

### 하드웨어 모킹
```rust
// Mock trait for hardware abstraction
#[cfg(test)]
mod tests {
    use super::*;
    
    struct MockGpio {
        state: bool,
    }
    
    impl GpioPin for MockGpio {
        fn set_high(&mut self) { self.state = true; }
        fn set_low(&mut self) { self.state = false; }
        fn is_high(&self) -> bool { self.state }
    }
}
```

### 타이밍 테스트
```rust
#[test]
fn test_response_time() {
    let start = Instant::now();
    process_data(&input);
    let elapsed = start.elapsed();
    assert!(elapsed < Duration::from_micros(100));
}
```

---

## 체크리스트

### 사전 단계
- [ ] 모든 필수 정보 확인됨
- [ ] implementation_plan.md 작성됨
- [ ] 테스트 케이스 목록 작성됨
- [ ] 사용자 승인 받음

### 완료 조건
- [ ] 모든 테스트 케이스 통과
- [ ] 코드 커버리지 목표 달성
- [ ] 경계값 테스트 완료
- [ ] 에러 핸들링 테스트 완료
- [ ] 리팩토링 완료
