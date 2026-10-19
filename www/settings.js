'use strict';

// Settings and info page: an overview (printer, WiFi, access point, system), then four areas as tabs:
// the printer (acts at once), the network and the system (collected in one save bar, saving restarts
// labelbee), and the live log.

// German texts of this page (English is the source, see i18n.js).
I18N.add({
  'Open source licences': 'Open-Source-Lizenzen',
  'The new firmware {rejected} did not start; {version} runs again.':
    'Die neue Firmware {rejected} ist nicht gestartet; {version} läuft wieder.',
  'Europe/Berlin (CET/CEST)': 'Europe/Berlin (MEZ/MESZ)', 'Europe/Helsinki (EET/EEST)': 'Europe/Helsinki (OEZ/OESZ)',
  // settings.html
  'labelbee · Settings': 'labelbee · Einstellungen',
  'Settings': 'Einstellungen',
  'Label': 'Etikett',
  'Overview': 'Überblick',
  'Printer': 'Drucker',
  'WiFi': 'WLAN',
  'Access point': 'Access Point',
  'Sections': 'Bereiche',
  'Network': 'Netzwerk',
  'Printer switched on': 'Drucker eingeschaltet',
  'Normally always on. Switch it off only when there are problems; after a restart it is on again.':
    'Normalerweise immer an. Aus nur bei Problemen; nach einem Neustart ist er wieder an.',
  'State': 'Zustand',
  'Tape': 'Band',
  'Battery': 'Akku',
  'Test label': 'Testetikett',
  'Prints a short pattern to check the printer and the tape.': 'Druckt ein kurzes Muster, um Drucker und Band zu prüfen.',
  'Print test label': 'Testetikett drucken',
  'Network name (SSID)': 'Netzwerkname (SSID)',
  'Search networks': 'Netze suchen',
  'Password': 'Passwort',
  'Open network (no password)': 'Offenes Netz (kein Passwort)',
  'IP address': 'IP-Adresse',
  'Automatic (DHCP)': 'Automatisch (DHCP)',
  'Static': 'Statisch',
  'Netmask': 'Netzmaske',
  '255.255.255.0 or /24': '255.255.255.0 oder /24',
  'empty = gateway': 'leer = Gateway',
  'labelbee always opens its own WiFi, in addition to the home network. Through it, labelbee can also be reached without the home network at':
    'labelbee öffnet immer ein eigenes WLAN, zusätzlich zum Heimnetz. Darüber ist es auch ohne Heimnetz erreichbar unter',
  'Open (no password)': 'Offen (kein Passwort)',
  'Device': 'Gerät',
  'Device name': 'Gerätename',
  'Reachable as': 'Erreichbar als',
  'Time': 'Zeit',
  'NTP server': 'NTP-Server',
  'Time zone': 'Zeitzone',
  'POSIX time zone': 'POSIX-Zeitzone',
  'Installed:': 'Installiert:',
  'Choose file': 'Datei wählen',
  'Install firmware': 'Firmware aufspielen',
  'Danger zone': 'Gefahrenbereich',
  'Restart': 'Neu starten',
  'Restarts labelbee; settings and files are kept.': 'Startet labelbee neu, Einstellungen und Dateien bleiben.',
  'Factory settings': 'Werkseinstellungen',
  'WiFi, access point, name and time back to the factory state.':
    'WLAN, Access Point, Name und Zeit zurück auf den Auslieferungszustand.',
  'Reset …': 'Zurücksetzen …',
  'All': 'Alle',
  'Warnings': 'Warnungen',
  'Errors': 'Fehler',
  'Copy': 'Kopieren',
  'labelbee restarts afterwards (about 10 s).': 'labelbee startet danach neu (ca. 10 s).',
  'Discard': 'Verwerfen',
  'Save and restart': 'Speichern und neu starten',
  'Reset to factory settings?': 'Auf Werkseinstellungen zurücksetzen?',
  'WiFi, access point, device name and time are erased. Afterwards labelbee only starts its own WiFi':
    'WLAN, Access Point, Gerätename und Zeit werden gelöscht. labelbee startet danach nur mit seinem eigenen WLAN',
  '"labelbee"': '„labelbee“',
  'password': 'Passwort',
  'at': 'unter',
  'Also delete all saved labels': 'Auch alle gespeicherten Etiketten löschen',
  'Cancel': 'Abbrechen',
  'Reset': 'Zurücksetzen',

  // settings.js
  '"{text}"': '„{text}“',
  '{d} d {h} h': '{d} d {h} h',
  '{h} h {m} min': '{h} h {m} min',
  '{m} min': '{m} min',
  'weak': 'schwach',
  'fair': 'mäßig',
  'good': 'gut',
  'very good': 'sehr gut',
  'switched off': 'ausgeschaltet',
  'starting …': 'startet …',
  'not responding': 'meldet sich nicht',
  'printing …': 'druckt …',
  'ready': 'bereit',
  'unknown': 'unbekannt',
  'Printer {state}': 'Drucker {state}',
  'inserted': 'eingelegt',
  'no tape': 'kein Band',
  'tape inserted': 'Band eingelegt',
  '{tape} · {percent} % charging': '{tape} · {percent} % lädt',
  'switch it on under Printer': 'im Bereich Drucker einschalten',
  'Model': 'Modell',
  '{percent} % · {volts} · charging': '{percent} % · {volts} · lädt',
  'connecting …': 'verbindet …',
  'not set up': 'nicht eingerichtet',
  'set it up under Network': 'im Bereich Netzwerk einrichten',
  'Address': 'Adresse',
  '{rssi} dBm · channel {channel}': '{rssi} dBm · Kanal {channel}',
  '1 device': '1 Gerät',
  '{n} devices': '{n} Geräte',
  'open': 'offen',
  'Protection': 'Schutz',
  'open, no password': 'offen, ohne Passwort',
  'WPA2 with password': 'WPA2 mit Passwort',
  'Connected': 'Verbunden',
  'via NTP': 'per NTP',
  'from the browser': 'vom Browser',
  'not set': 'nicht gestellt',
  'running for {duration}': 'läuft seit {duration}',
  'Built': 'Gebaut',
  'Clock': 'Uhrzeit',
  'Memory': 'Arbeitsspeicher',
  '{free} KB free (min. {min} KB) · PSRAM {psram} KB': '{free} KB frei (min. {min} KB) · PSRAM {psram} KB',
  'File storage': 'Dateispeicher',
  '{used} of {total} KB used': '{used} von {total} KB belegt',
  'Clock: {time} · set {source}': 'Uhr: {time} · gestellt {source}',
  'Clock: not set yet (no NTP and no time from the browser yet)': 'Uhr: noch nicht gestellt (kein NTP und noch keine Zeit vom Browser)',
  'not reachable': 'nicht erreichbar',
  'Connecting …': 'Verbinde …',
  'Printer switched off': 'Drucker ausgeschaltet',
  'Printed': 'Gedruckt',
  'Failed': 'Fehlgeschlagen',
  'unchanged': 'unverändert',
  'no password stored': 'kein Passwort gespeichert',
  'currently open': 'zurzeit offen',
  'Custom (POSIX)': 'Eigene (POSIX)',
  'Please enter the network name.': 'Bitte den Netzwerknamen angeben.',
  'A WPA password has at least 8 characters.': 'Ein WPA-Passwort hat mindestens 8 Zeichen.',
  'Please enter the WiFi password (or choose "Open network").': 'Bitte das WLAN-Passwort eingeben (oder „Offenes Netz“ wählen).',
  'Not a valid IP address, e.g. 192.168.1.50.': 'Keine gültige IP-Adresse, z. B. 192.168.1.50.',
  'E.g. 255.255.255.0 or /24.': 'Z. B. 255.255.255.0 oder /24.',
  'Not a valid address.': 'Keine gültige Adresse.',
  'Please enter a name.': 'Bitte einen Namen angeben.',
  'At least 8 characters (WPA2).': 'Mindestens 8 Zeichen (WPA2).',
  'Please set a password or leave it "Open".': 'Bitte ein Passwort festlegen oder „Offen“ lassen.',
  'Only lower-case letters, digits and hyphens (not at the start or end), up to 32 characters.':
    'Nur Kleinbuchstaben, Ziffern und Bindestriche (nicht am Anfang oder Ende), bis 32 Zeichen.',
  'Please enter a POSIX time zone.': 'Bitte eine POSIX-Zeitzone angeben.',
  '1 unsaved change': '1 ungespeicherte Änderung',
  '{n} unsaved changes': '{n} ungespeicherte Änderungen',
  'Hide password': 'Passwort verbergen',
  'Show password': 'Passwort anzeigen',
  'Searching for networks … (a few seconds)': 'Suche Netze … (einige Sekunden)',
  'No networks found.': 'Keine Netze gefunden.',
  '{signal}, channel {channel}': '{signal}, Kanal {channel}',
  'Saved. labelbee is restarting …': 'Gespeichert. labelbee startet neu …',
  'Afterwards reachable at http://{name}.local/ or via the WiFi "{ap}" at http://192.168.4.1/.':
    'Danach erreichbar unter http://{name}.local/ oder über das WLAN „{ap}“ unter http://192.168.4.1/.',
  'Could not load the settings: {error}': 'Einstellungen nicht ladbar: {error}',
  'labelbee is not coming back': 'labelbee meldet sich nicht zurück',
  '{text} Reload the page later.': '{text} Seite später neu laden.',
  'No .bin file chosen': 'Keine .bin-Datei gewählt',
  'Update written, labelbee is restarting …': 'Update geschrieben, labelbee startet neu …',
  'The page reloads by itself afterwards.': 'Die Seite lädt danach von selbst neu.',
  'Update failed: {error}': 'Update fehlgeschlagen: {error}',
  'Update failed: no connection.': 'Update fehlgeschlagen: keine Verbindung.',
  'Restarted: version {version} is running.': 'Neu gestartet: Version {version} läuft.',
  'Restarted, version unchanged ({version}).': 'Neu gestartet, Version unverändert ({version}).',
  'Really restart?': 'Wirklich neu starten?',
  'labelbee is restarting …': 'labelbee startet neu …',
  'Reset, labelbee is restarting': 'Zurückgesetzt, labelbee startet neu',
  'Connect to the WiFi "labelbee" (password labelbee) and open http://192.168.4.1/settings to set it up again.':
    'Verbinde dich mit dem WLAN „labelbee“ (Passwort labelbee) und öffne http://192.168.4.1/settings, um es neu einzurichten.',
  '{ms} ms after start': '{ms} ms nach dem Start',
  'Pause': 'Anhalten',
  'Resume': 'Fortsetzen',
  'Log copied': 'Log kopiert',
  'Log selected, copy it with Ctrl+C': 'Log markiert, mit Strg+C kopieren',
});

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => [...document.querySelectorAll(sel)];

