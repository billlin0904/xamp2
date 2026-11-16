# XAMP website design record

Mode: visual redesign of the GitHub Pages landing page, retaining the project identity and technical documentation.

## Audit and protected content

The previous homepage rendered README.md through Jekyll Slate: narrow reading column, duplicated project title, small product images, and an outdated 1.0.2 download link. Keep the XAMP logo, Traditional Chinese voice, MIT/third-party license text, documentation routes, releases, issue tracker, and existing heading deep links. README remains the developer reference. No analytics or forms existed.

## Design read

A Windows audio-player homepage for people with local music collections. Quiet, product-led visual language drawn from the real player, with a charcoal background and mint controls. Visual variance 6, motion 3, density 4, asset dependence 9, brand fidelity 8.

The first viewport introduces the player and offers the download; the full-width real screenshot provides evidence. Feature rows explain daily use, the audio section distinguishes PCM bit transparency from DSD conversion, and disclosures preserve setup/developer details without overwhelming the download path. Laptop and phone reading distances determine the heading/body scale.

## Design system and assets

- Brand logo: `assets/site/xamp-logo.png`, copied without modification from `src/xamp/xamp2.png`. Preserve its original pink/violet identity; mint is the interaction accent from the player.
- Screenshots: four user-approved replacement images supplied on 2026-09-27, copied without modification. Solid playlist: `assets/site/player-playlist.png` (also replaces `docs/images/player-playlist.png`); solid library: `docs/images/player-library.png`; transparent variants: `assets/site/player-playlist-transparent.png` and `assets/site/player-library-transparent.png`. Two independent controls select view and background. Previously published paths are overwritten too, so current site and README no longer serve the prior images. This does not rewrite historical Git commits or invalidate third-party caches.
- Charcoal #111715, surface #19221e, text #eef3ef, muted #a1b0a7, mint #a3e2c3. Light website tokens preserve the same hierarchy. Website theme is independent from desktop player settings.
- Typography: locally hosted Karla with platform Traditional Chinese body fallback; subset Source Han Sans TC Bold for display, retaining SIL OFL notices under `assets/site/fonts`. Subset named XampDisplay to avoid reserved font names. Regenerate after changing display text with `tools/Subset-WebsiteFont.py`.
- Spacing: 8px baseline, 40/56/96px responsive outer gutters, 64/80/112px section rhythm.
- Radii: 6px controls, 10px product/download surfaces. Shadow only on the product image. Header stays in normal document flow; only skip link uses an elevated layer.
- Motion: brief opacity/transform entry and 180ms button feedback, disabled for reduced motion. No scroll hijacking. Decorative hero playback follows the policy below.
- Static HTML/CSS with progressively enhanced JavaScript; existing GitHub Pages master/root deployment and Jekyll documentation remain intact. No new framework/build dependency.
- Download links have a verified v1.0.3 fallback; optional GitHub release metadata updates version and installer links together only when an uploaded installer exists.

## Verification

Check syntax, local asset and anchor references, keyboard-visible focus, desktop/mobile layout rules, reduced motion, screenshot switching, disclosure links and fallback download URLs before publication. No test installers or local configuration are part of the website changes.

## Linux download extension (2026-09-27)

Extension mode: retain the palette, typography, assets, anchors and Windows download behavior. Add a matching Linux download button and platform-specific detail in the existing mint panel, plus terminal instructions in the existing Linux disclosure. No new animation or screenshot is introduced. Dials: visual variance 6, motion 3, density 4, asset dependence 9, brand fidelity 10. Linux links stay pinned to the verified Linux asset independently of future Windows-only release metadata.

