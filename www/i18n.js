'use strict';

// Interface languages: English (the source text everywhere, also in the firmware's messages) and
// German. The language follows the browser until the user picks one with the switch; that choice
// is remembered in localStorage.
//
//   t('Saved "{name}"', { name })      text in the current language, {placeholders} filled in
//   I18N.add({ 'English': 'Deutsch' }) a page adds its own translations
//   I18N.server(message)               a message from the firmware, translated where known
//   <span data-i18n>Save</span>        static text, translated by I18N.apply()
//   <input data-i18n-attr="placeholder title" placeholder="Search" title="...">
//   I18N.onChange(fn)                  re-render dynamic parts after a switch

const I18N = (() => {
  const STORAGE_KEY = 'labelbee.lang';
  const LANGS = ['en', 'de'];
  const de = Object.create(null);   // no prototype: t('constructor') stays text
  const patterns = [];   // [RegExp, German template with $1, $2, ...] for firmware messages with values
  const listeners = [];

  function detect() {
    try {
      const saved = localStorage.getItem(STORAGE_KEY);
      if (LANGS.includes(saved)) return saved;
    } catch { /* storage off */ }
    for (const l of navigator.languages || [navigator.language || '']) {
      const code = String(l).slice(0, 2).toLowerCase();
      if (LANGS.includes(code)) return code;
    }
    return 'en';
  }

  let lang = detect();

  function fill(text, params) {
    return params ? text.replace(/\{(\w+)\}/g, (m, k) => (params[k] !== undefined ? params[k] : m)) : text;
  }

  function t(text, params) {
    return fill(lang === 'de' && de[text] !== undefined ? de[text] : text, params);
  }

  /** A message from the firmware (always English): exact matches first, then patterns. */
  function server(message) {
    if (lang !== 'de' || !message) return message;
    if (de[message] !== undefined) return de[message];
    for (const [re, template] of patterns) {
      const m = re.exec(message);
      if (m) return template.replace(/\$(\d)/g, (_, i) => m[Number(i)] ?? '');
    }
    return message;
  }

  /** Translates the static parts of the page: [data-i18n] text and [data-i18n-attr] attributes. */
  function apply(root = document) {
    document.documentElement.lang = lang;
    for (const el of root.querySelectorAll('[data-i18n]')) {
      if (el.dataset.i18nKey === undefined) el.dataset.i18nKey = el.textContent.trim();
      el.textContent = t(el.dataset.i18nKey);
    }
    for (const el of root.querySelectorAll('[data-i18n-attr]')) {
      for (const attr of el.dataset.i18nAttr.split(/\s+/).filter(Boolean)) {
        const store = `i18n${attr.replace(/(^|-)(\w)/g, (_, __, c) => c.toUpperCase())}`;
        if (el.dataset[store] === undefined) el.dataset[store] = el.getAttribute(attr) || '';
        el.setAttribute(attr, t(el.dataset[store]));
      }
    }
    const title = document.querySelector('title');
    if (title) {
      if (title.dataset.i18nKey === undefined) title.dataset.i18nKey = title.textContent;
      title.textContent = t(title.dataset.i18nKey);
    }
    for (const b of document.querySelectorAll('.lang-switch button')) {
      b.setAttribute('aria-pressed', String(b.dataset.lang === lang));
    }
  }

  function set(next) {
    if (!LANGS.includes(next) || next === lang) return;
    lang = next;
    try { localStorage.setItem(STORAGE_KEY, next); } catch { /* storage off */ }
    apply();
    for (const fn of listeners) fn(lang);
  }

  /** The DE | EN switch in a container (the page header). */
  function mountSwitch(container) {
    const group = document.createElement('div');
    group.className = 'lang-switch';
    group.setAttribute('role', 'group');
    group.setAttribute('aria-label', 'Language / Sprache');
    for (const [code, label, name] of [['de', 'DE', 'Deutsch'], ['en', 'EN', 'English']]) {
      const b = document.createElement('button');
      b.type = 'button';
      b.dataset.lang = code;
      b.textContent = label;
      b.title = name;
      b.setAttribute('aria-pressed', String(code === lang));
      b.addEventListener('click', () => set(code));
      group.append(b);
    }
    container.append(group);
  }

  return {
    t,
    server,
    apply,
    set,
    mountSwitch,
    add: (dict) => Object.assign(de, dict),
    addPatterns: (list) => patterns.push(...list),
    onChange: (fn) => listeners.push(fn),
    get lang() { return lang; },
    /** For toLocaleString and friends. */
    get locale() { return lang === 'de' ? 'de-DE' : 'en-GB'; },
  };
})();

const t = I18N.t;