// Common time zones as POSIX TZ strings (what the firmware's libc understands).
const TIME_ZONES = [
  ['Europe/Berlin (CET/CEST)', 'CET-1CEST,M3.5.0,M10.5.0/3'],
  ['Europe/London (GMT/BST)', 'GMT0BST,M3.5.0/1,M10.5.0'],
  ['Europe/Lisbon (WET/WEST)', 'WET0WEST,M3.5.0/1,M10.5.0'],
  ['Europe/Helsinki (EET/EEST)', 'EET-2EEST,M3.5.0/3,M10.5.0/4'],
  ['Europe/Moscow (MSK)', 'MSK-3'],
  ['UTC', 'UTC0'],
  ['America/New_York (EST/EDT)', 'EST5EDT,M3.2.0,M11.1.0'],
  ['America/Chicago (CST/CDT)', 'CST6CDT,M3.2.0,M11.1.0'],
  ['America/Denver (MST/MDT)', 'MST7MDT,M3.2.0,M11.1.0'],
  ['America/Los_Angeles (PST/PDT)', 'PST8PDT,M3.2.0,M11.1.0'],
  ['Asia/Tokyo (JST)', 'JST-9'],
  ['Australia/Sydney (AEST/AEDT)', 'AEST-10AEDT,M10.1.0,M4.1.0/3'],
];
const CUSTOM_TZ = 'custom';
const TABS = ['drucker', 'netzwerk', 'system', 'log'];   // also the URL hashes, kept for old links
const UPDATE_KEY = 'labelbee.updateFrom';   // sessionStorage: the version before a firmware update

