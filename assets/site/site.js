(() => {
  'use strict';
  // Keep links to sections from the original one-page website useful.
  const legacyPages = {
    '主要功能': 'features.html',
    '音訊輸出': 'audio.html',
    '啟用-bitperfect': 'audio.html',
    '下載與安裝': 'downloads.html',
    'macos-install': 'downloads.html',
    'linux-wsl': 'downloads.html',
    'windows-11-透明背景': 'downloads.html',
    '從原始碼編譯': 'downloads.html',
    '自動測試': 'downloads.html',
    'windows-打包': 'downloads.html',
    'intel-onemkl-runtime': 'downloads.html',
    '授權': 'downloads.html',
    '免責聲明': 'downloads.html'
  };
  if (document.body.dataset.page === 'home') {
    let anchor;
    try { anchor = decodeURIComponent(location.hash.slice(1)); } catch { /* Leave invalid fragments alone. */ }
    if (Object.hasOwn(legacyPages, anchor)) {
      location.replace(legacyPages[anchor] + location.search + location.hash);
      return;
    }
  }

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
  if (controls && screenshot && screenshotLink && caption) {
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
  }

  if (document.querySelector('.hero-film')) {
    // Decorative playback is silent and pauses outside the viewport or behind the full film.
    const backgroundFilm = document.querySelector('.hero-film');
    const motionButton = document.querySelector('.motion-toggle');
    const fullFilm = document.querySelector('.product-film');
    const filmDialog = document.querySelector('.film-dialog');
    const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
    const phoneViewport = matchMedia('(max-width: 767px)');
    let manualMotion = null;
    let filmVisible = true;
    let playbackBlocked = false;
    const updateMotionButton = () => {
      const playing = !backgroundFilm.paused;
      motionButton.classList.toggle('is-playing', playing);
      motionButton.querySelector('span').textContent = playing ? '暫停背景' : '播放背景';
    };
    const syncBackgroundFilm = () => {
      const automaticMotion = !reducedMotion.matches && !phoneViewport.matches && !navigator.connection?.saveData && !playbackBlocked;
      const shouldPlay = (manualMotion ?? automaticMotion) && filmVisible && !document.hidden && !filmDialog.open;
      if (!shouldPlay) {
        backgroundFilm.pause();
        return;
      }
      const source = backgroundFilm.querySelector('source');
      if (!source.hasAttribute('src')) {
        source.src = source.dataset.src;
        backgroundFilm.muted = true;
        backgroundFilm.load();
      }
      backgroundFilm.play().catch(error => {
        // Intentional pauses may interrupt a pending play without blocking later playback.
        if (error.name !== 'AbortError') playbackBlocked = true;
        updateMotionButton();
      });
    };
    motionButton.hidden = false;
    motionButton.addEventListener('click', () => {
      manualMotion = backgroundFilm.paused;
      playbackBlocked = false;
      syncBackgroundFilm();
    });
    backgroundFilm.addEventListener('play', updateMotionButton);
    backgroundFilm.addEventListener('pause', updateMotionButton);
    backgroundFilm.addEventListener('error', updateMotionButton);
    document.addEventListener('visibilitychange', syncBackgroundFilm);
    reducedMotion.addEventListener('change', () => {
      manualMotion = null;
      syncBackgroundFilm();
    });
    phoneViewport.addEventListener('change', syncBackgroundFilm);
    if ('IntersectionObserver' in window) {
      const filmObserver = new IntersectionObserver(entries => {
        filmVisible = entries[0].isIntersecting;
        syncBackgroundFilm();
      }, { threshold: 0 });
      filmObserver.observe(document.querySelector('.cinema'));
    }
    syncBackgroundFilm();

    // Native dialog provides Escape-to-close, focus containment and focus restoration.
    document.querySelector('[data-watch-film]').addEventListener('click', event => {
      if (typeof filmDialog.showModal !== 'function') return;
      event.preventDefault();
      filmDialog.showModal();
      root.classList.add('film-open');
      syncBackgroundFilm();
      fullFilm.play().catch(() => { /* Native controls let the visitor retry. */ });
    });
    document.querySelector('.close-film').addEventListener('click', () => filmDialog.close());
    filmDialog.addEventListener('click', event => {
      if (event.target !== filmDialog) return;
      const bounds = filmDialog.getBoundingClientRect();
      if (event.clientX < bounds.left || event.clientX > bounds.right || event.clientY < bounds.top || event.clientY > bounds.bottom) filmDialog.close();
    });
    filmDialog.addEventListener('close', () => {
      fullFilm.pause();
      if (fullFilm.readyState > 0) fullFilm.currentTime = 0;
      root.classList.remove('film-open');
      syncBackgroundFilm();
    });

  }

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

  // Verified platform downloads remain usable when GitHub is unavailable.
  if (!document.querySelector('[data-download], [data-linux-download]')) return;
  fetch('https://api.github.com/repos/billlin0904/xamp2/releases/latest', { signal: AbortSignal.timeout(5000) })
    .then(response => {
      if (!response.ok) throw new Error('Release metadata unavailable');
      return response.json();
    })
    .then(release => {
      if (release.draft || release.prerelease || !/^v\d+\.\d+\.\d+$/.test(release.tag_name)) return;
      const platforms = [
        { name: 'xamp2-setup.exe', links: '[data-download]', versions: '[data-version]' },
        { name: `xamp2-${release.tag_name.slice(1)}-linux-x86_64.zip`, links: '[data-linux-download]', versions: '[data-linux-version]' }
      ];
      platforms.forEach(platform => {
        const asset = release.assets?.find(item => item.name === platform.name && item.state === 'uploaded');
        const expected = `https://github.com/billlin0904/xamp2/releases/download/${release.tag_name}/${platform.name}`;
        if (asset?.browser_download_url !== expected) return;
        document.querySelectorAll(platform.links).forEach(link => { link.href = expected; });
        document.querySelectorAll(platform.versions).forEach(label => { label.textContent = release.tag_name; });
      });
      document.querySelectorAll('[data-release]').forEach(link => { link.href = `https://github.com/billlin0904/xamp2/releases/tag/${release.tag_name}`; });
    })
    .catch(() => { /* Keep the verified release fallback; downloads do not depend on JavaScript. */ });
})();
