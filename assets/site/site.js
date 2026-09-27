(() => {
  'use strict';
  const root = document.documentElement;
  const themeButton = document.querySelector('.theme-toggle');
  const systemTheme = matchMedia('(prefers-color-scheme: dark)');
  let savedTheme;
  try { savedTheme = localStorage.getItem('xamp-site-theme'); } catch { /* Storage is optional. */ }
  const setTheme = theme => {
    root.dataset.theme = theme;
    const label = theme === 'dark' ? '切換為淺色網頁' : '切換為深色網頁';
    themeButton.setAttribute('aria-label', label);
    themeButton.title = label;
  };
  setTheme(savedTheme === 'light' || savedTheme === 'dark' ? savedTheme : systemTheme.matches ? 'dark' : 'light');
  themeButton.hidden = false;
  themeButton.addEventListener('click', () => {
    savedTheme = root.dataset.theme === 'dark' ? 'light' : 'dark';
    setTheme(savedTheme);
    try { localStorage.setItem('xamp-site-theme', savedTheme); } catch { /* Keep the in-memory choice. */ }
  });
  systemTheme.addEventListener('change', event => {
    if (!savedTheme) setTheme(event.matches ? 'dark' : 'light');
  });

  const screenshots = {
    playlist: { src: 'assets/site/player-playlist.png', alt: 'XAMP 2 播放清單：左側導覽、專輯封面、歌曲列表與底部播放控制，搭配 Windows 透明背景。', caption: '播放清單實機畫面。介面與背景效果會依版本、系統及設定而異。' },
    library: { src: 'docs/images/player-library.png', alt: 'XAMP 音樂庫：本機資料夾瀏覽與歌曲列表，搭配 Windows 背景材質。', caption: '音樂庫開發版本實機畫面，歌曲列表為統一樣式前的版本；靜態圖片僅展示當時觀感。' }
  };
  const viewSwitch = document.querySelector('.view-switch');
  const screenshot = document.querySelector('#player-screenshot');
  const screenshotLink = document.querySelector('#screenshot-link');
  const caption = document.querySelector('#screenshot-caption');
  viewSwitch.hidden = false;
  viewSwitch.addEventListener('click', event => {
    const button = event.target.closest('button[data-view]');
    if (!button) return;
    const view = screenshots[button.dataset.view];
    screenshot.src = view.src;
    screenshot.alt = view.alt;
    screenshotLink.href = view.src;
    caption.textContent = view.caption;
    viewSwitch.querySelectorAll('button').forEach(item => item.setAttribute('aria-pressed', String(item === button)));
  });

  // Preserve links to headings that now live inside disclosure panels.
  const revealAnchor = () => {
    let id;
    try { id = decodeURIComponent(location.hash.slice(1)); } catch { return; }
    const target = document.getElementById(id);
    const disclosure = target?.closest('details');
    if (disclosure) disclosure.open = true;
  };
  revealAnchor();
  addEventListener('hashchange', revealAnchor);

  // The shipped 1.0.3 links remain usable offline or when GitHub rate-limits requests.
  fetch('https://api.github.com/repos/billlin0904/xamp2/releases/latest', { signal: AbortSignal.timeout(5000) })
    .then(response => {
      if (!response.ok) throw new Error('Release metadata unavailable');
      return response.json();
    })
    .then(release => {
      if (release.draft || release.prerelease || !/^v\d+\.\d+\.\d+$/.test(release.tag_name)) return;
      const asset = release.assets?.find(item => item.name === 'xamp2-setup.exe' && item.state === 'uploaded');
      const expected = `https://github.com/billlin0904/xamp2/releases/download/${release.tag_name}/xamp2-setup.exe`;
      if (asset?.browser_download_url !== expected) return;
      document.querySelectorAll('[data-download]').forEach(link => { link.href = expected; });
      document.querySelectorAll('[data-version]').forEach(label => { label.textContent = release.tag_name; });
      document.querySelectorAll('[data-release]').forEach(link => { link.href = `https://github.com/billlin0904/xamp2/releases/tag/${release.tag_name}`; });
    })
    .catch(() => { /* Keep the verified release fallback; downloads do not depend on JavaScript. */ });
})();