const ICON = {
  eye: '\u{F0208}', eyeOff: '\u{F0209}', lock: '\u{F033E}', pause: '\u{F03E4}', play: '\u{F040A}',
  wifi: ['\u{F092F}', '\u{F091F}', '\u{F0922}', '\u{F0925}', '\u{F0928}'], wifiOff: '\u{F092E}',
};

let settings = null;      // as loaded from the device
let original = null;      // the form state that matches it
let info = null;          // the last /api/info
let reachable = null;     // whether the last /api/info answered (null: not asked yet)
let restartView = null;   // what the restart overlay shows, kept for a language switch
let logSince = 0;
let timeOffered = false;  // sent the browser's clock once

// ---------- Helpers ----------

async function api(path, options) {
  const response = await fetch(path, options);
  const body = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(I18N.server(body.error || body.message) || `HTTP ${response.status}`);
  return body;
}

const postJson = (path, data) => api(path, {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify(data),
});

let toastTimer;
function toast(message, isError = false) {
  const el = $('#toast');
  el.textContent = message;
  el.classList.toggle('error', isError);
  el.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove('show'), isError ? 6000 : 4000);
}

function isIPv4(s) {
  const parts = s.split('.');
  return parts.length === 4 && parts.every((p) => /^\d{1,3}$/.test(p) && Number(p) <= 255);
}

