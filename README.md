# Voceversa C++ Project

Cross-platform CMake C++17 skeleton with basic test via CTest.

## Build (macOS/Linux)

```bash
./scripts/build.sh debug
```

or manually:

```bash
cmake --preset debug
cmake --build --preset debug -j
ctest --preset debug --output-on-failure
```

## Build (Windows)

```bat
scripts\build.bat debug
```

## Notes

- Uses C++17 and enables warnings.
- If `fmt` is available via vcpkg/conan, it will be used; otherwise a local stub target satisfies linking.
- Test checks stdout contains `Hello, Voceversa!`.
- asmjit 적용 검토 문서: `docs/asmjit_evaluation.md`
- FFT 라이브러리 비교 리포트: `docs/fft_libraries_report.md`
- 경량 DSP 라이브러리 요구사항/설계: `docs/vv_dsp_requirements_and_design.md`

## Dependencies

- Toolchain
  - CMake >= 3.16
  - C++17-capable compiler (Clang/GCC/MSVC)

- Required
  - fmt (vcpkg port: `fmt`)
    - Already declared in `vcpkg.json`. Falls back to a local stub if not found.

- Optional (enable as needed)
  - WORLD vocoder (mmorise/World) — high-quality analysis/synthesis
    - No official vcpkg port; this project can fetch via CMake `FetchContent` when `VOCEVERSA_USE_WORLD=ON`.
  - FFT library — performance/accuracy
    - `kissfft` or `fftw3` (vcpkg ports: `kissfft`, `fftw3`)
    - macOS alternative: Accelerate/vDSP (system framework)
  - Audio file I/O — broader format support
    - `libsndfile` (vcpkg port: `libsndfile`)
  - Logging
    - `spdlog` (vcpkg port: `spdlog`)
  - Testing framework (currently CTest + asserts)
    - `catch2` or `gtest` (vcpkg ports: `catch2`, `gtest`)
  - JSON handling (for robust metadata I/O)
    - `nlohmann-json` (vcpkg port: `nlohmann-json`) or `rapidjson` (vcpkg port: `rapidjson`)
  - CLI utilities (argument parsing for tools)
    - `cli11` (vcpkg port: `cli11`)
  - Realtime audio I/O (if needed)
    - `portaudio` or `rtaudio` (vcpkg ports: `portaudio`, `rtaudio`)

- macOS notes
  - You can use the system Accelerate framework for FFTs instead of an external port.
  - vcpkg triplet `arm64-osx` works well with the above ports.

## 왜 voceversa인가

voceversa는 에디터가 아닌 “고성능 합성 엔진/SDK”입니다. 기존 에디터(OpenUtau, TuneLab)와 함께 사용하거나, 범용 C-ABI로 임베딩하여 다양한 프런트엔드에서 활용할 수 있습니다.

- 포지션
  - 에디터가 아닌 엔진/SDK. OpenUtau/TuneLab 등에서 resampler/엔진으로 연동하여 사용
  - C++17/CMake 기반 크로스플랫폼(Windows/macOS/Linux)

- 차별점 핵심
  - 멀티 백엔드 파이프라인: WORLD(구현), PSOLA(계획/진행), LLSM2 통계 모델(계획), ONNX 딥러닝 추론(계획)
  - 저지연·고성능: 멀티스레드 그래프 실행(구현), 상주(Server) 모드/캐싱으로 최초·반복 렌더 지연 최소화(계획)
  - 호환성: UTAU resampler/wavtool 어댑터 및 UTAU 아티팩트(f0/sp/ap/meta) 입·출력(계획/진행), 범용 C-ABI 공개 API(계획)
  - 재현 가능한 품질 검증: ABX/MUSHRA-lite, STFT/LSD 등 지표 기반 벤치마크(계획)

- 다른 프로젝트와의 관계
  - OpenUtau/TuneLab: 강력한 “에디터”. voceversa는 이들 환경의 엔진/확장으로 동작하여 품질·지연·확장성을 제공
  - ENUNU(NNSVS 경로): UTAU에서 신경 모델을 쓰는 플러그인. voceversa는 ONNX Runtime 기반 추론으로 유사/대체 경로를 제공 예정
  - WORLD 계열 resampler(world4utau, straycat/straycrab 등): WORLD 단일 경로 중심. voceversa는 WORLD+PSOLA+신경 하이브리드와 서버/캐시·C-ABI로 차별화

- 로드맵 스냅샷(요약)
  - UTAU 어댑터·아티팩트 호환 강화 → 에디터 통합(A/B 렌더)
  - ONNX 최소 경로(단일 모델 로드·추론) → 스트리밍 프리뷰
  - PSOLA 품질 강화(포만트/트랜지언트 보존)
  - TuneLab .tlx 확장 배포
  - 공용 벤치마크/리포트 공개(지연·품질·자원 사용)

상세 비교/벤치마크는 추후 docs에 리포트로 공개될 예정입니다.
