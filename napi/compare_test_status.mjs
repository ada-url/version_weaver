#!/usr/bin/env node
import { describe, it } from "node:test";
import { open, opendir, mkdir } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const shouldOverwrite = process.argv[2] === '--overwrite';

const actualDir = fileURLToPath(
  new URL("./tap_output/build/test/", import.meta.url),
);
const expectedDir = fileURLToPath(
  new URL("./tap_output/test/", import.meta.url),
);
// Remove test duration and line numbers that might not be stable across versions of Node.js.
const timeRegex = /(# time=.+s|(?<=node:\S+:)\d+)$/gm;
// Node.js internals might change stack trace accross version, but that is not a relevant change for us.
const nodeJSInternalRegex = /\n\s+\n\s+\\e\[90m\s+at\n? .+?\(\n?node:.+?:\d+:\d+\)\\e\[39m/gs;

describe("compare test results", { concurrency: true }, async () => {
  for await (const dirent of await opendir(actualDir, { recursive: true })) {
    if (dirent.isDirectory() || !dirent.name.endsWith(".tap")) continue;
    const actualResultPath = path.join(dirent.parentPath, dirent.name);
    const testDirSubpath = dirent.parentPath.slice(actualDir.length);
    const expectedResultDir = path.join(expectedDir, testDirSubpath);
    const expectedResultPath = path.join(expectedResultDir, dirent.name);
    it(testDirSubpath + path.sep + dirent.name.slice(0, -4), async (t) => {
      await mkdir(expectedResultDir, { recursive: true });
      const files = await Promise.all([
        open(actualResultPath, "r"),
        open(expectedResultPath, shouldOverwrite ? "w" : "r"),
      ]);
      try {
        const [actualRawResult, expectedResult] =
          shouldOverwrite ?
            [await files[0].readFile("utf-8")]
          : await Promise.all(files.map(async (f) => f.readFile("utf-8")));
        const actualResult = actualRawResult
          .replace(timeRegex, "")
          .replace(nodeJSInternalRegex, "")
          .replaceAll(`Node.js ${process.version}\n`, 'Node.js vXX.XX.XX\n');
        if (shouldOverwrite) {
          await files[1].writeFile(actualResult);
        } else {
          t.assert.strictEqual(actualResult, expectedResult, 'use --overwrite to update the snapshot');
        }
      } finally {
        await Promise.all(files.map((f) => f.close()));
      }
    });
  }
});
