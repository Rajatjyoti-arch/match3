# Match 3 Game - Project Memory

## Project Info
- **Language**: C (std=c99)
- **Library**: Raylib (built from source)
- **Raylib Location**: `/home/rajat/C/raylib_src/` (static lib at `raylib_src/src/libraylib.a`, headers at `raylib_src/src/raylib.h`)
- **OS**: Ubuntu 26.04 LTS
- **Compiler**: GCC 15

## File Structure
```
/home/rajat/C/
├── match3.c          # Entire game (single file)
├── Makefile           # Build script
├── memory.md          # This file
├── tasklist.md        # Detailed implementation plan
└── raylib_src/        # Raylib source (already cloned & built)
    └── src/
        ├── raylib.h
        └── libraylib.a
```

## Build Command
```bash
gcc -o match3 match3.c -I./raylib_src/src -L./raylib_src/src -lraylib -lm -lpthread -ldl -lrt -lX11 -lGL
```

## Key Design Decisions
- Single file (`match3.c`) for simplicity
- 8x8 grid, 6 gem colors
- State machine architecture for game flow
- Per-gem offset floats for smooth animations
- No audio (keeping it basic)
- Fixed 800x700 window

## Current Progress
- [x] Raylib cloned and built
- [x] X11 dev dependencies installed
- [ ] tasklist.md created
- [ ] match3.c coded
- [ ] Makefile created
- [ ] Game compiled and tested
