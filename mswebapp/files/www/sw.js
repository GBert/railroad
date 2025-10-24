const CACHE_VERSION = 'mswebapp-v1';
const PRECACHE = [
	'/',
	'/static/style.css',
	'/static/script.js',
	'/static/grafics/apple-touch-icon-180.png'
];

self.addEventListener('install', (e) => {
	e.waitUntil(
		caches.open(CACHE_VERSION)
			.then((c) => c.addAll(PRECACHE))
			.then(() => self.skipWaiting())
	);
});

self.addEventListener('activate', (e) => {
	e.waitUntil(
		caches.keys().then(keys => Promise.all(keys.map(k => (k !== CACHE_VERSION ? caches.delete(k) : Promise.resolve()))))
			.then(() => self.clients.claim())
	);
});

self.addEventListener('fetch', (e) => {
	const req = e.request;
	if (req.method !== 'GET') return;
	const url = new URL(req.url);

	// Stale-while-revalidate for static assets
	if (url.pathname.startsWith('/static/')) {
		e.respondWith((async () => {
			const cache = await caches.open(CACHE_VERSION);
			const cached = await cache.match(req);
			const net = fetch(req).then((resp) => {
				if (resp && resp.ok) cache.put(req, resp.clone());
				return resp;
			}).catch(() => cached);
			return cached || net;
		})());
		return;
	}

	// Navigation fallback: network-first, fallback to cached '/'
	if (req.mode === 'navigate') {
		e.respondWith((async () => {
			try { return await fetch(req); } catch (e) {
				const cache = await caches.open(CACHE_VERSION);
				return (await cache.match('/')) || Response.error();
			}
		})());
		return;
	}
});

