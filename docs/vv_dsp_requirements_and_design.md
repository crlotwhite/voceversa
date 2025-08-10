# vv-dsp: 경량 DSP 라이브러리 요구사항 및 시스템 디자인

본 문서는 ciglet 등 기존 사례를 참고하여, 본 프로젝트(VoceVersa)의 재사용 가능한 경량 DSP 라이브러리(vv-dsp, 가칭)의 요구사항과 시스템 설계를 정의합니다. 목표는 MIT 호환, 이식성, 실시간 처리에 적합한 기본 DSP 프리미티브를 제공하는 것입니다.

## 배경과 목표

- 재사용성: 음성/노래 합성 파이프라인에서 반복되는 FFT/STFT/윈도/보간/리샘플/필터/위상/켑스트럼/LPC 등을 표준화.
- 가벼움: 외부 의존성 최소, 선택적 백엔드(SIMD/FFT)로 확장 가능.
- 라이선스 안전: MIT(또는 BSD)만 직접 링크. GPL 계열은 플러그인/프로세스 분리.
- 통합 용이성: CMake 타깃 제공(vv::dsp), FetchContent/서브모듈/vcpkg로 손쉬운 통합.

## 범위(Functional)

- 코어 수치/벡터 연산: sum/mean/var, min/max, 복소 헬퍼, 누적합, diff, clamp/denormal flush.
- 윈도우: boxcar, hann, hamming, blackman(-harris), nuttall.
- 스펙트럼: FFT/IFFT 래퍼, STFT/ISTFT, fftshift, 위상 랩/언랩, 위상차.
- 보간/리샘플: 선형/구간 보간(interp1[u]), sinc 기반 업샘플(옵션), polyphase/ratio 리샘플러.
- 필터: FIR(fir1, conv), IIR(biquad) 최소 세트, filtfilt(옵션).
- 켑스트럼/최소위상: rceps/irceps, minphase.
- LPC/스펙트럴 엔벨로프: levinson, lpc, lpspec.
- 신호 프레이밍: fetch_frame/overlap-add, 창/프레임 처리 헬퍼.
- 오디오 I/O: 기본적으로 제외(선택 빌드 옵션). 파이프라인은 외부 I/O에 의존.

비범위(Non-Goals)

- F0 추정/고급 음원-통과 모델(LLSM 등) 포함하지 않음(별도 프로젝트/GPL 분리).
- 대형 DSP 프레임워크 수준의 광범위 기능 제공하지 않음.

## 비기능 요구사항(Non-Functional)

- 언어/표준: C99(필수), 선택적 C++ 래퍼(헤더만) 제공.
- 라이선스: MIT. 제3자 의존성은 MIT/BSD 등 GPL-비오염만 허용.
- 이식성: macOS, Linux, Windows. x86_64, arm64.
- 성능: 48kHz 모노 기준 핵심 연산(FFT/STFT/리샘플/컨볼루션)이 싱글 스레드에서 실시간 처리가 가능해야 함. SIMD 사용 시 추가 가속.
- 정확도: float32 기본, double 선택. 기준 구현 대비 상대/절대 오차 허용: 1e-6(더블), 1e-4(플롯) 수준을 단위 테스트로 검증.
- 스레드 안전: 순수 함수/불변 버퍼 사용 시 재진입 안전. 상태 객체(plan) 변경은 외부 동기화 책임.
- 메모리: 내부 루프에서 동적 할당 금지. plan/buffer 사전 할당 API 제공. 정렬(16/32바이트) 선택.

## API 규약(Contract)

- 네임스페이스/접두사: `vv_dsp_` (C). C++ 얇은 래퍼는 `vv::dsp`.
- 주요 타입
  - `vv_dsp_real`(typedef float/double), `vv_dsp_cpx{ real re, im; }`
  - `vv_dsp_status`(OK, ERR_INVAL, ERR_NOMEM, ERR_BACKEND, ERR_RANGE)
  - 핸들: `vv_dsp_fft_plan*`, `vv_dsp_stft*`, `vv_dsp_resampler*`, `vv_dsp_fir*`, `vv_dsp_biquad*`
