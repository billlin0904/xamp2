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
    playlist: {
      solid: { src: 'assets/site/player-playlist.png?v=2', alt: 'XAMP 2 實色背景播放清單：專輯封面、歌曲資訊與底部播放控制。', caption: '播放清單 · 實色背景。專輯封面、曲目資訊與播放控制集中在同一個畫面。' },
      transparent: { src: 'assets/site/player-playlist-transparent.png', alt: 'XAMP 2 透明背景播放清單，展示 Windows 背景材質與歌曲列表。', caption: '播放清單 · 透明背景。背景材質融入桌面色彩，保留清楚的文字與控制項；實際效果依系統與設定而異。' }
    },
    library: {
      solid: { src: 'docs/images/player-library.png?v=2', alt: 'XAMP 2 實色背景音樂庫：資料夾樹、專輯歌曲列表與頻譜顯示區。', caption: '音樂庫 · 實色背景。以資料夾瀏覽收藏，查看專輯曲目，並保留頻譜顯示區。' },
      transparent: { src: 'assets/site/player-library-transparent.png', alt: 'XAMP 2 透明背景音樂庫：資料夾瀏覽、專輯曲目與頻譜顯示區。', caption: '音樂庫 · 透明背景。資料夾導覽搭配桌面背景材質，曲目與頻譜區維持清楚的層次。' }
    }
  };
  const controls = document.querySelector('.screenshot-controls');
  const screenshot = document.querySelector('#player-screenshot');
  const screenshotLink = document.querySelector('#screenshot-link');
  const caption = document.querySelector('#screenshot-caption');
  let selectedView = 'playlist';
  let selectedBackground = 'solid';
  controls.hidden = false;
  controls.addEventListener('click', event => {
    const button = event.target.closest('button');
    if (!button) return;
    if (button.dataset.view) selectedView = button.dataset.view;
    if (button.dataset.background) selectedBackground = button.dataset.background;
    const view = screenshots[selectedView][selectedBackground];
    screenshot.src = view.src;
    screenshot.alt = view.alt;
    screenshotLink.href = view.src;
    caption.textContent = view.caption;
    controls.querySelectorAll('[data-view]').forEach(item => item.setAttribute('aria-pressed', String(item.dataset.view === selectedView)));
    controls.querySelectorAll('[data-background]').forEach(item => item.setAttribute('aria-pressed', String(item.dataset.background === selectedBackground)));
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
