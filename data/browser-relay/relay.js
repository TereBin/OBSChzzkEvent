(() => {
  const RELAY_SOURCE = "obs-chzzk-websocket-relay";
  const { request } = globalThis.ObsChzzkClient;
  const MAX_PENDING = 100;
  const MAX_SEEN = 1024;
  const MAX_AGE_MS = 5 * 60 * 1000;
  const seen = new Set();
  const pending = [];
  const session = `bridge:${Date.now()}:${Math.random().toString(36).slice(2)}`;
  let sequence = 0;
  let draining = false;
  let retryTimer = null;
  let lastError = "";

  function eventKey(event) {
    if (event.newsFeedNo !== null && event.newsFeedNo !== undefined && event.newsFeedNo !== "") {
      return `feed:${event.newsFeedNo}`;
    }
    if (event.userIdHash && event.createdDate) {
      return `user:${event.userIdHash}|date:${event.createdDate}`;
    }
    return null;
  }


  function remember(key) {
    if (key === null) return;
    seen.add(key);
    if (seen.size > MAX_SEEN) seen.delete(seen.values().next().value);
  }

  async function drain() {
    if (draining) return;
    draining = true;
    retryTimer = null;
    try {
      while (pending.length) {
        const item = pending[0];
        if (Date.now() - item.created >= MAX_AGE_MS) {
          pending.shift();
          lastError = "expired";
          continue;
        }
        try {
          await request("publish", new URLSearchParams({
            type: "follow", nickname: item.nickname, eventId: item.id
          }));
          pending.shift();
          remember(item.key);
        } catch (error) {
          lastError = error.message === "http" ? "http" : "connection";
          const delay = Math.min(10000, 500 * 2 ** Math.min(item.attempts++, 5));
          retryTimer = setTimeout(drain, delay);
          break;
        }
      }
    } finally {
      draining = false;
    }
  }

  function publishFollow(event) {
    const key = eventKey(event);
    if (key !== null && (seen.has(key) || pending.some(item => item.key === key))) return;
    if (pending.length >= MAX_PENDING) {
      // Preserve the in-flight request; discard the oldest waiting item.
      pending.splice(draining ? 1 : 0, 1);
      lastError = "full";
    }
    const id = key !== null && key.length <= 256 ? key : `${session}:${++sequence}`;
    pending.push({ key, id, nickname: event.nickname.trim(), created: Date.now(), attempts: 0 });
    if (!retryTimer) void drain();
  }

  async function heartbeat() {
    try {
      await request("heartbeat", new URLSearchParams({
        session, pending: String(pending.length), error: lastError
      }));
    } catch {
      lastError = "connection";
    } finally {
      setTimeout(heartbeat, 5000);
    }
  }

  window.addEventListener("message", message => {
    if (message.source !== window || message.origin !== window.location.origin) return;
    const event = message.data;
    if (event?.source !== RELAY_SOURCE || event.type !== "follow") return;
    if (typeof event.nickname !== "string" || !event.nickname.trim()) return;
    publishFollow(event);
  });
  void heartbeat();
})();
