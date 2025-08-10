# UTAU/음성 합성 생태계 경쟁·연계 분석 보고서 (voceversa 관점)

작성일: 2025-08-10
범위: ciglet, libllsm, libllsm2, moresampler, OpenUtau, ENUNU, TuneLab, world4utau, straycat, straycrab
목적: 해당 프로젝트들의 성격과 강점/제약을 정리하고, voceversa 엔진의 차별 포인트와 연계 전략을 제안

## 요약

- 생태계는 크게 세 축으로 구분됨
  - 에디터/호스트: OpenUtau, TuneLab
  - 엔진/리샘플러: moresampler, world4utau, straycat, straycrab, voceversa
  - 알고리즘/라이브러리: ciglet, libllsm, libllsm2
- voceversa의 현재 포지션: C++17·WORLD 기반의 오픈소스 리샘플 엔진. 모듈러 노드/그래프 설계를 통해 분석·합성 파이프라인을 명시적으로 구성. CMake로 리눅스 우선 개발, 윈도우/맥 확장 용이. 테스트와 도구 포함.
- 핵심 차별화 메시지
  - 네이티브 C++ + WORLD, 순수 컴파일드 경로로 저지연·안정적 품질
  - 명시적 그래프/노드 아키텍처로 유지보수성과 실험 민첩성 확보
  - 오픈소스·크로스플랫폼·테스트 내장으로 재현성 강화
  - OpenUtau/TuneLab 연동 로드맵과 깔끔한 플래그/표현식 설계 제공 예정

---

## 항목별 정리

### ciglet (Sleepwalking)

- 성격: C 언어 기반 경량 DSP 유틸리티 집합. FFT, 필터, 보코더 보조, 보간 등 광범위한 신호처리 코드렛 제공
- 용도: libllsm(2), libpyin 등 프로젝트의 공통 기반. 단독 리샘플러가 아닌 하위 레벨 도구
- 라이선스/상태: BSD, 경량·링크 용이. 릴리스 패키지는 없으나 소스 구조 단순
- 시사점(voceversa)
  - 장점 흡수: 일부 보간/윈도/위상 보정 루틴 벤치마킹 가치
  - 대체·연동: 현재 FFTWrapper 등 내부 유틸이 있다면 성능/정확도 비교 벤치마크 후보

### libllsm / libllsm2 (Sleepwalking)

- 성격: 고품질 음성 분석·합성 라이브러리. HNM + 소스-필터 2층(Layer0/1) 모델, 위상 보존 지향
- 특징
  - PbP(Pulse-by-Pulse) 합성 지원, 실시간 합성 API, 고급 잡음 모델링 등
  - WORLD와 유사한 경로도 제공하되, 위상 모델링을 통한 시간영역 손실 최소화 지향
- 제약: GPLv3 라이선스. 상용/비GPL 연계는 제약. 빌드 시 ciglet, libpyin 등 외부 의존
- 시사점(voceversa)
  - 메시지: voceversa는 WORLD 경량 경로로 속도·안정·호환성에 초점. 복잡도·의존성 낮춤
  - 로드맵: PbP 유사 효과나 글로털 파라미터 실험은 “옵션” 모듈로 분리해 GPL 오염 없이 연구 가능

### moresampler (Kanru Hua)

- 성격: 널리 쓰인 고품질 리샘플러. WAVTool도 겸함
- 상태: 공식 배포 경로가 제한적(아카이브/서드파티 링크, BowlRoll). 오픈소스 아님
- 시사점(voceversa)
  - 메시지: 접근성·재현성·장기 유지보수 관점에서 오픈소스 대안 제시
  - 연동: OpenUtau에서 moresampler 호환 플래그 하위셋 맵핑 가이드 제공 가치

### OpenUtau

- 성격: 현대적 UTAU 후속 에디터/호스트. 내장 WORLD 계열 렌더(worldline) 및 외부 리샘플러 인터페이스
- 특징: 표현식(Expressions) 기반 플래그, 사전 프리렌더, 다국어 폰마이저, 리눅스/맥/윈도 지원
- 시사점(voceversa)
  - 연동 1: Resampler Manifest(YAML) 제공으로 플래그 일괄 등록 지원
  - 연동 2: 리샘플러 바이너리 + YAML 배포, pitchbend 등 인자 사양 문서화
  - 연동 3: OpenUtau CI용 샘플 프로젝트·스냅샷 테스트 추가 고려

### ENUNU (NNSVS 기반 UTAU 플러그인)

- 성격: 학습된 NNSVS 모델을 UTAU 워크플로우에서 사용하도록 하는 플러그인
- 시사점(voceversa)
  - 구분 포지션: 신경망 합성 vs 전통 WORLD 기반 리샘플. 경쟁보다는 보완
  - 메시지: voceversa는 저지연, 경량 의존으로 빠른 프리뷰와 일관된 품질. ENUNU 출력과의 파이프라인 연계 가이드 제공 가능

### TuneLab

- 성격: 경량 보컬 합성 에디터. 다중 엔진과 프로젝트 포맷을 플러그인처럼 확장
- 시사점(voceversa)
  - 연동: tlx 확장 패키지 포맷 문서화되어 있어 엔진 플러그인 배포 경로로 적합
  - 메시지: OpenUtau와 병행 지원으로 사용자 저변 확대

### world4utau (xrdavies)

- 성격: WORLD 기반 리샘플러. Linux/Mac 포팅, FFTW 내장 빌드 스크립트 제공
- 시사점(voceversa)
  - 메시지: voceversa는 CMake와 vcpkg로 크로스플랫폼을 기본 제공, 테스트·도구 포함으로 재현성 강화
  - 성능: 동일 WORLD 기반이라 레이턴시·멀티스레딩·I/O 오버헤드 최적화로 차별화 여지