- 에러 처리: 정수 상태코드 반환, out-parameter에 결과 작성. 길이는 `size_t`.
- 예시 시그니처
  - FFT: `int vv_dsp_fft_make_plan(size_t n, int dir, vv_dsp_fft_plan** out);`
  - STFT: `int vv_dsp_stft_create(size_t nfft, size_t hop, vv_dsp_window win, vv_dsp_stft** out);`
  - FIR: `int vv_dsp_fir_design_lowpass(float fc, size_t taps, float* out_taps);`
  - 리샘플: `int vv_dsp_resampler_create(double ratio, vv_dsp_resampler** out);`

## 모듈 구조(Architecture)

- core: 타입/상태코드/유틸(denorm, align, math helpers)
- spectral: fft 백엔드(KISS FFT 기본), stft/istft, 위상 처리
- filter: fir/conv, biquad, filtfilt(옵션)
- resample: linear/sinc/polyphase 리샘플러
- envelope: lpc/levinson/lpspec, 켑스트럼/최소위상
- window: 윈도우 생성기
- adapters(cpp): C++ RAII 래퍼, span/strided view

## 빌드/패키징

- CMake 타깃: `vv::dsp`(STATIC/SHARED 선택), 설치/내보내기 지원
- 옵션(ON/OFF)
  - `VV_DSP_BUILD_TESTS`(기본 ON)
  - `VV_DSP_BUILD_BENCH`(기본 OFF)
  - `VV_DSP_FLOAT32`(기본 ON), `VV_DSP_FLOAT64`
  - `VV_DSP_USE_SIMD`(SSE2/AVX2/NEON 자동 감지)
  - `VV_DSP_BACKEND_FFT` = KISS(기본) | FFTS | FFTW(기본 비활성)
  - `VV_DSP_SINGLE_FILE`(amalgamation 생성)
- 통합 방법
  - Git Submodule: `external/vv_dsp` + `add_subdirectory`
  - FetchContent: 프로젝트 내부에서 원격/로컬 참조
  - vcpkg: 포트 추가 후 `vcpkg.json` 의존성 선언(장기)

## 정확도/성능/품질 보증

- 테스트: 단위 테스트(윈도/FFT/STFT/conv/resample/LPC), 수치 허용오차 검증(NumPy/Scipy 참조치 생성)
- 벤치: 마이크로벤치(FFT, STFT, conv, resample). 아키별 결과 비교.
- 정적 분석/산술 경고: UBSan/ASan(옵션), -ffast-math 비권장 기본.

## 플랫폼/에지케이스 정책

- NaN/Inf 전파 규칙 문서화, 입력 검증(길이/정렬/범위)
- Denormal flush-to-zero 활성(옵션), fade-in/out 유틸 제공
- 큰 프레임/긴 컨볼루션 시 overlap-save 지원

## 라이선스/타사 의존성 정책

- 본 라이브러리: MIT
- 직접 링크 허용: MIT/BSD 등 호환 라이선스(KISS FFT 등)
- 직접 링크 금지: GPL/LGPL 의존성(필요 시 별도 프로세스/플러그인으로 연계)

## 이 레포와의 통합 계획

- CMake 옵션 `VOCEVERSA_USE_VV_DSP` 추가(기본 ON)
- 기존 `include/utils/FFTWrapper.h` 등 얇은 어댑터로 단계적 대체
- WORLD 관련 코드는 유지. vv-dsp는 저수준 프리미티브 제공에 집중
- 의존성 주입: `vv::dsp` 미존재 시 기존 구현 경로로 폴백

## 마이그레이션 체크리스트

1) vv-dsp 별도 레포 생성(MIT, CMake 스켈레톤, `vv::dsp` 타깃)
2) 최소 기능(윈도/FFT 래퍼/STFT/선형보간/리샘플/FIR) 구현 + 테스트
3) 본 레포에 서브모듈 추가 + `VOCEVERSA_USE_VV_DSP` 스위치 도입
4) 기존 유틸을 vv-dsp 호출로 점진 교체(모듈 단위)
5) vcpkg 포트 작성(선택), FetchContent 샘플 추가

## 부록: 예시 CMake 연동 스니펫(FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(vv_dsp
  GIT_REPOSITORY https://github.com/your-org/vv-dsp.git
  GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(vv_dsp)
target_link_libraries(voceversa_core PRIVATE vv::dsp)
```

본 설계는 ciglet의 경량/단일파일 전략과 FP 타입 전환 가능성, 그리고 현 레포의 MIT 라이선스를 존중하는 통합 방식을 채택합니다.
