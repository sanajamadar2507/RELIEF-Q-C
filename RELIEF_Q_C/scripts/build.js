const fs = require('fs');
const path = require('path');

const ROOT_DIR = path.resolve(__dirname, '..');
const DIST_DIR = path.join(ROOT_DIR, 'dist');

function copyDir(src, dest) {
  if (!fs.existsSync(src)) return;
  fs.mkdirSync(dest, { recursive: true });
  const entries = fs.readdirSync(src, { withFileTypes: true });
  for (const entry of entries) {
    const srcPath = path.join(src, entry.name);
    const destPath = path.join(dest, entry.name);
    if (entry.isDirectory()) {
      copyDir(srcPath, destPath);
    } else {
      fs.copyFileSync(srcPath, destPath);
    }
  }
}

console.log('=====================================================');
console.log('  RELIEF-Q — Production Build for Vercel / Web');
console.log('=====================================================');

// 1. Prepare dist directory
if (fs.existsSync(DIST_DIR)) {
  fs.rmSync(DIST_DIR, { recursive: true, force: true });
}
fs.mkdirSync(DIST_DIR, { recursive: true });

// 2. Copy root index.html
const indexSrc = path.join(ROOT_DIR, 'index.html');
if (fs.existsSync(indexSrc)) {
  fs.copyFileSync(indexSrc, path.join(DIST_DIR, 'index.html'));
  console.log(' [OK] Copied index.html');
} else {
  console.error(' [ERROR] index.html not found!');
  process.exit(1);
}

// 3. Copy css, js, wasm
const dirs = ['css', 'js', 'wasm'];
for (const dir of dirs) {
  const src = path.join(ROOT_DIR, dir);
  const dest = path.join(DIST_DIR, dir);
  if (fs.existsSync(src)) {
    copyDir(src, dest);
    console.log(` [OK] Copied /${dir} directory`);
  } else {
    console.warn(` [WARN] Directory /${dir} does not exist yet.`);
  }
}

// 4. Verify WebAssembly artifacts in dist
const wasmFile = path.join(DIST_DIR, 'wasm', 'relief_q.wasm');
const jsGlue = path.join(DIST_DIR, 'wasm', 'relief_q.js');

if (fs.existsSync(wasmFile) && fs.existsSync(jsGlue)) {
  const wasmStat = fs.statSync(wasmFile);
  const jsStat = fs.statSync(jsGlue);
  console.log(` [OK] WebAssembly binary verified: ${wasmFile} (${(wasmStat.size / 1024).toFixed(1)} KB)`);
  console.log(` [OK] Emscripten glue verified:   ${jsGlue} (${(jsStat.size / 1024).toFixed(1)} KB)`);
} else {
  console.warn(' [NOTICE] WebAssembly artifacts not found in dist/wasm yet. Ensure C build runs or committed artifacts exist.');
}

console.log('=====================================================');
console.log('  BUILD COMPLETE: Output written to /dist');
console.log('=====================================================');
