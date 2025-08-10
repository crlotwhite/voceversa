# FFT 라이브러리 비교 리포트

본 문서는 Voceversa(C++17/CMake) 프로젝트 관점에서 실무적으로 자주 쓰이는 FFT 라이브러리들을 조사하여 장단점, 라이선스, 플랫폼/패키징, 성능 특성, 통합 난이도를 정리합니다. macOS(vDSP), CPU(FFTW/KissFFT/PFFFT/PocketFFT/FFTS), GPU(cuFFT), 상용/듀얼 라이선스(KFR)까지 포함합니다.

참고 출처(대표):

- FFTW 공식: [fftw.org][fftw]
- KissFFT: [github.com/mborgerding/kissfft][kissfft]
- PocketFFT: [gitlab.mpcdf.mpg.de/mtr/pocketfft][pocketfft]
- PFFFT: [github.com/marton78/pffft][pffft]
- FFTS: [github.com/anthonix/ffts][ffts]
- Apple Accelerate/vDSP: [developer.apple.com/documentation/accelerate/vdsp][vdsp]
- NVIDIA cuFFT: [docs.nvidia.com/cuda/cufft][cufft]
- KFR: [kfr.dev][kfr]

## 핵심 요약 (권고)

- macOS 전용/우선: Accelerate vDSP 사용 권장. 시스템 프레임워크로 배포 부담이 없고 Apple 실리콘/Intel 맥 모두 최적화가 우수합니다.
- 크로스플랫폼 + 퍼미시브 라이선스: KissFFT 또는 PFFFT가 안전한 기본값. 작고 이식성 좋으며 vcpkg 지원(KissFFT)도 양호.
- 최고 CPU 성능/기능(플래너, 다차원, 실수/코사인/사인, OpenMP/MPI 등): FFTW. 단 GPL(또는 상용 라이선스 구매) 이슈 유의.
- 파이프라인 경량/단순 API가 중요: PocketFFT(단일 헤더/작음) 또는 PFFFT(경량+SIMD) 고려. 패키징은 직접 벤더링이 현실적.
- GPU 가속 필요 시: NVIDIA 플랫폼에서 cuFFT가 사실상 표준. FFTW API 래퍼(cuFFTW) 제공으로 이식성 높음.
- 상용/하이엔드 DSP 프레임워크: KFR는 FFT 포함 폭넓은 DSP를 제공. 라이선스/가격 정책(듀얼 라이선스) 확인 필요.

## 비교 상세

### 1) FFTW 3

- 장점
  - 다양한 차원/형식(복소/실수, DCT/DST 포함), 가변 크기에 대한 높은 성능과 플래너(wisdom) 기반 튜닝.[FFTW]
  - AVX/NEON, OpenMP, MPI 등 광범위한 기능.
  - 풍부한 문서와 실전 사례 다수.
- 단점
  - 라이선스: GPL. 폐쇄/상용 제품엔 부적합. 상용 라이선스는 MIT 통해 별도 구매 필요.
  - 바이너리/의존 크기와 통합 복잡도는 경량 라이브러리 대비 큼.
- 플랫폼/패키징
  - 크로스플랫폼. vcpkg 포트 존재(fftw3).
- 성능/특성
  - 여전히 강력한 CPU 성능과 다양한 크기에서의 안정적 성능. 벤더 BLAS/FFT에 비해 느릴 수 있으나 휴대성 우수.
- 통합 난이도: 중간~상. (플래너/위즈덤 관리 포함)

### 2) KissFFT

- 장점
  - 매우 단순한 API/작은 코드베이스, 퍼미시브(BSD) 라이선스.[KissFFT]
  - float/double/정수 고정소수점까지 지원. 1D/ND/real-optimized, 간단한 FIR/컨볼루션 유틸 포함.
  - vcpkg 포트 존재, 손쉬운 빌드/통합.
- 단점
  - 고성능 벤치에서 FFTW/MKL 대비 느릴 수 있음(단순 설계 지향).[KissFFT]
- 플랫폼/패키징: 크로스플랫폼. CMake 지원.
- 성능/특성: 경량/안정/예측 가능. 실시간 오디오 등에서 충분한 경우 다수.
- 통합 난이도: 낮음.

### 3) PocketFFT

- 장점
  - 가볍고, BSD-3 라이선스. NumPy/SciPy에도 채택된 현대적 구현(템플릿 C++ 버전 존재).[PocketFFT]
  - AVX로 빌드 시 FFTW에 근접한 성능 사례 보고.
  - 단일/소형 코드로 프로젝트 벤더링 용이.
