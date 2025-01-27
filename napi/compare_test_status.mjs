import {describe, it} from 'node:test';
import {open, opendir, mkdir} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const [,, shouldOverwrite] = process.argv;

const actualDir = fileURLToPath(new URL('./tap_output/build/test/', import.meta.url));
const expectedDir = fileURLToPath(new URL('./tap_output/test/', import.meta.url));
const timeRegex = /# time=.+s$/gm;

describe('compare test results', {concurrency: true}, async () => {
for await (const dirent of await opendir(actualDir, {recursive: true})) {
  if (dirent.isDirectory() || !dirent.name.endsWith('.tap')) continue;
  const actualResultPath = path.join(dirent.parentPath, dirent.name);
  const expectedResultDir = path.join(expectedDir, dirent.parentPath.slice(actualDir.length));
  const expectedResultPath = path.join(expectedResultDir, dirent.name);
  it(actualResultPath, async (t) => {
    await mkdir(expectedResultDir, {recursive: true});
    const files = await Promise.all([open(actualResultPath, 'r'), open(expectedResultPath, shouldOverwrite ? 'w+' : 'r')]);
    try {
        const [actualRawResult, expectedResult] = shouldOverwrite ? [await files[0].readFile('utf-8')] : await Promise.all(files.map(async f => f.readFile('utf-8')));
        const actualResult = actualRawResult.replace(timeRegex, '');
        if (shouldOverwrite) {
            await files[1].writeFile(actualResult);
        } else {
          t.assert.strictEqual(actualResult, expectedResult);
        }
    } finally {
        await Promise.all(files.map(f => f.close()));
    }
  })
}
});
