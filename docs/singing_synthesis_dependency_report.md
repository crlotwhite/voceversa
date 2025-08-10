# 합성 엔진 의존성 리서치 리포트 (ciglet, llsm/llsm2, moresampler, OpenUtau, ENUNU, TuneLab, world4utau, straycat/straycrab)

작성일: 2025-08-10

본 문서는 UTAU/OpenUtau 생태계 및 관련 합성/리샘플러 도구의 의존성 라이브러리를 조사하여, 유사한 합성 엔진을 개발·유지보수할 때 필요한 스택을 정리한 자료입니다. 각 항목은 공식 README, manifest, requirements 등을 기반으로 요약했습니다(출처 링크 포함).

---

## 1) ciglet (Lightweight DSP library)

- 언어/빌드: C99, Makefile. BSD 라이선스
- 핵심 기능: FFT/CZT/DCT, STFT/ISTFT, 윈도/위상/보간/필터링, LPC, 스펙트럼 엔벨로프, 음원모델(LF), 오디오 I/O(wavread/wavwrite)
- 의존성: 외부 필수 의존성 없음(자급식). 플로팅 포인트 타입은 매크로로 제어(FP_TYPE)
- 비고: libllsm/llsm2, moresampler 등에서 공용 신호처리 루틴을 분리해 만든 경량 DSP 킷
- 소스: [Sleepwalking/ciglet](https://github.com/Sleepwalking/ciglet)

## 2) libllsm (구버전) / 3) libllsm2 (현행)

- 언어/빌드: C, Makefile. GPL-3.0
- 목적: 고품질 음성 분석·수정·합성(2계층 LLSM: HNM 기반 layer0 + source-filter 해석 layer1, PbP 합성 등)
- 필수 의존성:
  - ciglet (필수) [llsm2 README ‘Compiling’ 명시]
  - F0 추정 외부 라이브러리 필요(분석 단계): 예) libpyin, Nebula (선호·예시)
- 테스트/도구 의존성(옵션):
  - libpyin, libgvps (테스트용 심볼릭 링크)
  - doxygen (문서화)
