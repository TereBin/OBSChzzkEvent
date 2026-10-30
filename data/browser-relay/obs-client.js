(() => {
  const BASE_URL = "http://127.0.0.1:21345";
  const REQUEST_TIMEOUT_MS = 5000;
  const MAX_TEST_NICKNAME_LENGTH = 80;

  function requestError(code, status) {
    const error = new Error(code);
    if (status !== undefined) error.status = status;
    return error;
  }

  async function readStatus(response) {
    let state;
    try {
      state = await response.json();
    } catch {
      throw requestError("invalid");
    }
    if (!state || state.listening !== true || !Number.isInteger(state.bridges) || state.bridges < 0) {
      throw requestError("invalid");
    }
    return state;
  }

  async function request(path, params = new URLSearchParams(), expectedStatus = 204) {
    const controller = new AbortController();
    let timer;
    const timeout = new Promise((_, reject) => {
      timer = setTimeout(() => {
        controller.abort();
        reject(requestError("connection"));
      }, REQUEST_TIMEOUT_MS);
    });
    try {
      const response = await Promise.race([
        fetch(`${BASE_URL}/${path}?${params}`, {
          method: "GET", mode: "cors", signal: controller.signal, cache: "no-store"
        }),
        timeout
      ]);
      if (response.status !== expectedStatus) {
        throw requestError("http", response.status);
      }
      // Keep the deadline active until the status body has also arrived.
      return expectedStatus === 200 ? await Promise.race([readStatus(response), timeout]) : undefined;
    } finally {
      clearTimeout(timer);
    }
  }

  function getStatus() {
    return request("status", new URLSearchParams(), 200);
  }

  async function sendTest(nickname) {
    const name = typeof nickname === "string" ? nickname.trim() : "";
    if (!name || name.length > MAX_TEST_NICKNAME_LENGTH) {
      throw requestError("nickname");
    }
    await request("publish", new URLSearchParams({
      type: "test", nickname: name, eventId: `test:${crypto.randomUUID()}`
    }));
  }

  globalThis.ObsChzzkClient = Object.freeze({ request, getStatus, sendTest });
})();