- 단점
  - 패키징(패키지 매니저) 성숙도는 낮아 직접 포함이 현실적.
  - 일부 경로에서 SSE/AVX 설정에 성능 민감.
- 플랫폼/패키징: 크로스플랫폼. 주로 소스 벤더링.
- 통합 난이도: 낮음.

### 4) PFFFT (Pretty Fast FFT)

- 장점
  - BSD-like 라이선스, 경량/단순 API, SSE/AVX/NEON/Altivec SIMD 지원.[PFFFT]
  - 1D 실시간 처리/컨볼루션(PFFASTCONV) 친화. 더블/싱글 지원.
  - 작은 바이너리/낮은 메모리 할당(실시간에 유리).
- 단점
  - 주로 1D에 초점, 범용성(다차원/복합 계획)은 FFTW 대비 제한.
  - SIMD 제약(크기 배수 조건 등) 존재하는 경우가 있음.
- 플랫폼/패키징: 크로스플랫폼. CMake 지원, 필요 시 서브모듈/벤더링.
- 통합 난이도: 낮음.

### 5) FFTS (Fastest Fourier Transform in the South)

- 장점
  - 매우 높은 성능 목표, JIT 코드 생성(런타임) 등 공격적 최적화 아이디어.[FFTS]
  - MIT/유사 퍼미시브로 알려짐(프로젝트/포크별 라이선스 확인 권장).
- 단점
  - 오리지널 레포는 유지보수 중단 공지, 최근 업데이트는 포크에서 진행.
  - 일부 환경/아키텍처 제약 및 빌드 복잡도.
- 플랫폼/패키징: 크로스플랫폼. 포크 선택/검증 필요.
- 통합 난이도: 중간(환경 민감도 높음).

### 6) Apple Accelerate/vDSP (macOS/iOS)

- 장점
  - Apple 공식 시스템 프레임워크. NEON/AMX 등 Apple 실리콘 최적화로 매우 빠름.
  - 배포 부담 낮음(시스템 제공), FFT 외 벡터/신호처리 API 풍부.[vDSP]
- 단점
  - Apple 플랫폼 한정. API가 C/ObjC/Swift 위주.
- 플랫폼/패키징: macOS/iOS 전용. C/C++에서 링크 간단(-framework Accelerate).
- 통합 난이도: 낮음(플랫폼 종속 제외).

### 7) Intel oneMKL DFTI

- 장점
  - 인텔 CPU/벡터화에 매우 강함. FFTW 인터페이스 호환 모드(상호 호환 링크) 제공 사례 있음.
  - oneAPI로 무상 사용 범위 확대(배포/라이선스 정책은 최신 문서 확인 권장).
- 단점
  - 오픈소스 아님. 런타임 의존/배포 고려 필요.
- 플랫폼/패키징: 주로 Windows/Linux x86-64. 패키지/설치 툴로 제공(oneAPI).
- 통합 난이도: 중간.

### 8) NVIDIA cuFFT (GPU)

- 장점
  - NVIDIA GPU에서 고성능 FFT 표준. 1D/2D/3D, R2C/C2R, 배치/스트라이드, 멀티 GPU, 콜백 등 고급 기능.[cuFFT]
  - cuFFTW(FFTW3 API 포팅 툴)로 CPU FFTW 코드의 GPU 마이그레이션 용이.[cuFFT]
- 단점
  - NVIDIA GPU 필요. 초기 플랜/JIT 컴파일 오버헤드 존재(캐시됨).[cuFFT]
- 플랫폼/패키징: CUDA Toolkit 포함. CMake 연동 비교적 단순.
- 통합 난이도: 중간(메모리 배치/플랜 관리 이해 필요).

### 9) Ooura FFT / FFTPACK (레거시/참조)

- 장점
  - 매우 소형, 관대한 라이선스(상업적 사용 허용)로 소스 벤더링 쉬움.[Ooura]
  - 알고리즘적 레퍼런스로 유용.
- 단점
  - 현대 SIMD/멀티코어 최적화 부족. 유지보수 및 기능성 제한.
- 통합 난이도: 낮음(단, 성능 기대치 낮음).

### 10) KFR (상용/오픈소스 듀얼)

- 장점
  - 현대 C++ DSP 프레임워크: FFT, 필터, 리샘플링 등 종합 기능과 높은 성능. 멀티플랫폼/SIMD 폭넓은 지원.[KFR]
- 단점
  - 라이선스/가격 정책 확인 필요(상용 사용 시 구매). OSS 사용 시 적용 가능한 조건 확인 필요.
