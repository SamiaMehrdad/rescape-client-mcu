# Project Rename and GitHub Setup Instructions

## Quick Start (Automated)

Run the provided script:

```bash
./rename-and-init-git.sh
```

Then follow the on-screen instructions.

---

## Manual Instructions

### Step 1: Initialize Git Repository

```bash
cd /Users/mskmac/archive/MSK_local_src/code/escape/esp32/esp32-hello
git init
git add .
git commit -m "Initial commit: reEscape Client Device v1.0"
```

### Step 2: Rename Project Directory

```bash
cd /Users/mskmac/archive/MSK_local_src/code/escape/esp32
mv esp32-hello rescape-client-device
cd rescape-client-device
```

### Step 3: Create GitHub Repository

1. Go to https://github.com/new
2. Repository name: `rescape-client-device`
3. Description: "ESP32-based client controller for reEscape room automation system"
4. Public or Private (your choice)
5. Do NOT initialize with README (we already have one)
6. Click "Create repository"

### Step 4: Push to GitHub

```bash
# Using HTTPS
git remote add origin https://github.com/YOUR_USERNAME/rescape-client-device.git
git branch -M main
git push -u origin main

# OR using SSH (recommended)
git remote add origin git@github.com:YOUR_USERNAME/rescape-client-device.git
git branch -M main
git push -u origin main
```

### Step 5: Verify

Visit your GitHub repository to confirm all files are uploaded:

-   README.md (project overview)
-   CHANGELOG.md (development history)
-   src/ (source code)
-   include/ (header files)
-   platformio.ini (build config)

---

## Repository Details

**Name:** rescape-client-device  
**Description:** ESP32-based client controller for reEscape room automation system  
**Topics:** esp32, escape-room, iot, automation, rs485, keypad, led-animation

**README Sections:**

-   ✅ Overview and features
-   ✅ Hardware requirements
-   ✅ Quick start guide
-   ✅ API documentation
-   ✅ Project structure
-   ✅ Performance specs

---

## Future Updates

To commit new changes:

```bash
git add .
git commit -m "Description of changes"
git push
```

To create a release/tag:

```bash
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

---

## Project Name Change Summary

**Old Name:** esp32-hello  
**New Name:** rescape-client-device

**Files Updated:**

-   ✅ README.md created with full documentation
-   ✅ CHANGELOG.md already created
-   ✅ .gitignore already exists
-   ✅ All source code uses consistent naming

**No changes needed in:**

-   platformio.ini (project name not used)
-   Source code (uses generic names)
-   Pin definitions (board-independent)

---

## Suggested GitHub Settings

**Branch Protection (optional):**

-   Require pull request reviews
-   Require status checks before merge
-   Include administrators

**Topics/Tags:**

-   esp32
-   esp32c3
-   escape-room
-   iot
-   automation
-   embedded
-   rs485
-   i2c
-   keypad
-   neopixel
-   led-animation
-   audio-synthesis

---

**Created:** November 29, 2025
