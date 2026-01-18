#!/usr/bin/env bun

import sharp from "sharp";
import { readdir, stat } from "fs/promises";
import { join, extname, basename } from "path";

const HEIC_EXTENSIONS = [".heic", ".heif"];

interface ConvertOptions {
  quality: number;
  inputDir: string;
  outputDir: string;
}

async function convertHeicToWebp(
  inputPath: string,
  outputPath: string,
  quality: number
): Promise<void> {
  await sharp(inputPath).webp({ quality }).toFile(outputPath);
}

async function findHeicFiles(dir: string): Promise<string[]> {
  const files: string[] = [];

  const entries = await readdir(dir);

  for (const entry of entries) {
    const fullPath = join(dir, entry);
    const fileStat = await stat(fullPath);

    if (fileStat.isFile()) {
      const ext = extname(entry).toLowerCase();
      if (HEIC_EXTENSIONS.includes(ext)) {
        files.push(fullPath);
      }
    }
  }

  return files;
}

function getOutputPath(inputPath: string, outputDir: string): string {
  const name = basename(inputPath, extname(inputPath));
  return join(outputDir, `${name}.webp`);
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0 || args.includes("--help") || args.includes("-h")) {
    console.log(`
HEIC to WebP Converter

Usage:
  bun run convert.ts <input> [options]

Arguments:
  <input>          Path to a HEIC file or directory containing HEIC files

Options:
  -o, --output     Output directory (default: same as input)
  -q, --quality    WebP quality 1-100 (default: 80)
  -h, --help       Show this help message

Examples:
  bun run convert.ts photo.heic
  bun run convert.ts ./photos -o ./converted -q 90
  bun run convert.ts ./photos --output ./webp --quality 85
`);
    process.exit(0);
  }

  const input = args[0];
  let outputDir: string | null = null;
  let quality = 80;

  // Parse arguments
  for (let i = 1; i < args.length; i++) {
    const arg = args[i];
    if (arg === "-o" || arg === "--output") {
      outputDir = args[++i];
    } else if (arg === "-q" || arg === "--quality") {
      quality = parseInt(args[++i], 10);
      if (isNaN(quality) || quality < 1 || quality > 100) {
        console.error("Error: Quality must be between 1 and 100");
        process.exit(1);
      }
    }
  }

  // Check if input exists
  let inputStat;
  try {
    inputStat = await stat(input);
  } catch {
    console.error(`Error: Input path "${input}" does not exist`);
    process.exit(1);
  }

  let filesToConvert: string[] = [];
  let inputDir: string;

  if (inputStat.isDirectory()) {
    inputDir = input;
    filesToConvert = await findHeicFiles(input);
    if (filesToConvert.length === 0) {
      console.log("No HEIC files found in the directory");
      process.exit(0);
    }
  } else if (inputStat.isFile()) {
    const ext = extname(input).toLowerCase();
    if (!HEIC_EXTENSIONS.includes(ext)) {
      console.error("Error: Input file must be a HEIC/HEIF file");
      process.exit(1);
    }
    inputDir = join(input, "..");
    filesToConvert = [input];
  } else {
    console.error("Error: Input must be a file or directory");
    process.exit(1);
  }

  // Set output directory
  const finalOutputDir = outputDir || inputDir;

  // Ensure output directory exists
  await Bun.write(join(finalOutputDir, ".keep"), "");
  const { unlink } = await import("fs/promises");
  await unlink(join(finalOutputDir, ".keep")).catch(() => {});

  console.log(`Converting ${filesToConvert.length} file(s)...`);
  console.log(`Quality: ${quality}`);
  console.log(`Output: ${finalOutputDir}\n`);

  let successCount = 0;
  let errorCount = 0;

  for (const file of filesToConvert) {
    const outputPath = getOutputPath(file, finalOutputDir);
    const fileName = basename(file);

    try {
      process.stdout.write(`Converting ${fileName}... `);
      await convertHeicToWebp(file, outputPath, quality);
      console.log("✓");
      successCount++;
    } catch (error) {
      console.log("✗");
      console.error(`  Error: ${(error as Error).message}`);
      errorCount++;
    }
  }

  console.log(`\nDone! ${successCount} converted, ${errorCount} failed`);
}

main();
