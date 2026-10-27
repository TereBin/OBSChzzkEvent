# OBS Chzzk Alerts

치지직 스트리머 스튜디오 리모컨의 팔로우 이벤트를 OBS 전용 알림 소스로 전달하는 Windows용 OBS 플러그인입니다.

일반 사용자를 위한 설치 파일과 사용 설명서는 [배포 저장소](https://github.com/TereBin/obs-chzzk-alerts-releases)에서 제공합니다. 이 저장소는 소스 코드, 프로토콜 구조와 개발 문서를 관리합니다.

- 배포 패키지: [OBS Chzzk Alerts Releases](https://github.com/TereBin/obs-chzzk-alerts-releases/releases/latest)
- 소스 저장소: [OBS Chzzk Alerts](https://github.com/TereBin/obs-chzzk-alerts)

## 0.5.0 변경 사항

현재 소스 버전은 `0.5.0`, 중계 확장 프로그램은 `0.2.2`입니다. 공개 설치 파일의 버전은 [배포 저장소의 릴리스 페이지](https://github.com/TereBin/obs-chzzk-alerts-releases/releases)에서 확인할 수 있습니다.

- 기본 알림음 3종을 동봉하고 기존 사용자 음원 선택을 유지합니다.
- 소스 크기 조회를 원자적 스냅샷으로 분리해 설정 변경 중 동기 재진입으로 발생하던 종료를 수정했습니다.
- 선택된 FFmpeg 음원의 표시 참조와 오디오 캡처 수명을 관리해 부모 소스의 믹서·모니터링으로 소리가 전달되도록 수정했습니다.
- 설정·대기열·화면 자원·HTTP 요청 처리를 분리하고 OBS 참조를 자동 해제하도록 정리했습니다.
- 브라우저의 중복 기록을 제한하고 연결 실패 시 재시도를 허용하며, 닉네임의 공백·더하기 기호를 구분해 전달합니다.
- 상태·오디오·HTTP·브라우저 회귀 테스트를 추가하고 Windows CI에서 실행합니다.

## 기능

- 치지직 리모컨의 Socket.IO 팔로우 이벤트 감지
- `FOLLOW_CHANNEL` 이벤트 검증과 중복 알림 방지
- localhost HTTP를 통한 OBS 전용 소스 전달
- PNG, JPG, WebP, GIF 등 이미지 미디어 렌더링
- 미디어의 왼쪽·오른쪽·위·아래·배경 배치와 크기·불투명도 설정
- 알림마다 애니메이션 GIF를 처음부터 다시 재생하는 옵션
- 동봉된 기본 알림음 3종 또는 사용자 음원 재생 및 음량 설정
- 문구, 글꼴, 색상, 배경, 외곽선, 그림자와 정렬 설정
- 표시 시간, 페이드 인·아웃, 대기열과 중복 방지 설정
- Chrome/Edge Manifest V3 확장 프로그램
- Windows x64 설치 프로그램과 수동 설치 ZIP 생성

## 저장소 역할

이 저장소는 플러그인과 브라우저 중계 확장 프로그램의 소스, 빌드 설정과 기술 문서를 관리합니다. 일반 사용자는 소스를 직접 빌드할 필요 없이 [배포 저장소의 최신 릴리스](https://github.com/TereBin/obs-chzzk-alerts-releases/releases/latest)를 설치하면 됩니다.

## 아키텍처

```text
CHZZK remote-control Socket.IO
  → Chrome/Edge MAIN-world WebSocket hook
  → isolated content-script relay
  → HTTP GET 127.0.0.1:21345/publish
  → OBS plugin local TCP server
  → native CHZZK follow alert source
```

구성 요소:

- `src/chzzk-alert-server.*`: Qt Network 기반 localhost HTTP 수신 서버
- `src/chzzk-alert-request.*`: HTTP 요청 검증과 응답 생성
- `src/chzzk-alert-source.*`: OBS 입력 소스, 렌더링과 구성 요소 연결
- `src/chzzk-alert-settings.*`: 저장 설정 키, 기본값, 속성 UI와 런타임 설정 읽기
- `src/chzzk-alert-model.hpp`: 대기열, 중복 방지, 페이드와 재진입 가능한 크기 조회
- `src/chzzk-alert-visual.hpp`: 비공개 텍스트·이미지 소스 및 불투명도 필터 수명 관리
- `src/chzzk-alert-audio.*`: 음원 선택, 표시 상태와 부모 소스로의 오디오 전달
- `src/chzzk-obs-resource.hpp`: OBS 데이터·소스 참조의 자동 해제
- `src/plugin-main.cpp`: OBS 모듈 수명 주기와 포트 소유권 관리
- `data/browser-relay/websocket-hook.js`: 치지직 스튜디오의 일반 탭과 팝업 페이지 컨텍스트에서 WebSocket 수신 프레임 감시
- `data/browser-relay/relay.js`: 검증된 팔로우 이벤트를 localhost로 전달

## OBS 소스 설정

`치지직 팔로우 알림`은 브라우저 소스가 아닌 네이티브 OBS 입력 소스입니다.

| 설정 | 설명 |
| --- | --- |
| 이미지 / GIF | 로컬 이미지 또는 애니메이션 GIF 사용 여부와 파일 선택 |
| 미디어 배치 | 텍스트 기준 왼쪽·오른쪽·위·아래 또는 배경으로 배치 |
| 미디어 크기·여백 | 렌더링 영역의 너비·높이와 텍스트 사이 간격 설정 |
| 미디어 불투명도 | 알림 전체 페이드와 별개로 미디어의 최대 불투명도 설정 |
| 원본 비율 유지 | 지정 영역 안에 원본 비율을 유지해 맞춤 |
| GIF 재시작 | 새 알림이 시작될 때 애니메이션을 첫 프레임부터 재생 |
| 알림 소리 | 기본 알림음 3종 또는 사용자 파일 선택과 음량 설정 |
| 알림 문구 | `{nickname}`을 팔로워 닉네임으로 치환 |
| 글꼴·글자색 | GDI+ 텍스트 모양 설정 |
| 배경 | 색상과 불투명도 설정 |
| 외곽선·그림자 | 사용 여부, 색상, 크기·거리와 불투명도 설정 |
| 정렬·크기 | 텍스트 정렬과 소스의 기본 너비·높이 설정 |
| 표시 시간 | 완전히 보이는 시간 설정 |
| 페이드 | 나타나고 사라지는 시간을 밀리초 단위로 설정 |
| 최대 대기 알림 수 | 이벤트가 연속으로 들어올 때 보관할 대기열 크기 설정 |
| 중복 방지 시간 | 같은 닉네임의 반복 알림을 무시할 시간 설정 |

속성 화면의 `테스트 알림 표시` 버튼은 네트워크 연결 없이 렌더링, 애니메이션과 음원 재생을 확인합니다.

미디어는 OBS의 내장 `image_source`를 비공개 자식 소스로 생성해 렌더링합니다. 텍스트와 미디어에는 각각 `color_filter`가 적용되며, 알림의 페이드 값과 사용자가 지정한 미디어 불투명도를 함께 반영합니다. 왼쪽·오른쪽·위·아래 배치는 미디어 영역만큼 텍스트 extents를 줄이고, 배경 배치는 전체 소스 크기 안에 미디어를 맞춘 뒤 텍스트를 위에 그립니다.

알림 소리는 OBS의 내장 `ffmpeg_source`를 비공개 소스로 생성합니다. 선택된 음원에는 보조 표시 참조(`obs_source_inc_showing`)를 연결해 FFmpeg의 재생 조건을 충족시키고, 캡처한 오디오를 `obs_source_output_audio`로 부모 알림 소스에 전달합니다. 믹서·모니터링·출력 트랙은 부모 소스에서 처리하며, 음원을 선택하는 것만으로 재생하지 않습니다. 새 알림이 시작될 때 처음부터 한 번 재생합니다.

설정 갱신·렌더링·대기열 변경은 소스 잠금으로 직렬화합니다. OBS와 다른 플러그인의 신호 처리에서 크기 조회가 동기적으로 재진입할 수 있으므로 `get_width`와 `get_height`는 별도 원자적 스냅샷을 읽습니다. 오디오 콜백은 이 잠금을 획득하지 않으며, 현재 선택된 음원의 데이터만 전달합니다.

## 치지직 이벤트 프로토콜

현재 확인된 팔로우 이벤트는 Socket.IO의 `42` 이벤트 프레임입니다.

```text
42["follow", "{...JSON 문자열...}"]
```

두 번째 배열 원소는 JSON 객체가 아니라 JSON 문자열이므로 두 단계로 파싱합니다. 다음 조건을 모두 만족하는 이벤트만 처리합니다.

- Socket.IO 이벤트 이름이 `follow`
- `newsFeedType`이 `FOLLOW_CHANNEL`
- `user.nickname`이 비어 있지 않음

중복 방지는 우선 `newsFeedNo`를 사용하고, 값이 없으면 `userIdHash`와 `createdDate` 조합을 사용합니다. OBS로 전달하는 표시 데이터는 팔로워 닉네임입니다.

## 로컬 API

플러그인은 IPv4 loopback의 `127.0.0.1:21345`에만 바인딩합니다.

- `GET /publish?type=follow&nickname=<name>`: 팔로우 이벤트 발행

외부 서버나 클라우드 저장소를 사용하지 않습니다. 플러그인이 로드되지 않았거나 다른 프로세스가 포트를 점유하면 중계 서버 시작이 실패합니다.

## 호환성

- Windows 10/11 x64
- OBS Studio x64
- Chrome 또는 Edge의 Manifest V3 확장 프로그램
- OBS Live Editor와 함께 사용할 경우 `0.3.2` 이상

OBS Live Editor `0.3.1`에는 동일한 중계 서버가 포함되어 있어 `21345` 포트가 충돌합니다. `0.3.2`부터 알림 기능이 이 프로젝트로 분리되었습니다.

## 빌드

필요한 도구:

- Visual Studio 2022와 Desktop development with C++ 워크로드
- CMake 3.28 이상
- Inno Setup 6(Windows 설치 프로그램 생성 시)

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release
cmake --install build_x64 --config Release --prefix release/Release
./installer/Build-Installer.ps1 -Configuration Release
```

OBS 플러그인 템플릿의 의존성 캐시를 재사용하려면 구성 시 `OBS_PLUGIN_DEPENDENCIES_DIR`을 지정할 수 있습니다.

```powershell
cmake -S . -B build_x64 -G "Visual Studio 17 2022" -A x64 `
  -DOBS_PLUGIN_DEPENDENCIES_DIR=C:/path/to/.deps
```

## 배포

- 소스 버전: `buildspec.json`
- 확장 프로그램 버전: `data/browser-relay/manifest.json`
- Windows 설치 구성: `installer/obs-chzzk-alerts.iss`
- CI 구성: `.github/workflows/build-windows.yml`
- 사용자 배포: [obs-chzzk-alerts-releases](https://github.com/TereBin/obs-chzzk-alerts-releases)

릴리스 설치 파일은 현재 코드 서명되지 않았습니다. 배포 시 설치 파일, 수동 설치 ZIP과 `SHA256SUMS.txt`를 함께 게시합니다.

## 테스트

브라우저 확장 프로그램의 JavaScript와 JSON 매니페스트는 다음과 같이 확인할 수 있습니다.

```powershell
node --check data/browser-relay/websocket-hook.js
node --check data/browser-relay/relay.js
node -e "JSON.parse(require('fs').readFileSync('data/browser-relay/manifest.json', 'utf8'))"
```

OBS가 실행 중일 때 로컬 전달 경로는 다음 요청으로 확인할 수 있습니다.

```powershell
Invoke-WebRequest -UseBasicParsing `
  'http://127.0.0.1:21345/publish?type=follow&nickname=Integration%20Test'
```

정상 요청은 HTTP `204`를 반환합니다. Release 빌드와 설치 트리는 GitHub Actions의 `Build Windows` 워크플로에서도 검증합니다.

자동 회귀 테스트는 선택적으로 빌드합니다.

```powershell
cmake --preset windows-x64 -DENABLE_ALERT_TESTS=ON
cmake --build --preset windows-x64 --config Release
ctest --test-dir build_x64 -C Release --output-on-failure
```

테스트는 대기열·페이드·크기 재진입, OBS API 대역을 사용한 음원 표시 참조·재생·오디오 전달·해제, 실제 loopback 소켓의 분할 요청과 잘못된 요청, 브라우저 팔로우 프레임 파싱과 중복 방지를 검증합니다. 브라우저 테스트에는 Node.js가 필요합니다. 실제 OBS의 믹서·모니터링과 다른 플러그인과의 상호작용은 OBS에서 별도로 확인해야 합니다.

## 보안

- HTTP 서버는 외부 인터페이스가 아닌 `127.0.0.1`에만 바인딩합니다.
- 확장 프로그램은 치지직 스튜디오 리모컨 URL에서만 실행합니다.
- 치지직 로그인 쿠키, WebSocket 인증값과 전체 이벤트 JSON은 OBS로 전달하지 않습니다.
- 별도의 외부 서버, 계정 데이터베이스 또는 원격 로그 수집 기능을 사용하지 않습니다.

## 범위

현재는 팔로우 이벤트만 지원합니다. 구독과 후원은 치지직 자체 알림 기능을 사용하며 이 프로젝트에서 처리하지 않습니다.

## 문제 보고

오류 보고와 기능 제안은 [OBS Chzzk Alerts Issues](https://github.com/TereBin/obs-chzzk-alerts/issues)를 이용해 주세요. 문제 발생 직후 OBS 로그에서 `[obs-chzzk-alerts]`가 포함된 줄과 Chrome/Edge 개발자 도구의 오류를 함께 제공하면 원인 확인에 도움이 됩니다.

WebSocket URL의 `auth` 값, 치지직 쿠키 또는 전체 HAR 파일은 공개 이슈에 첨부하지 마세요.

## 라이선스

GPL-2.0 라이선스로 배포됩니다. 자세한 내용은 [LICENSE](LICENSE)를 확인하세요.