/** "255.255.255.0", "/24" or "24" -> 24; null if it is not a valid netmask. */
function parsePrefix(s) {
  s = s.trim().replace(/^\//, '');
  if (/^\d{1,2}$/.test(s)) return Number(s) <= 32 ? Number(s) : null;
  if (!isIPv4(s)) return null;
  const bits = s.split('.').map(Number).reduce((acc, b) => acc * 256 + b, 0);
  const ones = bits.toString(2).padStart(32, '0');
  return /^1*0*$/.test(ones) ? ones.indexOf('0') === -1 ? 32 : ones.indexOf('0') : null;
}

function prefixToMask(prefix) {
  const bits = prefix === 0 ? 0 : (0xFFFFFFFF << (32 - prefix)) >>> 0;
  return [24, 16, 8, 0].map((s) => (bits >>> s) & 255).join('.');
}

function formatDuration(s) {
  const d = Math.floor(s / 86400), h = Math.floor(s / 3600) % 24, m = Math.floor(s / 60) % 60;
  if (d) return t('{d} d {h} h', { d, h });
  if (h) return t('{h} h {m} min', { h, m });
  return t('{m} min', { m });
}

const clock = (ms) => new Date(ms).toLocaleTimeString(I18N.locale);
const dateTime = (ms) => new Date(ms).toLocaleString(I18N.locale, { dateStyle: 'medium', timeStyle: 'medium' });
/** A name in the quotation marks of the current language. */
const quote = (text) => t('"{text}"', { text });

/** 1 (weak) to 4 (very good) bars from the signal strength. */
function signalBars(rssi) {
  return rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : 1;
}
const signalWord = (bars) => [null, t('weak'), t('fair'), t('good'), t('very good')][bars];

function setFacts(dl, rows) {
  dl.replaceChildren(...rows.filter(Boolean).flatMap(([k, v]) => {
    const dt = document.createElement('dt');
    dt.textContent = k;
    const dd = document.createElement('dd');
    dd.textContent = v;
    return [dt, dd];
  }));
}

// ---------- Tabs ----------

function showTab(name, focus = false) {
  if (!TABS.includes(name)) name = TABS[0];
  for (const tab of TABS) {
    const button = $(`#tab-${tab}`);
    button.setAttribute('aria-selected', String(tab === name));
    button.tabIndex = tab === name ? 0 : -1;
    $(`#panel-${tab}`).hidden = tab !== name;
  }
  if (focus) $(`#tab-${name}`).focus();
  if (location.hash.slice(1) !== name) history.replaceState(null, '', `#${name}`);
  if (name === 'log') scrollLogToEnd();
}

for (const tab of TABS) $(`#tab-${tab}`).addEventListener('click', () => showTab(tab));
$('.settings-nav').addEventListener('keydown', (e) => {
  const i = TABS.indexOf(location.hash.slice(1));
  const step = { ArrowDown: 1, ArrowRight: 1, ArrowUp: -1, ArrowLeft: -1 }[e.key];
  if (!step) return;
  e.preventDefault();
  showTab(TABS[(i + step + TABS.length) % TABS.length], true);
});
window.addEventListener('hashchange', () => showTab(location.hash.slice(1)));

// ---------- Overview and printer ----------

function printerText(p) {
  switch (p?.state) {
    case 'off': return ['dry', t('switched off')];
    case 'starting': return ['unknown', t('starting …')];
    case 'missing': return ['missing', t('not responding')];
    case 'ready': return ['ok', p.busy ? t('printing …') : t('ready')];
    default: return ['unknown', t('unknown')];
  }
}

function renderPrinter(p) {
  const [mode, text] = printerText(p);
  const status = $('#printer-status');
  status.dataset.state = mode;
  status.querySelector('.label').textContent = t('Printer {state}', { state: text });

  const known = p?.state === 'ready' && p.statusKnown;
  const cassette = known ? (p.cassette ? t('inserted') : t('no tape')) : '–';
  const percent = known && p.batteryMv ? p.batteryPercent : null;
  const volts = known && p.batteryMv
    ? `${(p.batteryMv / 1000).toLocaleString(I18N.locale, { minimumFractionDigits: 2, maximumFractionDigits: 2 })} V` : '';
  const battery = percent == null ? '–'
    : p.charging ? t('{percent} % · {volts} · charging', { percent, volts }) : `${percent} % · ${volts}`;

  $('#tp-value').textContent = text;
  $('#tile-printer').dataset.state = mode;
  const tape = p?.cassette ? t('tape inserted') : t('no tape');
  $('#tp-sub').textContent = known
    ? (p.charging ? t('{tape} · {percent} % charging', { tape, percent: percent ?? '?' }) : `${tape} · ${percent ?? '?'} %`)
    : (p?.power ? '' : t('switch it on under Printer'));
  setFacts($('#tp-facts'), [
    [t('Model'), p?.model || '–'],
    [t('State'), text],
    [t('Tape'), cassette],
    [t('Battery'), battery],
  ]);

  $('#p-power').checked = !!p?.power;
  $('#p-state').textContent = `${p?.model || t('Printer')}: ${text}`;
  $('#p-cassette').textContent = cassette;
  $('#p-meter').value = percent ?? 0;
  $('#p-meter').hidden = percent == null;
  $('#p-battery').textContent = battery;
  $('#p-test').disabled = p?.state !== 'ready';
}

/** The header status when /api/info has not answered (yet). */
function renderUnreachable() {
  const status = $('#printer-status');
  status.dataset.state = reachable === false ? 'missing' : 'unknown';
  status.querySelector('.label').textContent = reachable === false ? t('not reachable') : t('Connecting …');
}

function renderInfo(i) {
  // log lines from before the clock was set: give them clock times now
  if (i.bootEpochMs && !info?.bootEpochMs) for (const el of $$('#log time[data-ms]')) el.textContent = clock(i.bootEpochMs + Number(el.dataset.ms));
  info = i;
  // WiFi
  const w = i.wifi || {};
  const tw = $('#tile-wifi');
  if (w.state === 'connected') {
    const bars = signalBars(w.rssi);
    $('#tw-icon').textContent = ICON.wifi[bars];
    $('#tw-value').textContent = `${signalWord(bars)} · ${w.rssi} dBm`;
    $('#tw-sub').textContent = `${quote(w.ssid)} · ${w.ip}`;
    tw.dataset.state = bars >= 2 ? 'ok' : 'warn';
  } else {
    $('#tw-icon').textContent = ICON.wifiOff;
    $('#tw-value').textContent = w.state === 'connecting' ? t('connecting …') : t('not set up');
    $('#tw-sub').textContent = w.ssid ? quote(w.ssid) : t('set it up under Network');
    tw.dataset.state = 'warn';
  }
  setFacts($('#tw-facts'), [
    [t('Network'), w.ssid ? quote(w.ssid) : '–'],
    w.state === 'connected' && [t('Address'), `${w.ip}/${w.prefix}`],
    w.state === 'connected' && ['Gateway', w.gateway || '–'],
    w.state === 'connected' && ['DNS', w.dns || '–'],
    w.state === 'connected' && ['Signal', t('{rssi} dBm · channel {channel}', { rssi: w.rssi, channel: w.channel })],
    w.state === 'connected' && [t('Access point'), w.bssid],
    ['MAC', w.mac],
  ]);

  // Access point
  const a = i.ap || {};
  const open = settings ? !settings.ap.hasPassword : null;
  const clients = a.clients ?? 0;
  $('#ta-value').textContent = clients === 1 ? t('1 device') : t('{n} devices', { n: clients });
  $('#ta-sub').textContent = [quote(a.ssid), open == null ? null : open ? t('open') : 'WPA2'].filter(Boolean).join(' · ');
  setFacts($('#ta-facts'), [
    ['Name', quote(a.ssid)],
    [t('Address'), `http://${a.ip}/`],
    open != null && [t('Protection'), open ? t('open, no password') : t('WPA2 with password')],
    [t('Connected'), `${clients}`],
  ]);

  // System
  const source = { ntp: t('via NTP'), browser: t('from the browser'), none: t('not set') }[i.timeSource] || '';
  $('#ts-value').textContent = t('running for {duration}', { duration: formatDuration(i.uptimeS) });
  $('#ts-sub').textContent = i.version;
  setFacts($('#ts-facts'), [
    ['Firmware', i.version],
    [t('Built'), i.built],
    [t('Clock'), i.time ? `${i.time} (${source})` : source],
    [t('Memory'), t('{free} KB free (min. {min} KB) · PSRAM {psram} KB',
      { free: i.ram.internalFreeKb, min: i.ram.internalMinKb, psram: i.ram.psramFreeKb })],
    [t('File storage'), t('{used} of {total} KB used', { used: i.storage.usedKb, total: i.storage.totalKb })],
  ]);
  $('#f-version').textContent = `${i.version} (${i.built})`;
  $('#t-now').textContent = i.bootEpochMs
    ? t('Clock: {time} · set {source}', { time: dateTime(i.bootEpochMs + i.uptimeS * 1000), source })
    : t('Clock: not set yet (no NTP and no time from the browser yet)');

  renderPrinter(i.printer);
  offerBrowserTime(i);
}

/** Without NTP (no internet, e.g. only the access point) the device takes the browser's clock. */
async function offerBrowserTime(i) {
  if (timeOffered || i.timeSource === 'ntp' || i.timeSource === 'browser') return;
  timeOffered = true;
  try {
    const r = await api(`/api/time?ms=${Date.now()}`, { method: 'POST' });
    if (r.taken) refreshInfo();
  } catch { /* older firmware */ }
}

async function refreshInfo() {
  try {
    renderInfo(await api('/api/info'));
    reachable = true;
    return true;
  } catch {
    reachable = false;
    renderUnreachable();
    return false;
  }
}

$('#p-power').addEventListener('change', async (e) => {
  const on = e.target.checked;
  try {
    const status = await api(`/api/power?on=${on ? 1 : 0}`, { method: 'POST' });
    renderPrinter(status.printer);
    toast(on ? t('Printer switched on') : t('Printer switched off'));
  } catch (err) {
    e.target.checked = !on;
    toast(err.message, true);
  }
});

$('#p-test').addEventListener('click', async () => {
  const button = $('#p-test');
  button.disabled = true;
  try {
    const r = await api(`/api/test-print?tape=${encodeURIComponent($('#p-tape').value)}`, { method: 'POST' });
    toast(r.message ? I18N.server(r.message) : (r.ok ? t('Printed') : t('Failed')), !r.ok);
  } catch (e) {
    toast(e.message, true);
  } finally {
    button.disabled = false;
  }
});

// ---------- Settings forms ----------

/** Placeholders and option texts that depend on the stored settings; leaves the user's input alone. */
function renderFormTexts() {
  if (!settings) return;
  $('#w-password').placeholder = settings.wifi.hasPassword ? t('unchanged') : t('no password stored');
  $('#a-password').placeholder = settings.ap.hasPassword ? t('unchanged') : t('currently open');
  const custom = $(`#t-zone option[value="${CUSTOM_TZ}"]`);
  if (custom) custom.textContent = t('Custom (POSIX)');
}

function fillForms() {
  const w = settings.wifi, a = settings.ap, tm = settings.time;
  $('#w-ssid').value = w.ssid || '';
  $('#w-password').value = '';
  $('#w-nopass').checked = !!w.ssid && !w.hasPassword;
  for (const r of $$('input[name="w-mode"]')) r.checked = r.value === (w.mode === 'static' ? 'static' : 'dhcp');
  $('#w-ip').value = w.ip || '';
  $('#w-mask').value = w.prefix ? prefixToMask(w.prefix) : '';
  $('#w-gateway').value = w.gateway || '';
  $('#w-dns1').value = w.dns1 || '';
  $('#w-dns2').value = w.dns2 || '';

  $('#a-ssid').value = a.ssid || '';
  $('#a-password').value = '';
  $('#a-nopass').checked = !a.hasPassword;

  $('#t-ntp').value = tm.ntp || '';
  const zone = $('#t-zone');
  zone.replaceChildren(...TIME_ZONES.map(([name, tz]) => new Option(t(name), tz)), new Option('', CUSTOM_TZ));
  const known = TIME_ZONES.some(([, tz]) => tz === tm.tz);
  zone.value = known ? tm.tz : CUSTOM_TZ;
  $('#t-custom').value = known ? '' : tm.tz || '';

  $('#d-name').value = settings.device.name || '';
  renderFormTexts();
  for (const id of ['w-password', 'a-password']) setPasswordVisible(id, false);
  $('#w-scan').hidden = true;
  original = formState();
  updateForm();
}

const wifiMode = () => $('input[name="w-mode"]:checked')?.value || 'dhcp';
const timeZone = () => ($('#t-zone').value === CUSTOM_TZ ? $('#t-custom').value.trim() : $('#t-zone').value);

/** The forms as flat key -> value, comparable with `original`. */
function formState() {
  const s = {
    'device.name': $('#d-name').value.trim().toLowerCase(),
    'wifi.ssid': $('#w-ssid').value.trim(),
    'wifi.password': $('#w-password').value,
    'wifi.open': $('#w-nopass').checked,
    'wifi.mode': wifiMode(),
    'ap.ssid': $('#a-ssid').value.trim(),
    'ap.password': $('#a-password').value,
    'ap.open': $('#a-nopass').checked,
    'time.ntp': $('#t-ntp').value.trim(),
    'time.tz': timeZone(),
  };
  if (s['wifi.mode'] === 'static') {
    Object.assign(s, {
      'wifi.ip': $('#w-ip').value.trim(), 'wifi.mask': $('#w-mask').value.trim(), 'wifi.gateway': $('#w-gateway').value.trim(),
      'wifi.dns1': $('#w-dns1').value.trim(), 'wifi.dns2': $('#w-dns2').value.trim(),
    });
  }
  return s;
}

function changedKeys() {
  if (!original) return [];
  const now = formState();
  return Object.keys(now).filter((k) => now[k] !== (original[k] ?? (k.startsWith('wifi.') ? now[k] : undefined))
    && !(k.endsWith('.password') && now[k] === ''));
}

/** Field errors as you type; returns the first message (or ''). */
function validate() {
  const errors = {};
  const s = formState();
  if (!s['wifi.ssid'] && (s['wifi.password'] || s['wifi.mode'] === 'static')) errors['w-ssid'] = t('Please enter the network name.');
  if (!s['wifi.open'] && s['wifi.password'] && s['wifi.password'].length < 8) errors['w-password'] = t('A WPA password has at least 8 characters.');
  if (!s['wifi.open'] && !s['wifi.password'] && s['wifi.ssid'] && original && (original['wifi.open'] || !settings.wifi.hasPassword)
      && (original['wifi.open'] !== s['wifi.open'] || original['wifi.ssid'] !== s['wifi.ssid'])) {
    errors['w-password'] = t('Please enter the WiFi password (or choose "Open network").');
  }
  if (s['wifi.mode'] === 'static') {
    if (!isIPv4(s['wifi.ip'])) errors['w-ip'] = t('Not a valid IP address, e.g. 192.168.1.50.');
    const prefix = parsePrefix(s['wifi.mask']);
    if (prefix === null || prefix < 1 || prefix > 30) errors['w-mask'] = t('E.g. 255.255.255.0 or /24.');
    if (!isIPv4(s['wifi.gateway'])) errors['w-gateway'] = t('Not a valid address.');
    if (s['wifi.dns1'] && !isIPv4(s['wifi.dns1'])) errors['w-dns1'] = t('Not a valid address.');
    if (s['wifi.dns2'] && !isIPv4(s['wifi.dns2'])) errors['w-dns2'] = t('Not a valid address.');
  }
  if (!s['ap.ssid']) errors['a-ssid'] = t('Please enter a name.');
  if (!s['ap.open'] && s['ap.password'] && s['ap.password'].length < 8) errors['a-password'] = t('At least 8 characters (WPA2).');
  if (!s['ap.open'] && !s['ap.password'] && original && original['ap.open']) errors['a-password'] = t('Please set a password or leave it "Open".');
  if (!/^[a-z0-9]([a-z0-9-]{0,30}[a-z0-9])?$/.test(s['device.name'])) {
    errors['d-name'] = t('Only lower-case letters, digits and hyphens (not at the start or end), up to 32 characters.');
  }
  if ($('#t-zone').value === CUSTOM_TZ && !s['time.tz']) errors['t-custom'] = t('Please enter a POSIX time zone.');
  for (const el of $$('.field-error')) {
    const id = el.id.replace(/^e-/, '');
    const touched = document.getElementById(id)?.dataset.touched || validate.all;
    el.textContent = touched ? errors[id] || '' : '';
  }
  return Object.entries(errors);
}

/** Show/hide dependent fields, mark changed ones, update the save bar. */
function updateForm() {
  $('#w-static').hidden = wifiMode() !== 'static';
  $('#w-password').disabled = $('#w-nopass').checked;
  $('#a-password').disabled = $('#a-nopass').checked;
  $('#t-custom-field').hidden = $('#t-zone').value !== CUSTOM_TZ;
  $('#d-host').textContent = `${$('#d-name').value.trim().toLowerCase() || 'labelbee'}.local`;
  const changed = changedKeys();
  for (const el of $$('[data-key]')) el.classList.toggle('changed', changed.includes(el.dataset.key));
  for (const tab of ['netzwerk', 'system']) {
    const panel = $(`#panel-${tab}`);
    $(`#tab-${tab}`).classList.toggle('has-changes', changed.some((k) => panel.querySelector(`[data-key="${k}"]`)));
  }
  $('#save-bar').hidden = changed.length === 0;
  document.body.classList.toggle('has-save-bar', changed.length > 0);
  $('#save-count').textContent = changed.length === 1 ? t('1 unsaved change') : t('{n} unsaved changes', { n: changed.length });
  validate();
}

for (const el of $$('#panel-netzwerk input, #panel-netzwerk select, #panel-system input:not([type="file"]), #panel-system select')) {
  el.addEventListener('input', () => { el.dataset.touched = '1'; updateForm(); });
  el.addEventListener('change', () => { el.dataset.touched = '1'; updateForm(); });
}
for (const form of $$('#wifi-form, #ap-form, #time-form, #device-form')) form.addEventListener('submit', (e) => e.preventDefault());

function setPasswordVisible(id, visible) {
  const input = document.getElementById(id);
  const eye = $(`.pw-eye[data-for="${id}"]`);
  input.type = visible ? 'text' : 'password';
  eye.textContent = visible ? ICON.eyeOff : ICON.eye;
  eye.setAttribute('aria-label', visible ? t('Hide password') : t('Show password'));
}
for (const eye of $$('.pw-eye')) {
  eye.addEventListener('click', () => setPasswordVisible(eye.dataset.for, document.getElementById(eye.dataset.for).type === 'password'));
}

/** One grey (or red) line in the scan list. */
function scanNote(text, isError = false) {
  const li = document.createElement('li');
  li.className = isError ? 'scan-note error' : 'scan-note';
  li.textContent = text;
  return li;
}

// WiFi scan: pick a network from the list
$('#w-scan-btn').addEventListener('click', async () => {
  const button = $('#w-scan-btn');
  const list = $('#w-scan');
  button.disabled = true;
  list.hidden = false;
  list.replaceChildren(scanNote(t('Searching for networks … (a few seconds)')));
  try {
    const nets = await api('/api/scan');
    list.replaceChildren();
    if (!nets.length) list.append(scanNote(t('No networks found.')));
    for (const n of nets) {
      const li = document.createElement('li');
      const b = document.createElement('button');
      b.type = 'button';
      const bars = signalBars(n.rssi);
      b.innerHTML = '<span class="mdi-glyph sig"></span><span class="ssid"></span><span class="meta"></span>';
      b.querySelector('.sig').textContent = ICON.wifi[bars];
      b.querySelector('.ssid').textContent = n.ssid;
      b.querySelector('.meta').textContent = `${n.open ? t('open') : ICON.lock} ${n.rssi} dBm`;
      b.title = t('{signal}, channel {channel}', { signal: signalWord(bars), channel: n.channel });
      if (n.ssid === $('#w-ssid').value.trim()) b.classList.add('current');
      b.addEventListener('click', () => {
        $('#w-ssid').value = n.ssid;
        $('#w-nopass').checked = n.open;
        list.hidden = true;
        $('#w-ssid').dataset.touched = '1';
        updateForm();
        if (!n.open) $('#w-password').focus();
      });
      li.append(b);
      list.append(li);
    }
  } catch (e) {
    list.replaceChildren(scanNote(e.message, true));
  } finally {
    button.disabled = false;
  }
});

$('#save-discard').addEventListener('click', () => {
  for (const el of $$('[data-touched]')) delete el.dataset.touched;
  fillForms();
});

/** The changed sections as the firmware expects them (POST /api/settings, partial). */
function payload(changed) {
  const s = formState();
  const data = {};
  const has = (prefix) => changed.some((k) => k.startsWith(prefix));
  if (has('wifi.')) {
    const w = { ssid: s['wifi.ssid'], mode: s['wifi.mode'] };
    if (s['wifi.open']) w.password = '';
    else if (s['wifi.password']) w.password = s['wifi.password'];
    if (w.mode === 'static') {
      Object.assign(w, { ip: s['wifi.ip'], prefix: parsePrefix(s['wifi.mask']), gateway: s['wifi.gateway'],
        dns1: s['wifi.dns1'], dns2: s['wifi.dns2'] });
    }
    data.wifi = w;
  }
  if (has('ap.')) {
    data.ap = { ssid: s['ap.ssid'] };
    if (s['ap.open']) data.ap.password = '';
    else if (s['ap.password']) data.ap.password = s['ap.password'];
  }
  if (has('time.')) data.time = { ntp: s['time.ntp'] || 'pool.ntp.org', tz: s['time.tz'] };
  if (has('device.')) data.device = { name: s['device.name'] };
  return data;
}

$('#save-apply').addEventListener('click', async () => {
  validate.all = true;
  const errors = validate();
  validate.all = false;
  if (errors.length) {
    const field = document.getElementById(errors[0][0]);
    showTab(field.closest('.spanel').id.replace('panel-', ''));
    field.focus();
    toast(errors[0][1], true);
    return;
  }
  const changed = changedKeys();
  const data = payload(changed);
  const oldName = settings.device.name;
  const newName = data.device?.name || oldName;
  const button = $('#save-apply');
  button.disabled = true;
  try {
    await postJson('/api/settings', data);
  } catch (e) {
    // After a network change the answer may never arrive: the device is already switching.
    if (!(e instanceof TypeError)) {
      button.disabled = false;
      toast(e.message, true);
      return;
    }
  }
  button.disabled = false;
  // Where to find labelbee afterwards: a new name changes <name>.local, a new static address the IP.
  let target = null;
  if (location.hostname === `${oldName}.local` && newName !== oldName) target = `http://${newName}.local/settings`;
  if (data.wifi?.mode === 'static' && isIPv4(location.hostname) && location.hostname !== data.wifi.ip
      && location.hostname !== '192.168.4.1') target = `http://${data.wifi.ip}/settings`;
  const apName = data.ap?.ssid || settings.ap.ssid;
  waitForRestart('Saved. labelbee is restarting …',
    'Afterwards reachable at http://{name}.local/ or via the WiFi "{ap}" at http://192.168.4.1/.', { name: newName, ap: apName }, target);
});

async function loadSettings() {
  try {
    settings = await api('/api/settings');
    fillForms();
  } catch (e) {
    toast(t('Could not load the settings: {error}', { error: e.message }), true);
  }
}

// ---------- Restart: wait until labelbee is back, then reload ----------

/** Shows the restart overlay from `restartView` (English keys, translated here). */
function renderRestart() {
  if (!restartView) return;
  const v = restartView;
  const text = t(v.text, v.params);
  $('#restart-title').textContent = v.late ? t('labelbee is not coming back') : t(v.title, v.params);
  $('#restart-text').textContent = v.late ? t('{text} Reload the page later.', { text }) : text;
  $('#restart-overlay').hidden = false;
}

/** `title` and `text` are English keys for t(), filled with `params`. */
function waitForRestart(title, text, params = {}, target = null) {
  restartView = { title, text, params, late: false };
  renderRestart();
  if (target) {
    setTimeout(() => { location.href = target; }, 12000);
    return;
  }
  const before = info?.uptimeS ?? Infinity;
  const started = Date.now();
  const poll = async () => {
    try {
      const r = await fetch('/api/info', { cache: 'no-store' });
      const i = await r.json();
      if (i.uptimeS < before || i.uptimeS < 30) return location.reload();
    } catch { /* still restarting */ }
    if (Date.now() - started > 90000) {
      restartView.late = true;
      renderRestart();
      return;
    }
    setTimeout(poll, 2000);
  };
  setTimeout(poll, 4000);
}

// ---------- Firmware update ----------

function renderFileName() {
  const file = $('#f-file').files[0];
  $('#f-name').textContent = file ? `${file.name} (${Math.round(file.size / 1024)} KB)` : t('No .bin file chosen');
}

$('#f-file').addEventListener('change', () => {
  renderFileName();
  $('#f-update').disabled = !$('#f-file').files[0];
});

$('#f-update').addEventListener('click', () => {
  const file = $('#f-file').files[0];
  if (!file) return;
  const button = $('#f-update');
  button.disabled = true;
  $('#f-progress-row').hidden = false;
  const show = (p) => {
    $('#f-progress').value = p;
    $('#f-percent').textContent = `${Math.round(p)} %`;
  };
  show(0);
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota');
  xhr.setRequestHeader('Content-Type', 'application/octet-stream');
  xhr.upload.onprogress = (e) => { if (e.lengthComputable) show((e.loaded / e.total) * 100); };
  xhr.onload = () => {
    let body = {};
    try { body = JSON.parse(xhr.responseText); } catch { /* not JSON */ }
    if (xhr.status === 200 && body.ok !== false) {
      try { sessionStorage.setItem(UPDATE_KEY, info?.version || ''); } catch { /* storage off */ }
      waitForRestart('Update written, labelbee is restarting …', 'The page reloads by itself afterwards.');
    } else {
      toast(t('Update failed: {error}', { error: I18N.server(body.message || body.error) || xhr.status }), true);
      button.disabled = false;
    }
  };
  xhr.onerror = () => {
    toast(t('Update failed: no connection.'), true);
    button.disabled = false;
  };
  xhr.send(file);
});

/** After an update and reload: say which version now runs. */
function reportUpdate() {
  let from = null;
  try {
    from = sessionStorage.getItem(UPDATE_KEY);
    sessionStorage.removeItem(UPDATE_KEY);
  } catch { /* storage off */ }
  if (from === null || !info) return;
  if (info.version === from && info.rejectedVersion && info.rejectedVersion !== from) {
    // the bootloader went back: the new image restarted before its web server was up
    toast(t('The new firmware {rejected} did not start; {version} runs again.',
      { rejected: info.rejectedVersion, version: info.version }), true);
  } else {
    toast(info.version !== from
      ? t('Restarted: version {version} is running.', { version: info.version })
      : t('Restarted, version unchanged ({version}).', { version: info.version }));
  }
  showTab('system');
}

// ---------- Danger zone ----------

$('#d-restart').addEventListener('click', async () => {
  const button = $('#d-restart');
  const label = button.querySelector('[data-i18n]');
  if (!button.dataset.armed) {
    button.dataset.armed = '1';
    label.textContent = t('Really restart?');
    setTimeout(() => { delete button.dataset.armed; label.textContent = t('Restart'); }, 3000);
    return;
  }
  try {
    await api('/api/restart', { method: 'POST' });
  } catch { /* the device may close the connection while restarting */ }
  waitForRestart('labelbee is restarting …', 'The page reloads by itself afterwards.');
});

$('#d-reset').addEventListener('click', () => {
  $('#reset-files').checked = false;
  $('#reset-dialog').showModal();
});

$('#reset-dialog').addEventListener('close', async () => {
  if ($('#reset-dialog').returnValue !== 'reset') return;
  try {
    await api(`/api/factory-reset?files=${$('#reset-files').checked ? 1 : 0}`, { method: 'POST' });
  } catch { /* may restart before answering */ }
  restartView = {
    title: 'Reset, labelbee is restarting',
    text: 'Connect to the WiFi "labelbee" (password labelbee) and open http://192.168.4.1/settings to set it up again.',
    params: {},
    late: false,
  };
  renderRestart();
  $('#restart-overlay').classList.add('final');
});

// ---------- Log ----------

const LOG_MAX = 3000;
let logPaused = false;
let logPartial = '';

function logLine(raw) {
  const m = /^([EWIDV]) \((\d+)\) (.*)$/.exec(raw);
  const div = document.createElement('div');
  div.className = `lvl-${m ? m[1] : 'I'}`;
  const time = document.createElement('time');
  if (m) {
    const ms = Number(m[2]);
    time.textContent = info?.bootEpochMs ? clock(info.bootEpochMs + ms) : `+${(ms / 1000).toFixed(1)} s`;
    time.title = t('{ms} ms after start', { ms });
    time.dataset.ms = ms;
  }
  const text = document.createElement('span');
  text.textContent = m ? `${m[1]}  ${m[3]}` : raw;
  div.append(time, text);
  return div;
}

function scrollLogToEnd() {
  const log = $('#log');
  log.scrollTop = log.scrollHeight;
}

async function refreshLog() {
  try {
    const r = await fetch(`/api/log?since=${logSince}`);
    logSince = Number(r.headers.get('X-Next')) || logSince;
    const text = await r.text();
    if (!text) return;
    const lines = (logPartial + text).split('\n');
    logPartial = lines.pop();
    const log = $('#log');
    const atEnd = log.scrollTop + log.clientHeight >= log.scrollHeight - 8;
    const frag = document.createDocumentFragment();
    for (const line of lines) if (line.trim()) frag.append(logLine(line.replace(/\x1b\[[0-9;]*m/g, '')));
    log.append(frag);
    while (log.childElementCount > LOG_MAX) log.firstElementChild.remove();
    if (atEnd && !logPaused) scrollLogToEnd();
  } catch { /* offline for a moment */ }
}

for (const r of $$('input[name="log-filter"]')) {
  r.addEventListener('change', () => { $('#log').dataset.filter = r.value; scrollLogToEnd(); });
}

function renderPauseButton() {
  const b = $('#log-pause');
  b.querySelector('.mdi-glyph').textContent = logPaused ? ICON.play : ICON.pause;
  b.lastElementChild.textContent = logPaused ? t('Resume') : t('Pause');
  b.classList.toggle('active', logPaused);
}

$('#log-pause').addEventListener('click', () => {
  logPaused = !logPaused;
  renderPauseButton();
  if (!logPaused) scrollLogToEnd();
});

$('#log-copy').addEventListener('click', async () => {
  const filter = $('#log').dataset.filter;
  const keep = filter === 'error' ? ['lvl-E'] : filter === 'warn' ? ['lvl-E', 'lvl-W'] : null;
  const text = [...$('#log').children].filter((d) => !keep || keep.includes(d.className))
    .map((d) => `${d.firstChild.textContent} ${d.lastChild.textContent}`).join('\n');
  try {
    await navigator.clipboard.writeText(text);
    toast(t('Log copied'));
  } catch {
    const range = document.createRange();
    range.selectNodeContents($('#log'));
    getSelection().removeAllRanges();
    getSelection().addRange(range);
    toast(t('Log selected, copy it with Ctrl+C'));
  }
});

// ---------- Language ----------

/** Everything built in JavaScript; the static parts are done by I18N.apply(). */
function renderDynamicTexts() {
  if (info) renderInfo(info);
  if (!info || reachable === false) renderUnreachable();
  renderFormTexts();
  if (original) updateForm();
  for (const id of ['w-password', 'a-password']) setPasswordVisible(id, document.getElementById(id).type === 'text');
  renderFileName();
  renderPauseButton();
  renderRestart();
  if (info?.bootEpochMs) for (const el of $$('#log time[data-ms]')) el.textContent = clock(info.bootEpochMs + Number(el.dataset.ms));
}

I18N.onChange(() => {
  $('#w-scan').hidden = true;   // its texts were made in the old language
  renderDynamicTexts();
  for (const option of $('#t-zone').options) {
    const zone = TIME_ZONES.find(([, tz]) => tz === option.value);
    if (zone) option.textContent = t(zone[0]);
  }
  const restart = $('#d-restart');
  restart.querySelector('[data-i18n]').textContent = t(restart.dataset.armed ? 'Really restart?' : 'Restart');
});

// ---------- Start ----------

I18N.mountSwitch($('#lang-slot'));
I18N.apply();
renderDynamicTexts();
showTab(location.hash.slice(1));
loadSettings();
refreshInfo().then(() => { reportUpdate(); refreshLog(); });
setInterval(refreshInfo, 5000);
setInterval(refreshLog, 1000);