// Messages from the firmware (src/*.cpp), shared by both pages.
I18N.add({
  'Not found.': 'Nicht gefunden.',
  'The label is empty or too long.': 'Das Etikett ist leer oder zu lang.',
  'The print data does not match the width and tape.': 'Die Druckdaten passen nicht zu Breite und Band.',
  'The print data is incomplete.': 'Die Druckdaten sind unvollständig.',
  'The label is empty or too large.': 'Das Etikett ist leer oder zu groß.',
  'The label could not be saved.': 'Das Etikett konnte nicht gespeichert werden.',
  'The label data is not valid.': 'Die Etikettdaten sind ungültig.',
  'The settings could not be saved.': 'Die Einstellungen konnten nicht gespeichert werden.',
  'There is no such file.': 'Diese Datei gibt es nicht.',
  'Printed.': 'Gedruckt.',
  'Invalid request.': 'Ungültige Anfrage.',
  'A scan is already running. Please try again in a moment.': 'Es läuft schon eine Suche. Bitte gleich noch einmal.',
  'ms is missing.': 'ms fehlt.',
  'Reset. labelbee restarts and then only opens its own WiFi "labelbee" (password "labelbee").':
    'Zurückgesetzt. labelbee startet neu und öffnet dann nur sein eigenes WLAN „labelbee“ (Passwort „labelbee“).',
  'labelbee is restarting.': 'labelbee startet neu.',
  'No update partition found.': 'Kein Update-Bereich gefunden.',
  'The file is empty or larger than the update partition.': 'Die Datei ist leer oder größer als der Update-Bereich.',
  'Transfer aborted.': 'Übertragung abgebrochen.',
  'labelbee is busy, please try again in a moment.': 'labelbee ist beschäftigt, bitte gleich noch einmal.',
  'Device name: 1 to 32 characters, only lower-case letters, digits and hyphens.':
    'Gerätename: 1 bis 32 Zeichen, nur Kleinbuchstaben, Ziffern und Bindestriche.',
  'WiFi name (up to 32) or password (up to 64 characters) too long.': 'WLAN-Name (bis 32) oder Passwort (bis 64 Zeichen) zu lang.',
  'The WiFi password has at least 8 characters.': 'Das WLAN-Passwort hat mindestens 8 Zeichen.',
  'Fixed address: give the IP address and a prefix of 1 to 30.': 'Feste Adresse: IP-Adresse und Präfix von 1 bis 30 angeben.',
  'The gateway is not an IPv4 address.': 'Das Gateway ist keine IPv4-Adresse.',
  'A DNS server is not an IPv4 address.': 'Ein DNS-Server ist keine IPv4-Adresse.',
  'The access point name has 1 to 32 characters.': 'Der Name des Access Points hat 1 bis 32 Zeichen.',
  'The access point password is empty (open) or has 8 to 63 characters.':
    'Das Passwort des Access Points ist leer (offen) oder hat 8 bis 63 Zeichen.',
  'NTP server or time zone too long.': 'NTP-Server oder Zeitzone zu lang.',
  'The printer is switched off. Switch it on in the settings.': 'Der Drucker ist ausgeschaltet. In den Einstellungen einschalten.',
  'The printer does not respond. Is it connected?': 'Der Drucker meldet sich nicht. Ist er angeschlossen?',
  'The printer does not answer.': 'Der Drucker antwortet nicht.',
  'No tape inserted.': 'Kein Band eingelegt.',
  'The label is empty.': 'Das Etikett ist leer.',
  'The printer was disconnected while printing.': 'Der Drucker wurde während des Drucks getrennt.',
  'The printer stopped answering.': 'Der Drucker antwortet nicht mehr.',
  'Sending to the printer failed.': 'Übertragung zum Drucker fehlgeschlagen.',
});
I18N.addPatterns([
  [/^Unknown tape width "(.*)"\.$/, 'Unbekannte Bandbreite „$1“.'],
  [/^Copies: 1 to (\d+)\.$/, 'Kopien: 1 bis $1.'],
  [/^The name must have 1 to (\d+) characters\.$/, 'Der Name muss 1 bis $1 Zeichen haben.'],
  [/^Saved\. labelbee restarts and is then at (\S+) \(or via the WiFi "(.*)" at (\S+)\)\.$/,
    'Gespeichert. labelbee startet neu und ist danach unter $1 erreichbar (oder über das WLAN „$2“ unter $3).'],
  [/^Update not started: (.*)$/, 'Update nicht gestartet: $1'],
  [/^The scan failed: (.*)$/, 'Die Suche ist fehlgeschlagen: $1'],
  [/^Writing failed: (.*)$/, 'Schreiben fehlgeschlagen: $1'],
  [/^Invalid image: (.*)$/, 'Image ungültig: $1'],
  [/^Update written \((.*)\), restarting\.$/, 'Update geschrieben ($1), starte neu.'],
]);
