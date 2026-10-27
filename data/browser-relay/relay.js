(() => {
  const seen = new Set();
  const MAX_SEEN_EVENTS = 1024;
  const RELAY_SOURCE = "obs-chzzk-websocket-relay";
  const PUBLISH_URL = "http://127.0.0.1:21345/publish";

  function eventKey(event) {
    if (event.newsFeedNo !== null && event.newsFeedNo !== undefined && event.newsFeedNo !== "") {
      return `feed:${event.newsFeedNo}`;
    }
    if (event.userIdHash && event.createdDate) {
      return `user:${event.userIdHash}|date:${event.createdDate}`;
    }
    return null;
  }

  function publishFollow(event) {
    const key = eventKey(event);
    if (key !== null) {
      if (seen.has(key)) return;
      seen.add(key);
      if (seen.size > MAX_SEEN_EVENTS) seen.delete(seen.values().next().value);
    }

    const params = new URLSearchParams({
      type: "follow",
      nickname: event.nickname.trim()
    });
    fetch(`${PUBLISH_URL}?${params.toString()}`, {
      method: "GET",
      mode: "no-cors",
      keepalive: true
    }).catch(() => {
      // A later duplicate frame can retry after OBS becomes available.
      if (key !== null) seen.delete(key);
    });
  }

  window.addEventListener("message", (message) => {
    if (message.source !== window) return;
    if (message.origin !== window.location.origin) return;
    const event = message.data;
    if (event?.source !== RELAY_SOURCE) return;
    if (event.type !== "follow" || typeof event.nickname !== "string" || !event.nickname.trim()) return;
    publishFollow(event);
  });
})();
