# minmea NMEA 파서 드라이버 코드 리뷰

| 항목 | 내용 |
|------|------|
| **기능** | NMEA 0183 문장 파서 |
| **출처** | 오픈소스 (Kosma Moczek) |
| **코드 라인** | 681줄 |
| **라이선스** | WTFPL |

## 지원 문장

| 문장 | 함수 | 설명 |
|------|------|------|
| GGA | `minmea_parse_gga()` | Fix 데이터 |
| RMC | `minmea_parse_rmc()` | 최소 권장 |
| GSA | `minmea_parse_gsa()` | DOP/위성 |
| GSV | `minmea_parse_gsv()` | 위성 상세 |
| GLL | `minmea_parse_gll()` | 위치 |
| VTG | `minmea_parse_vtg()` | 속도/방향 |
| ZDA | `minmea_parse_zda()` | 날짜/시간 |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **체크섬** | ⭐⭐⭐⭐⭐ | XOR 검증 |
| **Talker ID** | ⭐⭐⭐⭐⭐ | GP/GL/GA/GB 지원 |
| **가변 인자** | ⭐⭐⭐⭐⭐ | scanf 스타일 |
| **이식성** | ⭐⭐⭐⭐⭐ | 순수 C |

## 주요 API

```c
bool minmea_check(sentence, strict);
enum minmea_sentence_id minmea_sentence_id(sentence, strict);
bool minmea_talker_id(talker[3], sentence);
```

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**성숙한 오픈소스 NMEA 파서.**
