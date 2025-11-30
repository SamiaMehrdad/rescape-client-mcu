#!/bin/bash

# Script to rename project and initialize Git repository
# Usage: ./rename-and-init-git.sh

echo "=== reEscape Client Device - Project Rename and Git Initialization ==="
echo ""

# Current directory name
CURRENT_DIR=$(basename "$PWD")
NEW_DIR="rescape-client-device"

echo "Current directory: $CURRENT_DIR"
echo "Target name: $NEW_DIR"
echo ""

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo "Error: platformio.ini not found. Are you in the project root?"
    exit 1
fi

# Step 1: Initialize Git (if not already done)
if [ ! -d ".git" ]; then
    echo "Step 1: Initializing Git repository..."
    git init
    echo "✓ Git initialized"
else
    echo "Step 1: Git repository already exists"
fi

# Step 2: Add all files
echo ""
echo "Step 2: Adding files to Git..."
git add .
echo "✓ Files staged"

# Step 3: Create initial commit
echo ""
echo "Step 3: Creating initial commit..."
git commit -m "Initial commit: reEscape Client Device v1.0

- ESP32-C3 escape room controller firmware
- PCF8575 I/O expander integration
- Debounced keypad scanning (4x4 matrix)
- WS2812B LED animation system
- PWM audio synthesizer with ADSR
- RS-485 Room Bus communication
- ESPTimer hardware timer library
- Professional debouncing and timing
"
echo "✓ Initial commit created"

# Step 4: Instructions for renaming directory
echo ""
echo "Step 4: Directory rename instructions"
echo "======================================"
echo ""
echo "To complete the rename, run these commands from the PARENT directory:"
echo ""
echo "  cd .."
echo "  mv $CURRENT_DIR $NEW_DIR"
echo "  cd $NEW_DIR"
echo ""

# Step 5: GitHub repository setup
echo "Step 5: GitHub repository setup"
echo "================================"
echo ""
echo "To push to GitHub:"
echo ""
echo "1. Create a new repository on GitHub named: rescape-client-device"
echo "   URL: https://github.com/new"
echo ""
echo "2. Add the remote and push:"
echo "   git remote add origin https://github.com/YOUR_USERNAME/rescape-client-device.git"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""
echo "Or if using SSH:"
echo "   git remote add origin git@github.com:YOUR_USERNAME/rescape-client-device.git"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""

echo "=== Setup Complete! ==="
echo ""
echo "Next steps:"
echo "1. Follow the directory rename instructions above"
echo "2. Create GitHub repository"
echo "3. Push to GitHub using the commands above"
