# RollRush – 3D Racing Ball Game

An interactive 3D two-player racing game built using **C++**, **OpenGL**, and **FreeGLUT**.  
Developed as the final project for **CSAI 360 – Computer Graphics** in Spring 2025.

# Project Overview

**RollRush** is a split-screen multiplayer ball racing game where players race along curved platform tiles, collect checkpoints and treasures, and survive by managing health and lives.  
It showcases practical implementation of OpenGL rendering, camera systems, user input, HUD overlays, texture mapping, and physics-based mechanics.

# Game Features

- 🧑‍🤝‍🧑 **Single Player & Multiplayer Modes**
- 🛣️ **Curved 3D Track** built from connected platform segments
- 💥 **Collision Detection** and player respawn on fall
- ⛳ **Checkpoint System** – Only one player can claim each
- 🏆 **Victory Effects** and end screen with scores
- ❤️ **HUD Elements** – Health bars, lives (textured hearts), score overlays
- 🖥️ **Split-Screen Rendering** for 2 players
- 📜 **Game Modes**:
  - **Single Player**
  - **Multiplayer Race**
  - **Timed Score Attack**
  - **Treasure Hunt**

# Controls

### Player 1 (WASD):
- `W` / `S` → Move Forward / Backward  
- `A` / `D` → Turn Left / Right

### Player 2 (Arrow Keys):
- `↑` / `↓` → Move Forward / Backward  
- `←` / `→` → Turn Left / Right

### Global:
- `ENTER` → Confirm Menu / Input  
- `ESC` → Go Back  
- `BACKSPACE` → Delete during name input

# Technologies Used

- **C++**
- **OpenGL**
- **FreeGLUT**
- **stb_image.h** – for PNG texture loading (e.g., heart icon)
- **Modular Source Files**:
  - `main.cpp` – Game logic, track generation, gameplay rendering
  - `screen.cpp` – UI and menu system
  - `stb_image.h` – Image loading header
  - `heart.png` – Textured life icon

# Project Structure

📦 RollRush
<br>├── main.cpp # Core game logic and rendering
<br>├── screen.cpp # Menu and UI logic
<br>├── stb_image.h # Texture loader
<br>├── heart.png # Heart texture for lives display
<br>├── Final Project Report.pdf

# Highlights

- Real-time split-screen rendering for multiplayer
- Perspective camera updates based on player orientation
- Checkpoint and pickup logic with visual feedback
- Floating text overlays for scoring messages (e.g., "+20 pts!")
- Seamless transitions between menus and gameplay
- Anti-collision: Players can't pick the same color
- Game ends with win/loss animations and score comparison

# License
<br>This project was created for educational purposes and is shared under academic fair-use guidelines.
