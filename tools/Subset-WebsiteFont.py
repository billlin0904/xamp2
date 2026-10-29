"""Regenerate the website's small, self-hosted Traditional Chinese display font.

Requires fonttools[woff]. The source font stays in the application's font directory.
"""
from pathlib import Path
from fontTools import subset
from fontTools.ttLib import TTCollection

root = Path(__file__).resolve().parents[1]
font = TTCollection(root / "src/xamp/fonts/SourceHanSans-Bold.ttc").fonts[3]
options = subset.Options()
options.flavor = "woff2"
options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]
subsetter = subset.Subsetter(options=options)
pages = ("index.html", "features.html", "audio.html", "downloads.html")
subsetter.populate(text="\n".join((root / page).read_text(encoding="utf-8") for page in pages))
subsetter.subset(font)
for record in font["name"].names:
    if record.nameID in (1, 3, 4, 6):
        record.string = "XampDisplay-TC".encode(record.getEncoding())
font.flavor = "woff2"
font.save(root / "assets/site/fonts/XampDisplay-TC.woff2")