Follow-up: give Windows and Linux equal download buttons in the hero, with local platform icons from [Font Awesome Free 6.7.2](https://github.com/FortAwesome/Font-Awesome/tree/6.7.2/svgs/brands) (CC BY 4.0; original SVG attribution and `assets/site/icons/LICENSE.txt` retained). Replace the bright mint CTA fills and download panel with the existing muted surface/soft tokens in both themes. Keep text contrast and focus outlines clear. Stack the hero platform choices below 400px.

## macOS download (2026-10-12)

- Added v1.0.5 Apple Silicon PKG link and GitHub Assets SHA-256 verification guidance, native installer guidance, macOS 26 minimum and explicit pending notarization status.
- Windows/Linux retain their independently verified v1.0.3 assets. General release history links to all releases.
- Added an original monitor SVG for macOS; existing third-party icons are unchanged. macOS hero button spans the available row; installation notes use three columns on desktop and one on mobile.

## macOS refresh (2026-10-30)

- Pin macOS downloads to the dated `XAMP2-1.0.5-macOS-arm64-20261030-installer-signed.pkg` asset so existing downloads remain reproducible.
- Show the update date and describe native AppKit materials, larger secondary text, vinyl playback, and the global A-key shortcut fix in the macOS installation guide.
- Link the matching SHA-256 file. The PKG remains Developer ID signed and pending Apple notarization; Apple Silicon and macOS 26 minimum remain unchanged.

## Product film (2026-10-30)

- Publish the user-selected 15-second “一首歌的旅程” film using the complete approved MP4. The current homepage uses it as a decorative hero background and in an on-demand full player, as described below.
- Use native playback controls, inline playback, a local WebP poster, and `preload="none"`; playback starts only after visitor input. Preserve the existing palette, download links and screenshots.
- Reserve the 16:9 player area to prevent layout shifts. Stack the heading and descriptive copy on phones, retain keyboard focus styling, and provide a direct MP4 link.
- The film includes music and visible feature text; adjacent HTML identifies the content and its duration. Do not add narration or autoplay.


## Cinematic homepage (2026-10-30)

- Reuse the approved “一首歌的旅程” film as the hero background. Keep the real product visuals to the right, with a dark gradient behind the left-hand title and platform downloads. The hero stays charcoal in both website themes.
- Move the standalone film player into a native dialog reached by “觀看影片・含配樂”. The background is always muted; the full player retains the supplied soundtrack and native controls. Keep the direct MP4 fallback and all download/documentation anchors.
- Provide an explicit background play/pause control. Pause off-screen, in hidden tabs, and while the full film is open. Do not automatically start for reduced motion, data saving or phone viewports; explicit visitor playback remains available.
- On phones, show the heading, uncropped 16:9 product film, playback controls, then platform downloads. On desktop, lead with downloads on the left and the product film on the right. The real screenshot browser, feature rows, output details and installation guide follow.
- Retain static HTML/CSS and progressive JavaScript, the existing master/root GitHub Pages deployment and local video assets. Lazy-load the screenshot below the hero.

Validation: checked 1280px desktop and 390px/320px phone layouts, both themes, background mute/pause, off-screen pause, full-film audio state, Escape/close and focus restoration. JavaScript syntax, local asset references, Jekyll document routes and heading anchors pass.


## Separate website pages (2026-10-30)

- Replace the long single-page navigation with four static documents: `index.html` (film-led introduction and page links), `features.html` (real screenshot browser and daily features), `audio.html` (output support and setup), and `downloads.html` (platform downloads, installation, FAQ and licenses).
- Render the same four navigation links on every page. Use `aria-current="page"` plus a mint underline to identify the current page. Keep all four links visible in a second header row on phones, including 320px screens.
- Preserve the homepage film/background controls. Initialize screenshot and film behavior only when the corresponding component is present; other pages do not load the product film. Retain the independent page titles, descriptions and canonical URLs, shared theme preference, platform assets and documentation routes.
- Rewrite cross-page section links to their destination documents. Redirect old homepage fragments to the corresponding new page and expand the destination disclosure. Native page links remain usable without JavaScript.
- Subset the display font from all four HTML files through `tools/Subset-WebsiteFont.py`.
- Verified local assets, cross-page fragments, Jekyll routes, a single H1 and active navigation item per document, desktop/320px/390px layouts, browser back, theme persistence, screenshot switching, legacy installation links and homepage film playback.

## NTFS film update (2026-10-30)

- Add “NTFS 高速讀取” to the approved film's opening collection scene, with “Windows 專屬 · 音樂庫掃描”. Retain the original library/playlist/search copy, all other scenes, the 15-second duration and supplied soundtrack.
- Replace the local MP4 and derive the poster from the verified opening export frame. Version both media URLs with `v=ntfs-20261030` to refresh previously cached video and poster assets.
- NTFS copy describes the opt-in Windows metadata-reading backend; the film retains its original macOS UI capture and identifies that material separately. No measured speedup is claimed.

## Hallmark v2.0.0 visual restyle (2026-11-16)

The user's clarified scope is to retain the existing layout and change only its visual
system. The earlier full recomposition has been withdrawn. All four documents retain
their original section sequence, grids, media positions, navigation, download placements,
installation disclosures and footer. Home keeps the approved cinematic hero background
with left-hand platform buttons; downloads keeps its original two-column download panel.
The original JavaScript behaviour is restored, including eligible desktop background
playback and reduced-motion/mobile safeguards. Screenshot switching, legacy fragment
redirects and the native full-film dialog retain their existing logic.

Active CSS is `assets/site/hallmark.css`, derived from the original `site.css` geometry.
Shared `assets/site/hallmark/tokens.css` supplies the Ultraviolet night/lavender palette,
original local Karla and XampDisplay fonts, neutral cinematic scrim and paired inks.
The original CSS file is unchanged. Existing images/video keep their original colours.
Radii, control states, border treatment and shadows are restyled. Hallmark-required
changes remove decorative eyebrows, status markers, tracked labels and arrow suffixes;
headings use one ink colour; focus is instant; image tracks are shrink-safe; html/body
use overflow-x:clip. Font subsets are regenerated from the restored original copy.

Original optical spacing, multi-line platform-button heights and compact section rhythm
are documented exceptions (24b, 39b, 59). Structural findings are notes in restyle mode,
because changing them would contradict the user's instruction. No compatibility,
release, signing or notarization claim was added. All original URLs and documents remain.

Designeer inspiration/resources and its Utopia/Minimal Gallery entries were consulted.
No new framework, remote font, imagery, analytics or runtime dependency is introduced.
QA dependencies are isolated under /tmp. Before/after images: `.hallmark/compare.html`.

Validation: all four documents' original section/header/footer/main/nav/details/dialog
sequence and every href compare exactly with Git HEAD. Twenty route/viewport checks
from 320 to 1920px, image loading, active navigation, theme persistence, all four
screenshot combinations, legacy-hash disclosure, reduced-motion idle state, film
play/pause/mute, dialog close/Escape/focus restoration and no-script visibility pass.
JavaScript syntax and whitespace checks pass. Static 0 FAIL / 0 open REFLEX;
HTTP render 0 FAIL / 0 open REFLEX / 0 WARN. Structural notes are retained by design.

Independent restyle finish review returned ship with no required fixes. No imagery slots or stand-ins need replacement.

## Remove hero background image (2026-11-16)

At the user's request, remove the homepage's right-side film/poster background and its
background play/pause control. The hero now uses a solid ground. Keep the existing
left-hand copy/downloads and on-demand full-film dialog. Dialog initialization is
independent of the removed background video; no background video is loaded or played.
Other pages and their screenshot browser remain unchanged.

## Right-side homepage downloads (2026-11-16)

Move the existing three platform downloads and installation links to the right column.
Keep headline, introduction and on-demand video link on the left. Mobile stacks the
introduction, video action and download choices in reading order. No background image.

## iPhone / iPad distribution (2026-11-16)

Verified the public Taiwan App Store listing, app ID 6829106965, developer ZHENG-FENG
LIN, requiring iOS/iPadOS 26. Added its exact store link to home and downloads, plus
iOS setup/privacy, mobile features and experimental DSD DoP caveats. Public listing
takes precedence over stale local README review status. No unpublished development
features/version numbers are advertised. Desktop binaries and right-side downloads stay.
