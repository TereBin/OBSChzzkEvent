const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function environment() {
  const listeners = new Map(), requests = [], timers = new Map();
  let now = 1000, timerId = 0;
  let handler = () => Promise.resolve({ status: 204 });
  class NativeWebSocket {
    addEventListener(type, cb) { if (type === 'message') this.receive = cb; }
  }
  const window = {
    location: { origin: 'https://studio.chzzk.naver.com' },
    WebSocket: NativeWebSocket,
    addEventListener(type, cb) { listeners.set(type, cb); },
    postMessage(data, origin) { listeners.get('message')?.({ source: window, origin, data }); }
  };
  const context = vm.createContext({
    window, URLSearchParams, AbortController,
    Date: class extends Date { static now() { return now; } },
    setTimeout(cb, delay) { const id = ++timerId; timers.set(id, { cb, at: now + delay }); return id; },
    clearTimeout(id) { timers.delete(id); },
    fetch(url, options) { requests.push({ url, options }); return handler(url, options); }
  });
  function load(name) {
    vm.runInContext(fs.readFileSync(path.join(__dirname, '../data/browser-relay', name), 'utf8'), context);
  }
  async function flush() { for (let i = 0; i < 40; ++i) await Promise.resolve(); }
  async function advance(ms) {
    const end = now + ms;
    await flush();
    for (;;) {
      const next = [...timers.entries()].filter(([, t]) => t.at <= end).sort((a, b) => a[1].at - b[1].at)[0];
      if (!next) break;
      timers.delete(next[0]); now = next[1].at; next[1].cb(); await flush();
    }
    now = end; await flush();
  }
  load('relay.js'); load('websocket-hook.js');
  const socket = new window.WebSocket('wss://test.invalid');
  return {
    window, socket, load, listeners, flush, advance,
    setFetch(fn) { handler = fn; },
    publishes() { return requests.filter(r => new URL(r.url).pathname === '/publish'); },
    heartbeats() { return requests.filter(r => new URL(r.url).pathname === '/heartbeat'); }
  };
}

const follow = (id = 216222144, nickname = 'Test User') => ({
  newsFeedNo: id, newsFeedType: 'FOLLOW_CHANNEL',
  user: { nickname, userIdHash: 'test-id' }, createdDate: '2026-10-27 02:03:56'
});
const frame = event => '42' + JSON.stringify(['follow', JSON.stringify(event)]);
const emit = (env, event = follow()) => env.socket.receive({ data: frame(event) });

test('follow reaches localhost once with a readable acknowledgement and stable event ID', async () => {
  const env = environment();
  emit(env); emit(env); await env.flush(); emit(env); await env.flush();
  assert.equal(env.publishes().length, 1);
  const request = env.publishes()[0], url = new URL(request.url);
  assert.equal(url.origin, 'http://127.0.0.1:21345');
  assert.equal(url.searchParams.get('nickname'), 'Test User');
  assert.equal(url.searchParams.get('eventId'), 'feed:216222144');
  assert.equal(request.options.mode, 'cors');
  assert.equal(url.searchParams.has('userIdHash'), false);
});

test('malformed, unrelated and empty frames are ignored', async () => {
  const env = environment();
  for (const data of [null, {}, '2', '42{', '42["chat",{}]',
    frame({ ...follow(), newsFeedType: 'DONATION' }), frame(follow(1, ' ')), frame(follow(1, 123))]) {
    env.socket.receive({ data });
  }
  await env.flush();
  assert.equal(env.publishes().length, 0);
});

test('object payload, unicode, fallback identity and repeated hook installation', async () => {
  const env = environment(), wrapped = env.window.WebSocket;
  env.load('websocket-hook.js'); assert.equal(env.window.WebSocket, wrapped);
  const event = follow(null, ' 테레빈 & + ');
  env.socket.receive({ data: '42' + JSON.stringify(['follow', event]) });
  emit(env, event); await env.flush();
  assert.equal(env.publishes().length, 1);
  assert.equal(new URL(env.publishes()[0].url).searchParams.get('nickname'), '테레빈 & +');
});

test('connection failures retry automatically without another incoming follow', async () => {
  const env = environment(); let attempts = 0;
  env.setFetch(url => {
    if (new URL(url).pathname === '/publish' && ++attempts < 3) return Promise.reject(new Error('offline'));
    return Promise.resolve({ status: 204 });
  });
  emit(env); await env.flush();
  await env.advance(500); await env.advance(1000);
  assert.equal(env.publishes().length, 3);
  assert.equal(new Set(env.publishes().map(r => new URL(r.url).searchParams.get('eventId'))).size, 1);
  emit(env); await env.flush();
  assert.equal(env.publishes().length, 3);
});

test('HTTP errors do not acknowledge or discard queued events', async () => {
  const env = environment();
  env.setFetch(() => Promise.resolve({ status: 503 }));
  emit(env); await env.flush();
  env.setFetch(() => Promise.resolve({ status: 204 }));
  await env.advance(500);
  assert.equal(env.publishes().length, 2);
  await env.advance(4500);
  const status = new URL(env.heartbeats().at(-1).url).searchParams;
  assert.equal(status.get('pending'), '0');
  assert.equal(status.get('error'), 'http');
});

test('a hung fetch is aborted and retried', async () => {
  const env = environment();
  env.setFetch(url => new URL(url).pathname === '/publish' ? new Promise(() => {}) : Promise.resolve({ status: 204 }));
  emit(env); await env.advance(5000);
  assert.equal(env.publishes()[0].options.signal.aborted, true);
  env.setFetch(() => Promise.resolve({ status: 204 }));
  await env.advance(500);
  assert.equal(env.publishes().length, 2);
});

test('offline queue is bounded, and retries expire after five minutes', async () => {
  const env = environment();
  env.setFetch(url => new URL(url).pathname === '/publish' ? Promise.reject(new Error('offline')) : Promise.resolve({ status: 204 }));
  emit(env, follow(0)); await env.flush();
  for (let i = 1; i <= 130; ++i) emit(env, follow(i));
  await env.advance(5000);
  assert.equal(new URL(env.heartbeats().at(-1).url).searchParams.get('pending'), '100');
  await env.advance(310000);
  const before = env.publishes().length;
  env.setFetch(() => Promise.resolve({ status: 204 }));
  await env.advance(10000);
  assert.equal(env.publishes().length, before);
  emit(env, follow(999)); await env.flush();
  assert.equal(env.publishes().length, before + 1);
});

test('successful deduplication history is bounded', async () => {
  const env = environment();
  for (let i = 0; i <= 1024; ++i) { emit(env, follow(i)); await env.flush(); }
  emit(env, follow(0)); await env.flush();
  assert.equal(env.publishes().length, 1026);
});

test('foreign origins, foreign windows and empty nicknames cannot publish', async () => {
  const env = environment(), listener = env.listeners.get('message');
  const data = { source: 'obs-chzzk-websocket-relay', type: 'follow', nickname: 'x' };
  listener({ source: {}, origin: env.window.location.origin, data });
  listener({ source: env.window, origin: 'https://other.invalid', data });
  listener({ source: env.window, origin: env.window.location.origin, data: { ...data, nickname: ' ' } });
  await env.flush(); assert.equal(env.publishes().length, 0);
});
