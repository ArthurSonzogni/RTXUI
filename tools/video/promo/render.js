// Renders promo.html to MP4, one frame at a time, so every frame is exact
// whatever the machine's speed.
//
//   node render.js <out.mp4> [fps] [from-second] [to-second]
//
// Needs puppeteer-core (npm i puppeteer-core), Google Chrome, ffmpeg, the
// fonts in fonts/ and the terminal footage in footage/ (see README.md).
const { spawn } = require('child_process');
const path = require('path');
const puppeteer = require('puppeteer-core');

(async () => {
  const out = process.argv[2] || 'promo.mp4';
  const fps = Number(process.argv[3] || 60);
  const browser = await puppeteer.launch({
    executablePath: process.env.CHROME || '/usr/bin/google-chrome',
    args: ['--allow-file-access-from-files', '--force-color-profile=srgb',
           '--hide-scrollbars'],
  });
  const page = await browser.newPage();
  await page.setViewport({ width: 1920, height: 1080 });
  await page.goto('file://' + path.join(__dirname, 'promo.html'));
  await page.evaluate(() => window.ready);
  const duration = await page.evaluate(() => window.DURATION);
  const from = Number(process.argv[4] || 0);
  const to = Number(process.argv[5] || duration);

  const ffmpeg = spawn('ffmpeg', [
    '-loglevel', 'error', '-y', '-f', 'image2pipe', '-framerate', String(fps),
    '-i', '-', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '16',
    '-preset', 'slow', '-movflags', '+faststart', out,
  ], { stdio: ['pipe', 'inherit', 'inherit'] });

  const frames = Math.round((to - from) * fps);
  for (let f = 0; f < frames; f++) {
    await page.evaluate((t) => window.setTime(t), from + f / fps);
    const shot = await page.screenshot({ type: 'jpeg', quality: 95 });
    if (!ffmpeg.stdin.write(shot)) {
      await new Promise((r) => ffmpeg.stdin.once('drain', r));
    }
    if (f % 60 === 0) process.stdout.write(`\r${f}/${frames}`);
  }
  ffmpeg.stdin.end();
  await new Promise((r) => ffmpeg.on('close', r));
  await browser.close();
  console.log(`\nwrote ${out}`);
})();