- 통합 난이도: 중간(프레임워크 학습 포함).

## 프로젝트 적용 가이드 (Voceversa)

- macOS 우선 경로
  - vDSP(Accelerate)로 FFT 구현. CMake에서 플랫폼 분기하여 `-framework Accelerate` 링크. 가장 낮은 지연/최적화 기대.
- 크로스플랫폼 공용 경로
  - 퍼미시브 우선: KissFFT 또는 PFFFT를 서브모듈/벤더링(+선택적 vcpkg)로 통합.
  - 최고 성능이 필요하고 GPL이 허용될 때: FFTW 채택(+vcpkg). 비호환 시 MIT를 통해 FFTW 상용 라이선스 고려.
- GPU 가속(선택 사항)
  - NVIDIA 환경에서 cuFFT로 백엔드 분기. 기존 FFTW 스타일 코드라면 cuFFTW로 마이그레이션 부담 완화.
- API 추상화
  - `FFTWrapper`(인터페이스) → 플랫폼별/백엔드별 어댑터(vDSP/KissFFT/PFFFT/FFTW/cuFFT)로 구현을 분리해 교체 가능성 확보.

## 라이선스/배포 유의사항

- GPL(FFTW)은 프로젝트 전체에 영향. 폐쇄 소스/상용 배포 시 부적합. 필요 시 상용 라이선스 구매.
- 퍼미시브(BSD/MIT 등: KissFFT, PocketFFT, PFFFT, 일부 FFTS 포크)는 통합이 용이.
- 벤더 라이브러리(oneMKL, vDSP, cuFFT)는 각각 런타임/드라이버/플랫폼 의존과 배포 정책 확인 필요.
- KFR은 듀얼 라이선스 성격. 상용 조건/가격 페이지 확인 필수.

## 성능 메모(참고)

- CPU: FFTW는 플래너/위즈덤으로 일관된 고성능을 제공. 최신 인텔에서는 MKL이 더 빠른 경우가 많음(레포트/벤치 사례). PocketFFT(AVX)와 PFFFT는 작고 빠른 경향. KissFFT는 단순하지만 상위권 대비 느릴 수 있음.
- GPU: cuFFT는 크기 소인수(2,3,5,7)에서 최적화가 특히 강하며 배치/스트라이드/콜백/멀티GPU 등 기능이 풍부.
- macOS: vDSP는 Apple 실리콘에서 매우 경쟁력 있는 성능을 보임.

## 빠른 선택 트리

- macOS만? → vDSP.
- 오픈소스, 퍼미시브, 작고 쉬운 통합? → KissFFT 또는 PFFFT.
- 최고급 CPU 성능/풍부한 기능 필요하고 GPL 가능? → FFTW.
- NVIDIA GPU 가속 필요? → cuFFT(필요 시 cuFFTW 인터페이스).
- 기업용 DSP 프레임워크/부가 기능 대거 필요? → KFR(라이선스 확인).

## 부록: 링크

- FFTW: [fftw.org][fftw]
- KissFFT: [github.com/mborgerding/kissfft][kissfft]
- PocketFFT: [gitlab.mpcdf.mpg.de/mtr/pocketfft][pocketfft]
- PFFFT: [github.com/marton78/pffft][pffft]
- FFTS: [github.com/anthonix/ffts][ffts]
- Apple vDSP: [developer.apple.com/documentation/accelerate/vdsp][vdsp]
- Intel oneMKL: [intel.com/oneapi/onemkl][onemkl]
- NVIDIA cuFFT: [docs.nvidia.com/cuda/cufft][cufft]
- KFR: [kfr.dev][kfr]

[fftw]: https://www.fftw.org/
[kissfft]: https://github.com/mborgerding/kissfft
[pocketfft]: https://gitlab.mpcdf.mpg.de/mtr/pocketfft
[pffft]: https://github.com/marton78/pffft
[ffts]: https://github.com/anthonix/ffts
[vdsp]: https://developer.apple.com/documentation/accelerate/vdsp
[cufft]: https://docs.nvidia.com/cuda/cufft/
[kfr]: https://www.kfr.dev/
[onemkl]: https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html

> 참고: 위 링크는 기능/라이선스 확인을 위한 1차 출처입니다. 일부 프로젝트(FFTS 등)는 원본 레포가 비활성화되어 포크 사용이 권장될 수 있습니다. 상용/폐쇄 배포 시에는 각 라이선스 전문과 최신 배포 정책을 반드시 확인하십시오.
