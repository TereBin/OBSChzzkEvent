const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const root = path.join(__dirname, '../data/browser-relay');

function popupEnvironment(initialHandler) {
  const elements = new Map(), requests = [], timers = new Map();
  let handler = initialHandler ?? (() => Promise.resolve({
    status: 200, json: async () => ({ listening: true, bridges: 0, testAlerts: true })
  }));
  let timerId = 0, sequence = 0;
  const document = { getElementById(id) {
    if (!elements.has(id)) elements.set(id, {
      value: '확장 프로그램 테스트', textContent: '', dataset: {}, disabled: false, listeners: new Map(),
      addEventListener(event, callback) { this.listeners.set(event, callback); },
      setAttribute() {}
    });
    return elements.get(id);
  } };
  const context = vm.createContext({
    document, URLSearchParams, AbortController,
    crypto: { randomUUID: () => `unique-${++sequence}` },
    setTimeout(callback) { const id = ++timerId; timers.set(id, callback); return id; },
    clearTimeout(id) { timers.delete(id); },
    fetch(url, options) { requests.push({ url, options }); return handler(url, options); }
  });
  for (const name of ['obs-client.js', 'popup.js']) {
    vm.runInContext(fs.readFileSync(path.join(root, name), 'utf8'), context);
  }
  return {
    context, elements, requests, timers,
    setFetch(next) { handler = next; },
    async flush() { for (let i = 0; i < 40; ++i) await Promise.resolve(); },
    async submit() { await elements.get('test-form').listeners.get('submit')({ preventDefault() {} }); },
    async check() { await elements.get('check').listeners.get('click')(); }
  };
}

test('manifest exposes a popup and loads the shared client before the relay', () => {
  const manifest = JSON.parse(fs.readFileSync(path.join(root, 'manifest.json'), 'utf8'));
  const popup = fs.readFileSync(path.join(root, manifest.action.default_popup), 'utf8');
  assert.match(popup, /src="obs-client.js"/);
  assert.match(popup, /src="popup.js"/);
  assert.deepEqual(manifest.content_scripts[1].js, ['obs-client.js', 'relay.js']);
  assert.deepEqual(manifest.host_permissions, ['http://127.0.0.1:21345/*']);
});

test('extension-only tests receive an ACK and repeated clicks use distinct IDs', async () => {
  const env = popupEnvironment(); await env.flush();
  assert.equal(env.elements.get('send').disabled, false);
  assert.match(env.elements.get('connection').textContent, /OBS 연결됨/);
  env.setFetch(() => Promise.resolve({ status: 204 }));
  env.elements.get('nickname').value = ' 테스트 & + ';
  await env.submit(); await env.submit();
  const messages = env.requests.filter(r => new URL(r.url).pathname === '/publish');
  assert.equal(messages.length, 2);
  const first = new URL(messages[0].url).searchParams;
  assert.equal(first.get('type'), 'test');
  assert.equal(first.get('nickname'), '테스트 & +');
  assert.notEqual(first.get('eventId'), new URL(messages[1].url).searchParams.get('eventId'));
  assert.equal(env.elements.get('result').dataset.state, 'success');
  assert.equal(env.timers.size, 0);
});

test('HTTP failures are visible and manual tests are never retried in the background', async () => {
  const env = popupEnvironment(); await env.flush();
  env.setFetch(() => Promise.resolve({ status: 503 }));
  await env.submit();
  assert.equal(env.elements.get('result').dataset.state, 'error');
  assert.match(env.elements.get('result').textContent, /503/);
  assert.equal(env.timers.size, 0);
  assert.equal(env.elements.get('send').disabled, false);
});

test('an offline OBS can be checked again after starting', async () => {
  const env = popupEnvironment(() => Promise.reject(new Error('offline'))); await env.flush();
  assert.equal(env.elements.get('send').disabled, true);
  assert.match(env.elements.get('connection').textContent, /연결하지 못했습니다/);
  env.setFetch(() => Promise.resolve({ status: 200, json: async () => ({ listening: true, bridges: 1, testAlerts: true }) }));
  await env.check();
  assert.equal(env.elements.get('send').disabled, false);
});

test('older plugins and unrelated status responses cannot enable tests', async () => {
  for (const state of [{ listening: true, bridges: 1 }, { ok: true }, { listening: true, bridges: -1, testAlerts: true }]) {
    const env = popupEnvironment(() => Promise.resolve({ status: 200, json: async () => state }));
    await env.flush();
    assert.equal(env.elements.get('send').disabled, true);
    assert.match(env.elements.get('connection').textContent, /0.6.1/);
  }
});

test('malformed status JSON displays the plugin guidance and releases the controls', async () => {
  const env = popupEnvironment(() => Promise.resolve({ status: 200, json: async () => { throw new SyntaxError('bad JSON'); } }));
  await env.flush();
  assert.equal(env.elements.get('send').disabled, true);
  assert.equal(env.elements.get('check').disabled, false);
  assert.match(env.elements.get('connection').textContent, /0.6.1/);
  assert.equal(env.timers.size, 0);
});

test('losing a previously working connection disables tests until a successful recheck', async () => {
  const env = popupEnvironment(); await env.flush();
  env.setFetch(() => Promise.reject(new Error('offline')));
  await env.check(); await env.submit();
  assert.equal(env.requests.length, 2);
  assert.equal(env.elements.get('send').disabled, true);
  assert.equal(env.elements.get('check').disabled, false);
});

test('blank names and rapid double submissions do not send extra requests', async () => {
  const env = popupEnvironment(); await env.flush();
  env.elements.get('nickname').value = ' ';
  await env.submit();
  assert.match(env.elements.get('result').textContent, /닉네임/);
  assert.equal(env.requests.length, 1);
  env.elements.get('nickname').value = 'Test';
  let acknowledge;
  env.setFetch(() => new Promise(resolve => { acknowledge = resolve; }));
  const first = env.submit();
  await env.submit();
  assert.equal(env.requests.length, 2);
  acknowledge({ status: 204 }); await first;
});

test('the five-second deadline includes a stalled response body', async () => {
  const env = popupEnvironment(() => Promise.resolve({ status: 200, json: () => new Promise(() => {}) }));
  await env.flush();
  for (const callback of [...env.timers.values()]) callback();
  await env.flush();
  assert.equal(env.requests[0].options.signal.aborted, true);
  assert.match(env.elements.get('connection').textContent, /연결하지 못했습니다/);
  assert.equal(env.elements.get('check').disabled, false);
});
