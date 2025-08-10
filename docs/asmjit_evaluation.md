# asmjit 적용 검토 보고서

작성일: 2025-08-10

## TL;DR

- asmjit은 x86/x64 및 AArch64(ARM64)를 지원하는 경량 JIT 코드 생성 라이브러리로, zlib 라이선스라 상용 포함 제약이 거의 없습니다.
- voceversa의 DSP 파이프라인(윈도잉·보간·오버랩-애드·간단 합성 커널 등) 일부를 런타임 파라미터에 맞춰 JIT로 특화(fusing, 언롤링, 벡터 ISA 선택)하면 이득 가능성이 있습니다.
- macOS(특히 Apple Silicon)에서는 W^X, MAP_JIT, pthread_jit_write_protect_np 등 JIT 제약을 준수해야 합니다. asmjit의 JitAllocator/VirtMem가 이를 추상화합니다.
- 첫 단계로 선택적(옵션) 통합과 마이크로벤치/ABX 품질 유지 확인을 병행하는 것이 안전합니다.

참고 근거:

- 공식 홈페이지: [asmjit.com](https://asmjit.com/) (X86/X64, AArch64 지원, zlib 라이선스)
- VirtMem 문서: <https://asmjit.com/doc/namespaceasmjit_1_1VirtMem.html> (MAP_JIT, W^X, 듀얼 매핑, icache flush, protectJitMemory)
- Apple Hardened Runtime: <https://developer.apple.com/documentation/security/hardened_runtime>

---

## 왜 asmjit인가(프로젝트 맥락)

- 현재 코드베이스는 WORLD 기반 분석/합성(`src/world/*`)과 오디오 유틸(`include/utils`, `src/utils`) 위주로 CPU 루프가 많습니다.
- 컴파일 타임에 최적화하기 어려운 “런타임 파라미터(프레임 크기, 피치, 윈도우 길이, 보간 계수, 채널 수, ISA 가용성 등)”가 다양합니다.
- asmjit로 “그때의 파라미터와 CPU 특성(AVX2/AVX-512, NEON 등)”에 맞는 커널을 즉시 생성하면, 다음을 노릴 수 있습니다.
  - 루프 언롤·상수 폴딩·계수 프리베이크(예: 윈도 함수, 보간 계수)
  - 커널 퓨전(windowing + gain + clamp + overlap-add)
  - ISA별 벡터 경로 선택 및 레지스터 할당 최적화

주의: FFT/블록 변환은 벤더/시스템 라이브러리(Accelerate, FFTW)가 이미 강력하므로 JIT 대체는 비효율일 수 있습니다. 대신 그 전후 스칼라/가벡터화 가능한 커널이 1차 후보입니다.

### 1차 후보 커널

- 창함수 적용 + 프리필터(gain/tilt) + 클램프/소프트리밋
- 오버랩-애드(OLA) 및 블렌딩(파셜 합성 후 mix)
- 간단한 FIR/선형 보간(샘플레이트 변환 전처리 경로)
- 에너지 합산/통계(분석 단계의 반복 루프)

---

## 지원 아키텍처·플랫폼

- X86/X64: MMX, SSE+, AVX+, FMA+, AVX-512+, AMX 등(asmjit 문서 기준 “complete x86/x64 ISA”).
- AArch64(ARM64): 베이스라인 + ASIMD(NEON) 확장. 최신 확장 일부는 미지원일 수 있음(공식 사이트 언급).
- 라이선스: zlib(정적/동적 링크 모두 문제 없음). 코드 스니펫 예시는 Unlicense.

### macOS(특히 Apple Silicon) 제약 요약

- W^X 정책: RWX 동시 허용 불가 환경에서 듀얼 매핑(RW/RX 분리) 또는 MAP_JIT + 스레드 단위 write-protect 토글 필요.
- MAP_JIT: Apple 플랫폼에서 JIT 코드 실행을 위해 필요할 수 있으며, asmjit JitAllocator가 자동 처리.
- pthread_jit_write_protect_np: AArch64에서 쓰기/실행 전환에 사용. asmjit의 `VirtMem::protectJitMemory()` 또는 `ProtectJitReadWriteScope` 사용 권장.
- 하든드 런타임/노타라이즈 앱: 배포 시 entitlements 필요 가능. CLI/개발 환경에서는 대개 문제 없음이나, 샌드박스/스토어 배포 형태는 별도 검토 필요.

---

## 통합 전략(안전한 단계적 접근)

1. 옵트인 빌드 옵션

- CMake 옵션: `VOCEVERSA_USE_ASMJIT` (기본 OFF)
- OFF: 기존 경로 유지. ON: asmjit 의존 + JIT 경로 병행 제공.
- Apple/ARM64와 Windows/Linux 모두에서 컴파일만 되도록 준비(실행 시 JIT 불가 상황 graceful fallback).

1. 의존성 획득

- 권장: 서브모듈 또는 FetchContent로 최신 master 사용(공식이 vcpkg 패키지 구버전 경고).
- 정적 링크 시 `ASMJIT_STATIC` 정의 필요(문서 참고).

1. JIT 런타임 관리

- 프로세스 전역 또는 모듈 전용 `asmjit::JitRuntime` 보유.
- 커널 캐시: (키) 파라미터/ISA → (값) 함수 포인터. LRU 또는 단순 매핑으로 메모리 관리.
- Apple ARM64: 수정 전 `ProtectJitReadWriteScope`로 RW 전환, 수정 후 RX 복귀 + `flushInstructionCache` 호출.

1. API 스케치

- `include/core/JitKernels.h` / `src/core/JitKernels.cpp`
  - `struct KernelKey { size_t N; ISA isa; Op op; /* flags... */ };`
  - `using KernelFn = void(*)(const float* in, float* out, size_t n, const void* params);`
  - `KernelFn getOrBuild(const KernelKey&, const KernelParams&);`
- `KernelParams`는 윈도 계수/스칼라 상수/스텝 등을 포함.

1. 실패·폴백 시나리오

- JIT 메모리 불가 또는 권한 오류: 로깅 후 네이티브 C++ 경로로 폴백.
- ISA 미지원/감지 실패: 스칼라/기존 SIMD 경로 사용.

---

## CMake 통합 가이드(제안)

- 옵션
  - `option(VOCEVERSA_USE_ASMJIT "Enable asmjit JIT kernels" OFF)`
- FetchContent(예시)
  - 최신 권장: `FetchContent_Declare(asmjit GIT_REPOSITORY https://github.com/asmjit/asmjit.git GIT_TAG master)`
  - `FetchContent_MakeAvailable(asmjit)` 후 `target_link_libraries(voceversa_core PRIVATE asmjit::asmjit)`
- 정의
  - 정적 링크 시: `target_compile_definitions(voceversa_core PRIVATE ASMJIT_STATIC)`
  - 이식성 가드: `VOCEVERSA_USE_ASMJIT`, `VOCEVERSA_ASMJIT_ENABLED_AT_RUNTIME`

주의: vcpkg 패키지는 구버전일 수 있으므로 우선은 vendoring/FetchContent 권장(공식 사이트 안내).

---

## POC 범위(1~2주)

- 대상 커널: “윈도잉 + 게인 + 클램프 + 오버랩-애드” 단일 패스 JIT.
- 타깃 ISA: x86 AVX2 우선, AArch64 NEON은 두 번째 단계.
- 입력 크기: 256/512/1024/2048 샘플 프레임 반복.
- 정확성 검증: 기존 경로와 샘플별 max-abs-error, RMS-Error 비교(허용 오차 1e-6~1e-5).
- 성능: 1) cold compile 시간(ms), 2) steady-state ns/sample, 3) 스레드 수 1/실사용 설정.
- 실패 처리: JIT 불가 시 자동 폴백되고 벤치마크에 기록.

