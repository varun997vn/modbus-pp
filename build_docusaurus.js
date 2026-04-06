#!/usr/bin/env node

const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const docDir = path.join(__dirname, "docusaurus");

try {
  console.log("Starting Docusaurus build...");
  process.chdir(docDir);

  const result = execSync("npm run build", {
    stdio: "pipe",
    encoding: "utf-8",
    maxBuffer: 10 * 1024 * 1024,
  });

  console.log("BUILD OUTPUT:");
  console.log(result);

  // Check if build directory exists
  const buildDir = path.join(docDir, "build");
  if (fs.existsSync(buildDir)) {
    console.log("\n✓ Build successful! Output in:", buildDir);
    const buildFiles = fs.readdirSync(buildDir);
    console.log("Contents:", buildFiles);
  } else {
    console.log("\n✗ Build output directory not found");
  }
} catch (error) {
  console.log("BUILD ERROR:");
  console.log(
    "stdout:",
    error.stdout ? error.stdout.toString().slice(-2000) : "empty",
  );
  console.log(
    "stderr:",
    error.stderr ? error.stderr.toString().slice(-2000) : "empty",
  );
  console.log("Code:", error.status);
  process.exit(1);
}
