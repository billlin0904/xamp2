# Design: XAMP 2

Shared Hallmark v2.0.0 system for the four GitHub Pages documents.

## System
- Genre: modern-minimal
- Layout: preserved from the original site. Home keeps its cinematic background and left-side platform downloads; features keeps its screenshot browser; audio keeps its mode split; downloads keeps its download panel and FAQ.
- Theme: Ultraviolet, night and lavender colourways, adapted to XAMP's existing local fonts and original logo.
- Axes: dark / geometric sans with Traditional Chinese display / violet. Restrained accent.
- Preserve real product colours in screenshots and video; never tint or redraw the interface.
- Home, features, audio and downloads share chrome, tokens and control states. No build dependency.

## Tokens
`assets/site/hallmark/tokens.css` is the source of truth.
- Paper: tinted violet at 15% lightness in dark mode, 95% in light mode.
- Ink: 94% in dark mode, 18% in light mode; muted text must meet 4.5:1.
- Accent: violet at 76% in dark mode and 48% in light mode with its own paired ink.
- Display: local XampDisplay, 700, with Traditional Chinese fallback.
- Body: local Karla, 400, with Traditional Chinese fallback.
- Code uses the outlier monospace token. Headings never have coloured words.
- Container: 1440px including 64px gutters, reducing to 24px then 16px.
- 4px spacing scale. 16px surfaces, pill controls. No gradient, glow or decorative drawing.

## Controls
- One primary violet pill style, 48px minimum height; other actions outline or text.
- Focus-visible outline appears instantly; disabled and pressed styles explicit.
- Navigation marks the active page using aria-current and an underline.
- Retain the original theme button, video dialog, screenshot controls and legacy fragments.

## Motion
Original motion behaviour retained: muted cinematic background on eligible desktop views, explicit play/pause, reduced-motion and mobile idle states, pause offscreen and during full-film dialog.
All transitions respect prefers-reduced-motion. Content works without scripts.

## Content
Traditional Chinese, concise, grounded in the existing project. Retain technical limitations,
platform-specific download versions, license notices, documentation and FAQ anchors.
No invented metrics, testimonials, screenshots, compatibility claims or release updates.
No eyebrows, tracked labels, middle-dot metadata or arrows suffixed to link labels.

## Exports
`assets/site/hallmark/tokens.css`, `assets/site/hallmark/base.css`, `assets/site/hallmark.css`.
The previous `site.css` stays archived in place; active pages load the new stylesheet.

## Restyle scope (user correction)
Preserve original section sequence, grids, widths, image positions, breakpoints and download arrangement. Only colour, typography, radii, borders, shadows and control states change. Decorative eyebrows and arrow suffixes are removed for Hallmark compliance. Gates 24b, 39b and 59 preserve original optical spacing, multi-line platform-button height and section rhythm.

## Home media update
The homepage has no background screenshot/video. Preserve its solid hero ground,
left-side introduction/platform downloads, and on-demand film link/dialog.

Homepage desktop: introduction and video action left, platform downloads right; mobile stacks these in reading order.
