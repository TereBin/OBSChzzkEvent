# OBS Chzzk Alerts

치지직 스트리머 스튜디오 리모컨의 팔로우 이벤트를 OBS 브라우저 소스로 전달하는 Windows용 OBS 플러그인입니다.

일반 사용자를 위한 설치 파일과 사용 설명서는 [배포 저장소](https://github.com/TereBin/obs-chzzk-alerts-releases)에서 제공합니다. 이 저장소는 소스 코드, 프로토콜 구조와 개발 문서를 관리합니다.

- 배포 패키지: [OBS Chzzk Alerts Releases](https://github.com/TereBin/obs-chzzk-alerts-releases/releases/latest)
- 소스 저장소: [OBS Chzzk Alerts](https://github.com/TereBin/obs-chzzk-alerts)

## 기능

- 치지직 리모컨의 Socket.IO 팔로우 이벤트 감지
- `FOLLOW_CHANNEL` 이벤트 검증과 중복 알림 방지
- localhost HTTP/SSE를 통한 OBS 브라우저 소스 전달
- 투명 배경의 팔로워 닉네임 텍스트 알림
- Chrome/Edge Manifest V3 확장 프로그램
- Windows x64 설치 프로그램과 수동 설치 ZIP 생성

## 아키텍처

```text
CHZZK remote-control Socket.IO
  → Chrome/Edge MAIN-world WebSocket hook
  → isolated content-script relay
  → HTTP GET 127.0.0.1:21345/publish
  → OBS plugin local TCP server
  → SSE 127.0.0.1:21345/events
  → OBS browser source
```

구성 요소:

- `src/chzzk-alert-server.*`: Qt Network 기반 localhost HTTP/SSE 중계 서버
- `src/plugin-main.cpp`: OBS 모듈 수명 주기와 포트 소유권 관리
- `data/browser-relay/websocket-hook.js`: 페이지 컨텍스트에서 WebSocket 수신 프레임 감시
- `data/browser-relay/relay.js`: 검증된 팔로우 이벤트를 localhost로 전달
- `data/browser-source/chzzk-alert.html`: SSE 이벤트를 텍스트 알림으로 렌더링

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
- `GET /events`: OBS 브라우저 소스용 Server-Sent Events 스트림

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

Release 빌드와 설치 트리는 GitHub Actions의 `Build Windows` 워크플로에서도 검증합니다.

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