### straycat (Python, 아카이브) / straycrab (Rust)

- 성격: WORLD 기반 구현. straycat은 파이썬 순수 구현으로 호환성 좋으나 속도 한계. straycrab은 Rust 컴파일드 대안
- 시사점(voceversa)
  - 메시지: voceversa는 C++ 네이티브로 로드/실행 오버헤드 최소화, 대용량 렌더·배치에 유리
  - 호환: 주요 플래그/인자 맵핑 표 제공 시 기존 사용자 온보딩 원활

---

## voceversa가 부각할 강점 정리

- 품질·성능
  - WORLD 기반 C++ 네이티브 경로로 저지연 프리뷰와 안정적 합성 품질
  - 파이프라인 단위 최적화 여지(윈도우 선택, STFT 파라미터, 멀티스레드/타일링)
- 아키텍처
  - 명시적 노드/그래프 구조(ComputationGraph, WorldAnalysisNode, WorldSynthesisNode 등)로 테스트와 실험에 강함
  - CLI 유틸(`tools/vv_world`)과 코어 라이브러리 분리로 스크립트/툴링 친화
- 엔지니어링·유지보수
  - CMake 기반, vcpkg.json, 유닛테스트(`tests/test_world.cpp`, `test_core.cpp`)로 재현 가능 빌드·검증
  - 문서화와 예제 폴더 구조로 기여자 온보딩 용이
- 생태계 연계
  - OpenUtau Resampler Manifest(YAML) 제공 계획
  - TuneLab 확장 패키지(tlx) 실험판 제공 고려
  - 기존 플래그(모레/스트레이 시리즈) 하위셋 호환 맵핑 가이드 공개

## 경쟁 항목 대비 메시지 가이드

- moresampler 대비
  - “오픈소스·크로스플랫폼·테스트 내장. 접근성과 재현성을 최우선으로 하는 현대적 대안”
- world4utau 대비
  - “그래프형 설계와 테스트·도구 번들로 팀 개발·실험 민첩성 강화. 빌드·배포 체계화(vcpkg/CMake)”
- straycat/straycrab 대비
  - “네이티브 C++ 최적화로 빠른 로드·합성. 대규모 프로젝트와 배치 렌더에 강함”
- libllsm(2) 대비
  - “고급 위상/글로털 모델 연구는 존중. voceversa는 호환성과 단순 의존 체인을 중시한 경량 경로를 우선”
- OpenUtau·TuneLab과의 관계
  - “호스트와의 긴밀 연동으로 사용 경험 개선: 플래그/표현식 자동 등록, 사양 문서화, 샘플 프로젝트 제공”

## 리스크·제약 및 대응

- WORLD 한계로 인한 특정 음색/발성 표현력 제약
  - 대응: 포스트필터, 대역별 무성/유성 조정, 포먼트 시프팅 등 옵션 제공
- 호환성 이슈(UST/플래그 해석 차이)
  - 대응: 플래그 맵핑 테이블과 회귀 스냅샷 테스트로 안정화
- 성능 벤치마크 부재
  - 대응: 공개 코퍼스 기반의 렌더 타임·SNR·PESQ 유사 지표를 문서화(주관/저작권 이슈 주의) 후 지속 업데이트

## 실행 가능한 로드맵 제안

- 단기
  - OpenUtau Resampler Manifest 초안 작성 및 샘플 플래그 노출
  - CLI 사용 가이드와 예제(음성 샘플, 파이프라인 다이어그램) 추가
  - 간단한 벤치마크 스크립트와 리그(동일 입력·파라미터) 공개
- 중기
  - TuneLab 확장 패키지(tlx) 프로토타입
  - 플래그 호환 맵핑 문서화(moresampler/straycat 하위셋)
  - 멀티스레딩·블록 처리 최적화 옵션 노출
- 장기
  - 선택적 고급 합성 모드(포먼트 보존 개선, 글로털 파라미터 실험)를 별도 모듈로 제공
  - 실시간 미리듣기 경로 최적화 및 에디터 통합 편의 기능

## 참고 출처

- ciglet: [https://github.com/Sleepwalking/ciglet](https://github.com/Sleepwalking/ciglet)
- libllsm2: [https://github.com/Sleepwalking/libllsm2](https://github.com/Sleepwalking/libllsm2)
- libllsm: [https://github.com/Sleepwalking/libllsm](https://github.com/Sleepwalking/libllsm)
- moresampler: 커뮤니티 자료 및 UTAU.info 디렉터리, BowlRoll 아카이브 언급
- OpenUtau: [https://github.com/stakira/OpenUtau](https://github.com/stakira/OpenUtau) (Resamplers and Wavtools 위키)
- ENUNU: [https://github.com/oatsu-gh/ENUNU](https://github.com/oatsu-gh/ENUNU)
- TuneLab: [https://github.com/LiuYunPlayer/TuneLab](https://github.com/LiuYunPlayer/TuneLab)
- world4utau: [https://github.com/xrdavies/world4utau](https://github.com/xrdavies/world4utau)
- straycat: [https://github.com/utautautau/straycat](https://github.com/utautautau/straycat) (아카이브)
- straycrab: [https://github.com/layetri/straycrab](https://github.com/layetri/straycrab)

주: 타 프로젝트의 세부 사양·성능 수치는 공개 문서를 기반으로 요약했으며, 최신 상태는 각 저장소를 참조 바랍니다.
