(() => {
  const NativeWebSocket = window.WebSocket;
  if (!NativeWebSocket || NativeWebSocket.__obsChzzkWrapped) return;

  function inspectFrame(frame) {
    if (typeof frame !== "string" || !frame.startsWith("42")) return;

    try {
      const packet = JSON.parse(frame.slice(2));
      if (!Array.isArray(packet) || packet[0] !== "follow") return;

      const event = typeof packet[1] === "string"
        ? JSON.parse(packet[1])
        : packet[1];
      if (!event || event.newsFeedType !== "FOLLOW_CHANNEL") return;

      const nickname = event.user?.nickname?.trim();
      if (!nickname) return;

      window.postMessage({
        source: "obs-chzzk-websocket-relay",
        type: "follow",
        nickname,
        newsFeedNo: event.newsFeedNo ?? null,
        userIdHash: event.user?.userIdHash ?? "",
        createdDate: event.createdDate ?? ""
      }, window.location.origin);
    } catch {
      // Ignore unrelated or malformed Socket.IO frames.
    }
  }

  function ObservedWebSocket(...args) {
    const socket = new NativeWebSocket(...args);
    socket.addEventListener("message", (event) => inspectFrame(event.data));
    return socket;
  }

  Object.setPrototypeOf(ObservedWebSocket, NativeWebSocket);
  ObservedWebSocket.prototype = NativeWebSocket.prototype;
  Object.defineProperty(ObservedWebSocket, "__obsChzzkWrapped", { value: true });
  window.WebSocket = ObservedWebSocket;
})();