- 빌드 팁: FP_TYPE 매크로(float/double) 컴파일 플래그로 지정 필요(-DFP_TYPE=float 등)
- 소스: [Sleepwalking/libllsm](https://github.com/Sleepwalking/libllsm), [Sleepwalking/libllsm2](https://github.com/Sleepwalking/libllsm2)

## 4) moresampler

- 개요: UTAU용 커스텀 리샘플러(제작자 Kanru Hua). 공개 소스 저장소는 공식적으로 제공되지 않음(바이너리 배포 중심)
- 개발 의존성(추정): ciglet 기반의 공통 DSP 루틴 활용 이력 언급. 내부적으로 WORLD/LLSM 류의 해석·합성 파이프라인과 F0 추정기가 필요했을 가능성
- 사용자 런타임 의존성: 별도 런타임 설치 없이 실행 가능한 단일 바이너리 배포 케이스가 일반적
- 참고: ciglet README의 배경 섹션에 moresampler가 개발 동기로 언급됨
- 정보 출처: [VocaDB 항목](https://vocadb.net/T/9775) (개요), [ciglet README](https://github.com/Sleepwalking/ciglet) (배경)

## 5) OpenUtau (에디터/호스트)

- 언어/빌드: C# (.NET), 다중 플랫폼 지원(Windows/macOS/Linux). MIT 라이선스
- 핵심 NuGet 패키지(발췌):
  - 오디오/코덱/미디: NAudio.Core/NAudio(Win), NAudio.Vorbis, NLayer.NAudioSupport, BunLabs.NAudio.Flac, Melanchall.DryWetMidi
  - 신호처리/수치: NWaves, NumSharp
  - 모델 추론: Microsoft.ML.OnnxRuntime (Win: DirectML 변형), 비Windows: OnnxRuntime
  - 기타: Newtonsoft.Json, YamlDotNet, SharpCompress, Serilog, System.* 보조 패키지, NetMQ(IPC), UTF.Unknown, K4os.Hash.xxHash, Ignore 등
- 플랫폼 분기:
  - Windows: NAudio, Microsoft.ML.OnnxRuntime.DirectML
  - 비Windows: Microsoft.ML.OnnxRuntime
- 소스: [stakira/OpenUtau](https://github.com/stakira/OpenUtau) (OpenUtau.Core.csproj 참조)

## 6) ENUNU (UTAU 플러그인, NNSVS 기반 AI 가창)

- 언어/빌드: Python 3.8+ (Windows 지향), MIT 라이선스
- requirements.txt (주요):
  - torch, nnsvs, nnmnkwii, numpy(>=1.20), scipy, hydra-core(<1.1), omegaconf, joblib, tqdm, utaupy(>=1.10), pyyaml
  - CUDA 11.3(옵션, GPU 가속)
- 소스: [oatsu-gh/ENUNU](https://github.com/oatsu-gh/ENUNU) (requirements.txt)

## 7) TuneLab (경량 가창 에디터)

- 언어/빌드: C# (.NET 8 / Avalonia), MIT 라이선스
- 핵심 NuGet 패키지(발췌):
  - UI: Avalonia.* (Desktop/Fluent/Fonts/ReactiveUI/Svg.Skia), Markdown.Avalonia
  - 오디오/입출력: NAudio, BunLabs.NAudio.Flac, NLayer.NAudioSupport
  - 기타: Flurl.Http, Newtonsoft.Json, System.Net.Http, System.Text.RegularExpressions, Microsoft.Extensions.ObjectPool, Tomlyn(TOML), ppy.SDL2-CS, ZstdSharp.Port 등
- 소스: [LiuYunPlayer/TuneLab](https://github.com/LiuYunPlayer/TuneLab) (TuneLab.csproj)

## 8) world4utau (WORLD 기반 리샘플러)

- 언어/빌드: C, Makefile. GPL-3.0
- 핵심 의존성:
  - FFTW 3.x (repo에 3.3.8 소스 포함/혹은 시스템 설치)
  - 컴파일러/툴체인: gcc/clang, Unix 툴체인(Xcode CLT on macOS)
  - WORLD 관련 코드 포함/포팅
- 소스: [xrdavies/world4utau](https://github.com/xrdavies/world4utau)

## 9) straycat (Python 구현 WORLD 리샘플러)

- 언어/빌드: Python 3.10.11 기준
- 파이썬 패키지: numpy, scipy, resampy, pyworld
- 서버 변형(Windows): libcurl.dll 필요(런처에서 사용), 실행용 exe 래퍼 제공
- 소스: [UtaUtaUtau/straycat](https://github.com/UtaUtaUtau/straycat)

## 10) straycrab (Rust 구현 리샘플러, straycat에서 영감)

- 언어/빌드: Rust (Cargo)
- Cargo.toml 주요 의존성:
  - WORLD 바인딩: rsworld, rsworld-sys
  - 신호/수치: ndarray, num-traits, csaps(스무딩), makima_spline(보간)
  - I/O/유틸: hound(WAV), bincode, serde(derive), regex, tqdm, dotenv, zstd, log, npyz(npz 지원)
- 소스: [layetri/straycrab](https://github.com/layetri/straycrab)

---

## 교차 의존성 테마/권장 스택

- Vocoder/합성 코어
  - WORLD 계열: WORLD(C/C++), pyworld(Python), rsworld(Rust 바인딩)
  - LLSM 계열: libllsm2(+ ciglet), F0 추정기(libpyin 등) 연계

- DSP/FFT/신호처리
  - 경량 C DSP: ciglet
  - FFT: FFTW(세계적으로 보편, GPL/LGPL 고려), 대안으로 KissFFT/FFTS/Accelerate(apple)/IPP(intel)
  - Python: numpy/scipy/resampy, Rust: ndarray + 전용 크레이트

- ML 추론/가창 모델
  - ONNX Runtime(DirectML/CPU): 에디터/호스트 측(OpenUtau 등)
  - PyTorch: 학습·런타임(ENUNU/NNSVS)

- 오디오 I/O/코덱/미디
  - C/C++: libsndfile, dr_wav, minimp3 등(본 리포지토리에는 자체 WavIO 구현 존재)
  - .NET: NAudio(+Vorbis/NLayer/Flac), DryWetMIDI
  - Python: soundfile(h5py 조합) 또는 hound(러스트)

- 앱/플러그인 호스트(UI)
  - .NET UI: Avalonia(크로스), WPF/WinUI(윈도우), Serilog/SharpCompress/JSON/YAML
  - IPC/런처: NetMQ, HTTP/CLI 래퍼, zstd 캐싱

---

## 이 리포지토리(voceversa)에서 유사 엔진을 구현할 때의 권장 의존성

현재 프로젝트는 CMake/CPP, WORLD 파이프라인과 자체 `WavIO`, `FFTWrapper` 등을 포함합니다. 다음을 권장합니다:

- 필수(코어)
  - WORLD: 이미 `src/world`에 포함되어 있음(모듈별 D4C/F0/Spectral 참고)
  - F0 추정기: libpyin(선호) 또는 대체 구현. llsm2와의 연계 고려 시 libpyin 강추
  - DSP 유틸: ciglet(경량 공용 루틴 활용 시 유리). 대안으로 현재 `FFTWrapper` 유지

- 선택(성능/편의)
  - FFTW3: 대용량/고정밀 분석 시(현재 FFT 구현과 상호 비교 필요)
  - libsndfile: 다양한 포맷 입출력(현재 `WavIO`는 WAV 중심)
  - zstd: 프리렌더/캐시 가속
  - CLI/설정: YAML/JSON 파서(예: yaml-cpp, nlohmann/json)

- 빌드/패키징(이식성)
  - vcpkg로 패키지 고정(fftw3, libsndfile, yaml-cpp, zstd 등)
  - CMake 옵션으로 WORLD/LLSM 경로/플래그 제어(FP_TYPE 등)

---

## 참고 링크(주요 출처)

- ciglet: [Sleepwalking/ciglet](https://github.com/Sleepwalking/ciglet)
- libllsm2: [Sleepwalking/libllsm2](https://github.com/Sleepwalking/libllsm2) (Compiling 섹션에 ciglet/libpyin/libgvps 언급)
- libllsm: [Sleepwalking/libllsm](https://github.com/Sleepwalking/libllsm) (Compiling 섹션)
- OpenUtau: [stakira/OpenUtau](https://github.com/stakira/OpenUtau) (OpenUtau.Core.csproj의 PackageReference)
- ENUNU: [oatsu-gh/ENUNU](https://github.com/oatsu-gh/ENUNU) (requirements.txt)
- TuneLab: [LiuYunPlayer/TuneLab](https://github.com/LiuYunPlayer/TuneLab) (TuneLab.csproj)
- world4utau: [xrdavies/world4utau](https://github.com/xrdavies/world4utau) (README, FFTW 빌드 지시)
- straycat: [UtaUtaUtau/straycat](https://github.com/UtaUtaUtau/straycat) (README의 pip 패키지 목록)
- straycrab: [layetri/straycrab](https://github.com/layetri/straycrab) (Cargo.toml 의존성)

---

## 요약 체크리스트

- ciglet/llsm/llsm2: C 기반 신호처리 + F0(libpyin) + FP_TYPE 매크로 설정 필요
- moresampler: 소스 비공개(개발 의존성 공개 정보 제한), 실행 바이너리 중심
- OpenUtau/TuneLab: .NET + NAudio/NWaves/ONNX Runtime 등
- ENUNU: Python + Torch + NNSVS + utaupy
- WORLD 계열 리샘플러: FFTW(또는 대체 FFT), WORLD/pyworld/rsworld
- 본 레포 권장: WORLD + libpyin + (옵션) FFTW/libsndfile/zstd + vcpkg 통합
