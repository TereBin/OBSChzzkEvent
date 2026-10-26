# OBS Chzzk Alerts

치지직 스트리머 스튜디오 리모컨의 새 팔로우 이벤트를 OBS 브라우저 소스에 표시하는 Windows용 도구입니다.

## 다운로드

일반 사용자는 [배포 저장소의 최신 릴리스](https://github.com/TereBin/obs-chzzk-alerts-releases/releases/latest)에서 Windows 설치 프로그램을 내려받으세요. 이 저장소는 소스 코드와 개발 문서를 제공합니다.

## 구성

- OBS 플러그인: `127.0.0.1:21345`에서 브라우저 확장 프로그램과 OBS 브라우저 소스를 연결합니다.
- Chrome/Edge 확장 프로그램: 리모컨의 Socket.IO `follow` 이벤트를 감지합니다.
- OBS 브라우저 소스: 팔로워 닉네임을 투명 배경의 텍스트 알림으로 표시합니다.

수집하는 값은 팔로워 닉네임, 이벤트 식별 번호, 사용자 해시와 발생 시각입니다. OBS로 전달되는 값은 알림 표시에 필요한 닉네임뿐이며 외부 서버로 전송하지 않습니다.

## 설치 및 설정

기존 OBS Live Editor를 함께 사용하는 경우 반드시 `0.3.2` 이상으로 업데이트해야 합니다. Live Editor `0.3.1`은 같은 로컬 포트를 사용하므로 동시에 실행할 수 없습니다.

1. OBS Studio를 종료하고 Windows 설치 프로그램을 실행합니다.
2. Chrome 또는 Edge에서 확장 프로그램 관리 페이지를 엽니다.
3. 개발자 모드를 활성화하고 `압축해제된 확장 프로그램을 로드`를 선택합니다.
4. 다음 폴더를 선택합니다.

   `C:\Program Files\obs-studio\data\obs-plugins\obs-chzzk-alerts\browser-relay`

5. OBS에서 브라우저 소스를 추가하고 `로컬 파일`로 다음 파일을 선택합니다.

   `C:\Program Files\obs-studio\data\obs-plugins\obs-chzzk-alerts\browser-source\chzzk-alert.html`

6. OBS를 실행한 상태에서 Chrome/Edge로 치지직 스트리머 스튜디오 리모컨을 열어 둡니다.

확장 프로그램을 설치하거나 업데이트한 뒤에는 리모컨 탭을 새로고침해야 합니다.

## 동작 방식

```text
치지직 리모컨 WebSocket
  → 브라우저 확장 프로그램
  → http://127.0.0.1:21345/publish
  → OBS 플러그인 SSE
  → OBS 브라우저 소스
```

확장 프로그램은 Socket.IO `follow` 이벤트에서 `FOLLOW_CHANNEL` 유형만 처리합니다. 구독과 후원은 현재 지원하지 않습니다.

## 테스트

브라우저 소스 파일 경로 뒤에 `?demo=follow&nickname=테스트`를 붙이면 화면 표시를 테스트할 수 있습니다. 실제 연동은 OBS, 확장 프로그램, 치지직 리모컨이 모두 실행된 상태에서 새로운 계정의 팔로우로 확인합니다.

## 빌드

필요한 도구:

- Visual Studio 2022와 Desktop development with C++ 워크로드
- CMake 3.28 이상
- Inno Setup 6(설치 프로그램 생성 시)

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release
cmake --install build_x64 --config Release --prefix release/Release
./installer/Build-Installer.ps1 -Configuration Release
```

## 라이선스

GPL-2.0 라이선스로 배포됩니다. 자세한 내용은 [LICENSE](LICENSE)를 확인하세요.
