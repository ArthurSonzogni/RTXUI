# Promo video

A 30-second motion-design promo: kinetic titles, the C++ source in 3D, then
real footage of `example/showcase.cpp` cut to the beat.

`promo.html` poses every element for any instant `t` (`setTime(t)`), and
`render.js` captures it frame by frame in headless Chrome, so the result is
exact whatever the machine. Open `promo.html?play` in Chrome (started with
`--allow-file-access-from-files`) to preview it in real time.

## Render

```bash
# 1. Footage: the showcase, driven by a script, as PNG frames.
cmake --build build --target rtxui_example_showcase
python3 tools/video/record.py tools/video/promo_footage.py \
    /tmp/footage.mp4 tools/video/promo/footage

# 2. Fonts (both OFL): Inter, and JetBrains Mono (Nerd Font Mono).
mkdir -p tools/video/promo/fonts   # InterVariable.ttf,
                                   # JetBrainsMonoNerdFontMono-{Regular,Bold}.ttf

# 3. Render at 60 fps.
cd tools/video/promo && npm i puppeteer-core@23.11.1
node render.js promo.mp4 60
```

The video is silent: add music in an editor.