마이크로벤치 툴 제안:

- `tools/bench_jit_kernels.cpp` 에서 Google Benchmark 또는 간단 커스텀 타이머 사용.
- CSV/JSON 출력으로 크기·ISA·스레드·지표 기록.

---

## 품질/안전 고려

- W^X 준수: Apple ARM64에서 `ProtectJitReadWriteScope` 사용 + icache flush.
- 스레드 안전: 커널 캐시 락 또는 컴파일-한정 싱글라이터/멀티리더 전략.
- 메모리 수명: `JitRuntime`가 함수 포인터의 생명주기를 관리. 종료 시 release.
- 재현성: 동일 파라미터에서 동일 코드 생성 보장(디버그 로깅 Hash/Key 출력).
- 로깅: 빌드 실패/권한 예외 시 사용자 가시성 있는 경고.

---

## 리스크와 완화

- macOS 배포(노타라이즈/샌드박스) 시 JIT 권한 이슈 → 런타임 디텍션 + 자동 폴백. 배포 타깃에서 JIT off 기본.
- 유지보수 비용(ISA별 코드 생성기) → 2~3개 핵심 커널부터 축소 적용. 코드 생성은 템플릿화/유닛테스트.
- 성능 기대 미충족 → 벤치 결과로 Go/No-Go 결정. 이득이 없는 구간은 삭제.

---

## 결정 게이트(권장)

1) POC 성능 임계치: 기존 대비 x1.2~x1.5 이상 가속(steady-state) + cold compile amortization 1초 내 수익분기 달성.

2) 정확성: float 경로에서 기존과 수치 오차 허용 범위 내 유지.

3) 운영: macOS/Windows/Linux에서 최소 문제로 빌드/실행 가능, 문제 시 폴백 확인.

---

## 다음 단계 체크리스트

- [ ] CMake 옵션 및 FetchContent 스켈레톤 추가(옵트인)
- [ ] `JitKernels` 모듈 생성 + JitRuntime/캐시 골격
- [ ] 윈도잉+게인+클램프+OLA 커널 AVX2 POC
- [ ] 마이크로벤치 및 정확성 테스트 추가
- [ ] macOS ARM64 경로에서 MAP_JIT/Protect 스코프/flush 확인 테스트
- [ ] 결과 리뷰 후 확대/축소 결정

---

## 부록: 코드 스케치(참고)
```cpp
// JitKernels.h
struct KernelKey { size_t n; int op; int isa; uint32_t flags; };
using KernelFn = void(*)(const float*, float*, size_t, const void*);
KernelFn getOrBuild(const KernelKey&, const void* params);
```
```cpp
// 사용 예
if (VOCEVERSA_USE_ASMJIT) {
  auto fn = getOrBuild(key, &params);
  if (fn) { fn(in, out, n, &params); } else { fallback(in, out, n, params); }
} else {
  fallback(in, out, n, params);
}
```

---

## 참고 링크

- AsmJit Home: [asmjit.com](https://asmjit.com/)
- AsmJit VirtMem (MAP_JIT / W^X / flush): <https://asmjit.com/doc/namespaceasmjit_1_1VirtMem.html>
- Apple Hardened Runtime: <https://developer.apple.com/documentation/security/hardened_runtime>
