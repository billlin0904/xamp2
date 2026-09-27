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
- Motion: brief opacity/transform entry and 180ms button feedback, disabled for reduced motion. No scroll hijacking or autoplay.
- Static HTML/CSS with progressively enhanced JavaScript; existing GitHub Pages master/root deployment and Jekyll documentation remain intact. No new framework/build dependency.
- Download links have a verified v1.0.3 fallback; optional GitHub release metadata updates version and installer links together only when an uploaded installer exists.

## Verification

Check syntax, local asset and anchor references, keyboard-visible focus, desktop/mobile layout rules, reduced motion, screenshot switching, disclosure links and fallback download URLs before publication. No test installers or local configuration are part of the website changes.

## Linux download extension (2026-09-27)

Extension mode: retain the palette, typography, assets, anchors and Windows download behavior. Add a matching Linux download button and platform-specific detail in the existing mint panel, plus terminal instructions in the existing Linux disclosure. No new animation or screenshot is introduced. Dials: visual variance 6, motion 3, density 4, asset dependence 9, brand fidelity 10. Linux links stay pinned to the verified Linux asset independently of future Windows-only release metadata.
