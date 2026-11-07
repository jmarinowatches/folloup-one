# Japanese starter font

The firmware already includes its generated glyph atlas. No font download is
required to compile it or use the starter deck.

For regeneration, download the Noto Sans JP variable font from:

https://raw.githubusercontent.com/google/fonts/main/ofl/notosansjp/NotoSansJP%5Bwght%5D.ttf

Save it here as `NotoSansJP.ttf`, then run `python scripts/build_japanese.py`.
The exact font used for this image has SHA-256:

`c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f`

License: SIL Open Font License 1.1, in OFL.txt. The downloadable source ZIP also
contains the original font bytes. The font is not required at runtime.
