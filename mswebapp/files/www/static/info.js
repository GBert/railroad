/*
 THE BEER-WARE LICENSE (Revision 42)

<mende.r@hotmail.de> wrote this file. As long as you retain this notice you can do whatever you want with this
 stuff. If we meet someday, and you think this stuff is worth it, you can
 buy me a beer in return.
 Ralf Mende
*/

// Info Site Button-Handler
// The 4 buttons are used to control the list of locomotives (Lokliste) handling of the SRSEII

// According to documentation locoId must be set 1.
const locoId = 1;

// Safe DOM helpers
function byId(id) { return document.getElementById(id); }
const isInModal = !!byId('infoModal');

// Wire event buttons only if present (works in standalone page and in modal)
[
  { id: 'eventBtn1', fn: 0 },
  { id: 'eventBtn2', fn: 1 },
  { id: 'eventBtn3', fn: 2 },
  { id: 'eventBtn4', fn: 4 },
].forEach(({ id, fn }) => {
  const el = byId(id);
  if (el && !el._wired) {
    el.addEventListener('click', function() {
      fetch('/api/info_events', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ loco_id: locoId, function: fn, value: 1 })
      });
    });
    el._wired = true;
  }
});

// Back/Close handler: prefer closing modal if available, else navigate home
(function(){
  const back = byId('backBtn') || byId('infoModalClose');
  if (back && !back._wired) {
    back.addEventListener('click', function() {
      if (isInModal && byId('infoModal')) {
        byId('infoModal').classList.add('hidden');
      } else {
        window.location.href = '/';
      }
    });
    back._wired = true;
  }
})();

// Dynamically fetch and display version/backend information
(async function loadHealth() {
  try {
    const res = await fetch('/api/health', { cache: 'no-store' });
    if (!res.ok) throw new Error('health fetch failed');
    const data = await res.json();
    const ver = (data && (data.version || data.Version)) || 'unknown';
    const dv = document.getElementById('appVersion');
    if (dv) dv.textContent = ver;
    // Heuristic backend type: Python returns system_state as an enum/str; C++ returns plain string too; we can add hint by checking headers in future
    const backend = data && typeof data.system_state !== 'undefined' ? 'active' : 'unknown';
    const db = document.getElementById('backendType');
    if (db) db.textContent = `HTTP OK (${backend})`;
  } catch (e) {
    const dv = document.getElementById('appVersion');
    if (dv) dv.textContent = 'unavailable';
    const db = document.getElementById('backendType');
    if (db) db.textContent = 'unavailable';
  }
})();

