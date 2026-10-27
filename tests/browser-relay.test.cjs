const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function environment() {
  const listeners = new Map();
  const posts = [];
  const requests = [];
  class NativeWebSocket {
    constructor(url) { this.url = url; }
    addEventListener(type, cb) { if (type === 'message') this.receive = cb; }
  }
  const window = {
    location: { origin: 'https://studio.chzzk.naver.com' },
    WebSocket: NativeWebSocket,
    addEventListener(type, cb) { listeners.set(type, cb); },
    postMessage(data, origin) {
      posts.push(data);
      listeners.get('message')?.({ source: window, origin, data });
    }
  };
  const context = vm.createContext({ window, URLSearchParams,
    fetch(url, options) { requests.push({ url, options }); return Promise.resolve(); } });
  function load(name) {
    vm.runInContext(fs.readFileSync(path.join(__dirname, '../data/browser-relay', name), 'utf8'), context);
  }
  load('relay.js');
  load('websocket-hook.js');
  const socket = new window.WebSocket('wss://test.invalid');
  return { window, context, posts, requests, socket, load, listeners };
}

const follow = (id = 216222144, nickname = 'Test User') => ({
  newsFeedNo: id, newsFeedType: 'FOLLOW_CHANNEL',
  user: { nickname, userIdHash: 'test-id' }, createdDate: '2026-10-27 02:03:56'
});
const frame = event => '42' + JSON.stringify(['follow', JSON.stringify(event)]);

test('captured Socket.IO follow frame reaches localhost once, without credentials', () => {
  const env = environment();
  env.socket.receive({ data: frame(follow()) });
  env.socket.receive({ data: frame(follow()) });
  assert.equal(env.requests.length, 1);
  const url = new URL(env.requests[0].url);
  assert.equal(url.origin, 'http://127.0.0.1:21345');
  assert.deepEqual([...url.searchParams.keys()], ['type', 'nickname']);
  assert.equal(url.searchParams.get('nickname'), 'Test User');
  assert.equal(env.requests[0].options.mode, 'no-cors');
});

test('malformed, unrelated, empty and non-string frames are ignored', () => {
  const env = environment();
  for (const data of [null, {}, '2', '42{', '42["chat",{}]',
    frame({ ...follow(), newsFeedType: 'DONATION' }), frame(follow(1, '  ')), frame(follow(1, 123))]) {
    env.socket.receive({ data });
  }
  assert.equal(env.requests.length, 0);
});

test('object payload, unicode, fallback identity and hook reinstallation', () => {
  const env = environment();
  const wrapped = env.window.WebSocket;
  env.load('websocket-hook.js');
  assert.equal(env.window.WebSocket, wrapped);
  const event = follow(null, ' 테레빈 & + ');
  env.socket.receive({ data: '42' + JSON.stringify(['follow', event]) });
  env.socket.receive({ data: frame(event) });
  assert.equal(env.requests.length, 1);
  assert.equal(new URL(env.requests[0].url).searchParams.get('nickname'), '테레빈 & +');
});

test('bounded deduplication permits old IDs after eviction', () => {
  const env = environment();
  for (let i = 0; i <= 1024; ++i) env.socket.receive({ data: frame(follow(i)) });
  env.socket.receive({ data: frame(follow(0)) });
  assert.equal(env.requests.length, 1026);
});

test('a failed local connection permits a later duplicate to retry', async () => {
  const env = environment();
  env.context.fetch = () => Promise.reject(new Error('OBS offline'));
  env.socket.receive({ data: frame(follow()) });
  await new Promise(resolve => setImmediate(resolve));
  env.context.fetch = (url) => { env.requests.push({ url }); return Promise.resolve(); };
  env.socket.receive({ data: frame(follow()) });
  assert.equal(env.requests.length, 1);
});

test('foreign origin, foreign window and empty nicknames cannot publish', () => {
  const env = environment();
  const data = { source: 'obs-chzzk-websocket-relay', type: 'follow', nickname: 'x' };
  const listener = env.listeners.get('message');
  listener({ source: {}, origin: env.window.location.origin, data });
  listener({ source: env.window, origin: 'https://other.invalid', data });
  listener({ source: env.window, origin: env.window.location.origin, data: { ...data, nickname: ' ' } });
  assert.equal(env.requests.length, 0);
});
