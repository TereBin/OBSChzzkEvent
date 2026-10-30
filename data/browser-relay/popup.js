(() => {
  const client = globalThis.ObsChzzkClient;
  const connection = document.getElementById("connection");
  const result = document.getElementById("result");
  const check = document.getElementById("check");
  const send = document.getElementById("send");
  const form = document.getElementById("test-form");
  const nickname = document.getElementById("nickname");
  let busy = false;
  let supported = false;

  function show(element, state, message) {
    element.dataset.state = state;
    element.textContent = message;
  }

  function setBusy(value) {
    busy = value;
    check.disabled = value;
    send.disabled = value || !supported;
    form.setAttribute("aria-busy", String(value));
  }

  function failure(error) {
    if (error.message === "nickname") return "테스트 닉네임을 1~80자로 입력해 주세요.";
    if (error.message === "invalid" || error.status === 400 || error.status === 404) {
      return "OBS Chzzk Alerts 0.6.1 이상으로 업데이트한 뒤 OBS를 다시 실행해 주세요.";
    }
    if (error.message === "http") return `OBS가 요청을 거부했습니다 (HTTP ${error.status}). 플러그인 상태를 확인해 주세요.`;
    return "OBS에 연결하지 못했습니다. OBS 실행 여부와 치지직 알림 플러그인 설치 상태를 확인해 주세요.";
  }

  async function runOperation(target, pendingMessage, operation) {
    if (busy) return;
    setBusy(true);
    show(target, "pending", pendingMessage);
    try {
      await operation();
    } catch (error) {
      show(target, "error", failure(error));
    } finally {
      setBusy(false);
    }
  }

  function checkConnection() {
    return runOperation(connection, "OBS 연결 확인 중…", async () => {
      supported = false;
      const state = await client.getStatus();
      supported = state.testAlerts === true;
      show(connection, supported ? "success" : "error", supported
        ? "OBS 연결됨 · 테스트 알림을 보낼 수 있습니다."
        : "OBS 연결됨 · 테스트 기능은 플러그인 0.6.1 이상에서 지원합니다.");
    });
  }

  check.addEventListener("click", checkConnection);
  form.addEventListener("submit", event => {
    event.preventDefault();
    if (busy || !supported) return;
    return runOperation(result, "테스트 알림을 보내는 중…", async () => {
      await client.sendTest(nickname.value);
      show(result, "success", "OBS가 테스트 신호를 받았습니다. 방송 화면의 알림 소스를 확인해 주세요.");
    });
  });
  void checkConnection();
})();
