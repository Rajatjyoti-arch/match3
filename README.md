# Raylib 2D ASCII Match 3

A simple, fast-paced Match-3 puzzle game built in C using the [Raylib](https://www.raylib.com/) library. 

## How to Play
- **Select a Tile:** Left-click on any tile on the board to select it (it will be highlighted in yellow).
- **Swap Tiles:** Left-click on an adjacent tile (up, down, left, or right) to swap their positions.
- **Match:** Align 3 or more identical characters horizontally or vertically to clear them and score points!
- **Cascades:** New tiles will fall from the top of the screen to fill empty spaces, potentially causing chain reactions.

## Installation & Compilation

This guide assumes you already have a basic C compiler (like `gcc` or `clang`) installed on your system.

### Dependencies (Raylib)
You must have Raylib installed to compile this game. The compilation flags differ slightly depending on your operating system.

#### Linux
Install the Raylib development package via your package manager. For Ubuntu/Debian:
```bash
sudo apt update
sudo apt install libraylib-dev
```
*(If you are compiling against a local source build of Raylib, you will need to add `-I./path/to/raylib/src` and `-L./path/to/raylib/src` to the command below).*

**Compile and run:**
```bash
gcc match3_1.c -o match3 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./match3
```

#### Windows
The easiest way to compile on Windows is using MinGW-w64.
1. Download the pre-compiled Raylib Windows binaries from the official [Raylib GitHub Releases](https://github.com/raysan5/raylib/releases).
2. Place the `raylib.h` file in your include path and `libraylib.a` in your library path.

**Compile and run:**
```cmd
gcc match3_1.c -o match3.exe -O1 -Wall -std=c99 -Wno-missing-braces -I C:/path/to/raylib/include -L C:/path/to/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
match3.exe
```

#### macOS
The easiest way to install Raylib on macOS is using Homebrew:
```bash
brew install raylib
```

**Compile and run:**
```bash
gcc match3_1.c -o match3 -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
./match3
```

## Assets
Make sure that the `assets/` folder (containing your background images, sound effects, and music) is located in the exact same directory as your compiled executable when you run the game!
