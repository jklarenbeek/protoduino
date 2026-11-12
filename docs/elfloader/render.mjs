// Renders the design document to PDF with headless Chromium.
//   node render.mjs            -> ../elf-loader-design.pdf
// Needs the "playwright" package and the Inter and DejaVu Sans Mono fonts.
import { readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { createRequire } from 'node:module';

const here = dirname(fileURLToPath(import.meta.url));
const { chromium } = createRequire(import.meta.url)('playwright');

const css = await readFile(join(here, 'doc.css'), 'utf8');
const parts = await Promise.all(['part1.html', 'part2.html', 'part3.html'].map((f) => readFile(join(here, f), 'utf8')));
const html = `<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>A streaming ELF loader for AVR - protoduino design document</title>
<style>${css}</style></head>
<body>${parts.join('\n')}</body></html>`;

const out = join(here, '..', 'elf-loader-design.pdf');
const footer = `<div style="width:100%; font-family:Inter,sans-serif; font-size:7.5px; color:#5b6672; padding:0 17mm; display:flex; justify-content:space-between;">
  <span>protoduino &middot; A streaming ELF loader for AVR &middot; draft 0.1</span>
  <span><span class="pageNumber"></span> / <span class="totalPages"></span></span></div>`;

const browser = await chromium.launch();
const page = await browser.newPage();
await page.setContent(html, { waitUntil: 'load' });
await page.evaluate(() => document.fonts.ready);
await page.pdf({
  path: out,
  format: 'A4',
  printBackground: true,
  displayHeaderFooter: true,
  headerTemplate: '<span></span>',
  footerTemplate: footer,
  margin: { top: '19mm', bottom: '21mm', left: '17mm', right: '17mm' },
});
await browser.close();
if (process.argv.includes('--html')) await writeFile(join(here, 'elf-loader-design.html'), html);
console.log('wrote', out);
