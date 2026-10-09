# Subdue icon artwork

This directory contains the BeOS/Haiku-style Subdue application icon.

The design adapts the existing Subdue moon, application-window, and countdown
clock motif to the compact isometric hardware style associated with classic
BeOS R5 icons. The image has a transparent background and remains recognizable
at Haiku's 16 px and 32 px icon sizes.

## Files

- `icons/Subdue-Haiku-master.png` — selected full-resolution transparent
  master, with a smaller complete crescent resting on the window.
- `icons/Subdue-Haiku-{128,64,32,16}.png` — selected size variants.

Earlier design iterations are intentionally excluded from the public source
repository; the unversioned files above are the approved artwork.

The PNG files are artwork sources. Before a packaged release, recreate or
trace the master in Haiku's Icon-O-Matic and export an HVIF resource for
`Subdue.rdef`.

Style research used the public BeOS R5 icon collection at:
https://github.com/jcherven/BeOS-r5-Icons
