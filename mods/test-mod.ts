/**
 * Command feedback: use chat.local (HUD only). Use chat.send to broadcast as you.
 */
engine.log('[test-mod] loaded — /hello, /echo (private feedback)');

commands.register('hello', () => {
  chat.local('Hello from test-mod! (only you see this)');
});

commands.register('echo', (args) => {
  chat.local('[echo] ' + (args.length ? args.join(' ') : '(empty)'));
});

events.onJoinWorld((_p, w) => {
  engine.log('[test-mod] join_world → ' + w.getName());
});
