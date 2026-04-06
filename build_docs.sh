#!/bin/bash
set -e

echo "Building modbus_pp Documentation..."
echo "===================================="
echo ""

cd docusaurus

echo "1. Installing dependencies..."
npm ci 2>&1 | grep -v "npm notice" || true

echo ""
echo "2. Building documentation..."
npm run build

echo ""
echo "✓ Build complete! Output in docusaurus/build/"
echo ""
echo "To serve locally:"
echo "  cd docusaurus && npm run serve"
