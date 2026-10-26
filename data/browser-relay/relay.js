(() => {
  const seen = new Set();

  function publishFollow(event) {
    const key = event.newsFeedNo
      ? `feed:${event.newsFeedNo}`
      : `user:${event.userIdHash}|date:${event.createdDate}`;
    if (seen.has(key)) return;
    seen.add(key);

    const params = new URLSearchParams({
      type: "follow",
      nickname: event.nickname
    });
    fetch(`http://127.0.0.1:21345/publish?${params.toString()}`, {
      method: "GET",
      mode: "no-cors",
      keepalive: true
    }).catch(() => {});
  }

  window.addEventListener("message", (message) => {
    if (message.source !== window) return;
    const event = message.data;
    if (event?.source !== "obs-chzzk-websocket-relay") return;
    if (event.type !== "follow" || typeof event.nickname !== "string") return;
    publishFollow(event);
  });
})();
