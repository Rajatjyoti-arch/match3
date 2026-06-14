# Match 3 Game — Complete Monkey-Proof Blueprint

> Every constant, struct, function, and algorithm is spelled out.
> Just type what you read. No thinking required.

---

## TASK 1: Constants (top of match3.c)

```c
#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   700
#define GRID_ROWS       8
#define GRID_COLS       8
#define CELL_SIZE       64
#define NUM_GEM_TYPES   6
#define GRID_OFFSET_X   ((WINDOW_WIDTH - GRID_COLS * CELL_SIZE) / 2)   // = 144
#define GRID_OFFSET_Y   130
#define GEM_PADDING     3
#define SWAP_SPEED      5.0f
#define FALL_SPEED      600.0f    // pixels per second
#define REMOVE_SPEED    4.0f      // scale shrink per second
```

---

## TASK 2: Enums

### Gem Types
```c
enum {
    GEM_NONE   = 0,
    GEM_RED    = 1,
    GEM_BLUE   = 2,
    GEM_GREEN  = 3,
    GEM_YELLOW = 4,
    GEM_PURPLE = 5,
    GEM_ORANGE = 6
};
```

### Game States
```c
typedef enum {
    STATE_IDLE,
    STATE_SWAP,
    STATE_CHECK,
    STATE_REMOVE,
    STATE_FALL,
    STATE_SWAP_BACK
} GamePhase;
```

---

## TASK 3: Structs

### Gem
```c
typedef struct {
    int   type;      // GEM_NONE..GEM_ORANGE
    float offsetX;   // pixel offset for swap animation
    float offsetY;   // pixel offset for fall/swap animation
    float scale;     // 1.0 = full size, 0.0 = invisible (for remove anim)
    bool  matched;   // true = flagged for removal
} Gem;
```

### GameState
```c
typedef struct {
    Gem       grid[GRID_ROWS][GRID_COLS];
    int       selectedRow, selectedCol;  // -1 = nothing selected
    int       score;
    int       combo;                     // cascade multiplier
    GamePhase phase;

    // Swap tracking
    int   swapR1, swapC1, swapR2, swapC2;
    float swapProgress;   // 0.0 → 1.0
} GameState;
```

---

## TASK 4: Color Lookup Functions

### GetGemColor
```c
Color GetGemColor(int type) {
    switch (type) {
        case GEM_RED:    return (Color){220,  50,  50, 255};
        case GEM_BLUE:   return (Color){ 50, 100, 230, 255};
        case GEM_GREEN:  return (Color){ 50, 200,  80, 255};
        case GEM_YELLOW: return (Color){240, 220,  40, 255};
        case GEM_PURPLE: return (Color){180,  50, 220, 255};
        case GEM_ORANGE: return (Color){240, 150,  30, 255};
        default:         return BLANK;
    }
}
```

### GetGemHighlight (lighter version for inner shine)
```c
Color GetGemHighlight(int type) {
    switch (type) {
        case GEM_RED:    return (Color){255, 130, 130, 200};
        case GEM_BLUE:   return (Color){140, 180, 255, 200};
        case GEM_GREEN:  return (Color){140, 255, 160, 200};
        case GEM_YELLOW: return (Color){255, 250, 150, 200};
        case GEM_PURPLE: return (Color){230, 150, 255, 200};
        case GEM_ORANGE: return (Color){255, 210, 140, 200};
        default:         return BLANK;
    }
}
```

---

## TASK 5: InitGame Function

**Signature**: `void InitGame(GameState *g)`

**Algorithm** (step by step):
1. Set `g->score = 0`, `g->combo = 0`, `g->phase = STATE_IDLE`
2. Set `g->selectedRow = -1`, `g->selectedCol = -1`
3. For each `row` from 0 to `GRID_ROWS-1`:
   - For each `col` from 0 to `GRID_COLS-1`:
     - **Pick a random type that doesn't create an instant match:**
       ```
       do {
           type = GetRandomValue(1, NUM_GEM_TYPES);
       } while (
           (col >= 2
            && g->grid[row][col-1].type == type
            && g->grid[row][col-2].type == type)
           ||
           (row >= 2
            && g->grid[row-1][col].type == type
            && g->grid[row-2][col].type == type)
       );
       ```
     - Set `g->grid[row][col].type = type`
     - Set `g->grid[row][col].offsetX = 0.0f`
     - Set `g->grid[row][col].offsetY = 0.0f`
     - Set `g->grid[row][col].scale = 1.0f`
     - Set `g->grid[row][col].matched = false`

---

## TASK 6: HandleInput Function

**Signature**: `void HandleInput(GameState *g)`

**Only runs when** `g->phase == STATE_IDLE`.

**Algorithm**:
1. If `!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)` → return immediately.
2. Get `mouseX = GetMouseX()`, `mouseY = GetMouseY()`
3. Compute grid coords:
   ```
   col = (mouseX - GRID_OFFSET_X) / CELL_SIZE
   row = (mouseY - GRID_OFFSET_Y) / CELL_SIZE
   ```
4. If `row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS` → return (clicked outside grid).
5. If `g->selectedRow == -1` (nothing selected yet):
   - Set `g->selectedRow = row`, `g->selectedCol = col`
   - Return.
6. If clicked same gem as already selected (`row == g->selectedRow && col == g->selectedCol`):
   - Deselect: `g->selectedRow = -1`, `g->selectedCol = -1`
   - Return.
7. Check adjacency:
   ```
   dr = abs(row - g->selectedRow)
   dc = abs(col - g->selectedCol)
   ```
8. If `(dr == 1 && dc == 0) || (dr == 0 && dc == 1)` → **adjacent, start swap**:
   - `g->swapR1 = g->selectedRow`
   - `g->swapC1 = g->selectedCol`
   - `g->swapR2 = row`
   - `g->swapC2 = col`
   - `g->swapProgress = 0.0f`
   - `g->combo = 0`
   - `g->phase = STATE_SWAP`
   - `g->selectedRow = -1`, `g->selectedCol = -1`
9. Else → **not adjacent, re-select**:
   - `g->selectedRow = row`, `g->selectedCol = col`

---

## TASK 7: SwapGemData Helper

**Signature**: `void SwapGemData(GameState *g)`

Just swap the two gems in the array:
```c
Gem temp = g->grid[g->swapR1][g->swapC1];
g->grid[g->swapR1][g->swapC1] = g->grid[g->swapR2][g->swapC2];
g->grid[g->swapR2][g->swapC2] = temp;
```
Then reset all four offsets to 0:
```c
g->grid[g->swapR1][g->swapC1].offsetX = 0;
g->grid[g->swapR1][g->swapC1].offsetY = 0;
g->grid[g->swapR2][g->swapC2].offsetX = 0;
g->grid[g->swapR2][g->swapC2].offsetY = 0;
```

---

## TASK 8: UpdateSwap Function

**Signature**: `void UpdateSwap(GameState *g)`

**Runs when** `g->phase == STATE_SWAP`.

**Algorithm**:
1. `g->swapProgress += SWAP_SPEED * GetFrameTime()`
2. If `g->swapProgress > 1.0f` → clamp to `1.0f`
3. Compute visual offsets:
   ```
   dx = (g->swapC2 - g->swapC1) * CELL_SIZE * g->swapProgress
   dy = (g->swapR2 - g->swapR1) * CELL_SIZE * g->swapProgress
   ```
4. Apply to gem 1 (moving toward gem 2's position):
   ```
   g->grid[g->swapR1][g->swapC1].offsetX =  dx
   g->grid[g->swapR1][g->swapC1].offsetY =  dy
   ```
5. Apply to gem 2 (moving toward gem 1's position):
   ```
   g->grid[g->swapR2][g->swapC2].offsetX = -dx
   g->grid[g->swapR2][g->swapC2].offsetY = -dy
   ```
6. If `g->swapProgress >= 1.0f`:
   - Call `SwapGemData(g)` (swaps array data + resets offsets)
   - `g->phase = STATE_CHECK`

---

## TASK 9: UpdateSwapBack Function

**Signature**: `void UpdateSwapBack(GameState *g)`

**Runs when** `g->phase == STATE_SWAP_BACK`.

**Context**: Data was already swapped back in `FindMatches`. Gems are in original array slots but need to visually slide back from the "wrong" position to their home.

**Algorithm**:
1. `g->swapProgress += SWAP_SPEED * GetFrameTime()`
2. If `g->swapProgress > 1.0f` → clamp to `1.0f`
3. `remaining = 1.0f - g->swapProgress`
4. Compute offsets (shrinking toward 0):
   ```
   g->grid[g->swapR1][g->swapC1].offsetX = (g->swapC2 - g->swapC1) * CELL_SIZE * remaining
   g->grid[g->swapR1][g->swapC1].offsetY = (g->swapR2 - g->swapR1) * CELL_SIZE * remaining
   g->grid[g->swapR2][g->swapC2].offsetX = (g->swapC1 - g->swapC2) * CELL_SIZE * remaining
   g->grid[g->swapR2][g->swapC2].offsetY = (g->swapR1 - g->swapR2) * CELL_SIZE * remaining
   ```
5. If `g->swapProgress >= 1.0f`:
   - Reset all four offsets to 0
   - `g->phase = STATE_IDLE`

---

## TASK 10: FindAndMarkMatches Function

**Signature**: `bool FindAndMarkMatches(GameState *g)`

**Runs when** `g->phase == STATE_CHECK`.

**Algorithm**:
1. `matchFound = false`
2. **Horizontal scan**:
   ```
   for row = 0 to GRID_ROWS-1:
       for col = 0 to GRID_COLS-3:   // up to col 5
           type = g->grid[row][col].type
           if type != GEM_NONE
              && g->grid[row][col+1].type == type
              && g->grid[row][col+2].type == type:
               // Mark entire horizontal run
               k = col
               while k < GRID_COLS && g->grid[row][k].type == type:
                   g->grid[row][k].matched = true
                   k++
               matchFound = true
   ```
3. **Vertical scan**:
   ```
   for col = 0 to GRID_COLS-1:
       for row = 0 to GRID_ROWS-3:   // up to row 5
           type = g->grid[row][col].type
           if type != GEM_NONE
              && g->grid[row+1][col].type == type
              && g->grid[row+2][col].type == type:
               k = row
               while k < GRID_ROWS && g->grid[k][col].type == type:
                   g->grid[k][col].matched = true
                   k++
               matchFound = true
   ```
4. If `matchFound`:
   - `g->combo++`
   - Count matched gems: loop all cells, count where `.matched == true`
   - `g->score += count * 10 * g->combo`
   - `g->phase = STATE_REMOVE`
5. Else if `g->combo == 0` (swap produced no match):
   - Call `SwapGemData(g)` (swap data back to original)
   - Set initial offsets for swap-back animation:
     ```
     g->grid[g->swapR1][g->swapC1].offsetX = (g->swapC2 - g->swapC1) * CELL_SIZE
     g->grid[g->swapR1][g->swapC1].offsetY = (g->swapR2 - g->swapR1) * CELL_SIZE
     g->grid[g->swapR2][g->swapC2].offsetX = (g->swapC1 - g->swapC2) * CELL_SIZE
     g->grid[g->swapR2][g->swapC2].offsetY = (g->swapR1 - g->swapR2) * CELL_SIZE
     ```
   - `g->swapProgress = 0.0f`
   - `g->phase = STATE_SWAP_BACK`
6. Else (cascade ended, no more matches):
   - `g->combo = 0`
   - `g->phase = STATE_IDLE`
7. Return `matchFound`

---

## TASK 11: UpdateRemove Function

**Signature**: `void UpdateRemove(GameState *g)`

**Runs when** `g->phase == STATE_REMOVE`.

**Algorithm**:
1. `allDone = true`
2. For each `row` 0..7, `col` 0..7:
   - If `g->grid[row][col].matched`:
     - `g->grid[row][col].scale -= REMOVE_SPEED * GetFrameTime()`
     - If `g->grid[row][col].scale <= 0.0f`:
       - `g->grid[row][col].scale = 0.0f`
       - `g->grid[row][col].type = GEM_NONE`
       - `g->grid[row][col].matched = false`
     - Else:
       - `allDone = false`
3. If `allDone`:
   - Call `ApplyGravity(g)`
   - Call `FillEmptyCells(g)`
   - `g->phase = STATE_FALL`

---

## TASK 12: ApplyGravity Function

**Signature**: `void ApplyGravity(GameState *g)`

**Purpose**: For each column, move gems down to fill gaps. Set `offsetY` so they animate.

**Algorithm** (per column):
```
for col = 0 to GRID_COLS-1:
    writeRow = GRID_ROWS - 1    // start from bottom
    for row = GRID_ROWS-1 down to 0:
        if g->grid[row][col].type != GEM_NONE:
            if writeRow != row:
                // Move gem from row to writeRow
                g->grid[writeRow][col] = g->grid[row][col]
                g->grid[writeRow][col].offsetY = (float)(row - writeRow) * -CELL_SIZE
                // offsetY is negative because gem starts ABOVE target
                // e.g., if gem moves down 2 rows: offsetY = -128
                g->grid[row][col].type = GEM_NONE
                g->grid[row][col].matched = false
                g->grid[row][col].scale = 1.0f
                g->grid[row][col].offsetY = 0.0f
            writeRow--
```

---

## TASK 13: FillEmptyCells Function

**Signature**: `void FillEmptyCells(GameState *g)`

**Purpose**: Fill remaining GEM_NONE cells at top of each column with new random gems, with negative offsetY so they fall in from above.

**Algorithm**:
```
for col = 0 to GRID_COLS-1:
    // Count empty cells in this column
    emptyCount = 0
    for row = 0 to GRID_ROWS-1:
        if g->grid[row][col].type == GEM_NONE:
            emptyCount++

    // Fill from top
    fillIndex = 0
    for row = 0 to GRID_ROWS-1:
        if g->grid[row][col].type == GEM_NONE:
            g->grid[row][col].type = GetRandomValue(1, NUM_GEM_TYPES)
            g->grid[row][col].matched = false
            g->grid[row][col].scale = 1.0f
            g->grid[row][col].offsetX = 0.0f
            // Start above the grid: further up for higher gems
            g->grid[row][col].offsetY = -(float)(emptyCount - fillIndex) * CELL_SIZE
            fillIndex++
```

---

## TASK 14: UpdateFall Function

**Signature**: `void UpdateFall(GameState *g)`

**Runs when** `g->phase == STATE_FALL`.

**Algorithm**:
1. `allSettled = true`
2. For each `row`, `col`:
   - If `g->grid[row][col].offsetY < -0.5f`:
     - `g->grid[row][col].offsetY += FALL_SPEED * GetFrameTime()`
     - If `g->grid[row][col].offsetY > 0.0f`:
       - `g->grid[row][col].offsetY = 0.0f`
     - Else:
       - `allSettled = false`
3. If `allSettled`:
   - `g->phase = STATE_CHECK` (cascade check)

---

## TASK 15: UpdateGame Function

**Signature**: `void UpdateGame(GameState *g)`

This is the master update dispatcher. Just a switch:
```c
switch (g->phase) {
    case STATE_IDLE:      HandleInput(g);            break;
    case STATE_SWAP:      UpdateSwap(g);             break;
    case STATE_CHECK:     FindAndMarkMatches(g);     break;
    case STATE_REMOVE:    UpdateRemove(g);           break;
    case STATE_FALL:      UpdateFall(g);             break;
    case STATE_SWAP_BACK: UpdateSwapBack(g);         break;
}
```

---

## TASK 16: DrawGem Function

**Signature**: `void DrawGem(int type, float cx, float cy, float scale, bool selected)`

**Parameters**: `cx, cy` = center of cell in screen coords.

**Algorithm**:
1. If `type == GEM_NONE` → return.
2. `halfSize = (CELL_SIZE - 2*GEM_PADDING) * scale / 2.0f`
3. `rect = (Rectangle){cx - halfSize, cy - halfSize, halfSize*2, halfSize*2}`
4. Draw shadow: `DrawRectangleRounded({rect.x+2, rect.y+2, rect.width, rect.height}, 0.3f, 6, (Color){0,0,0,60})`
5. Draw body: `DrawRectangleRounded(rect, 0.3f, 6, GetGemColor(type))`
6. Draw highlight (inner shine, top-left area):
   ```
   hlRect = {cx - halfSize*0.5f, cy - halfSize*0.7f, halfSize*1.0f, halfSize*0.5f}
   DrawRectangleRounded(hlRect, 0.5f, 6, GetGemHighlight(type))
   ```
7. If `selected`:
   ```
   // Pulsing white border
   pulse = 0.7f + 0.3f * sinf(GetTime() * 6.0f)
   alpha = (unsigned char)(255 * pulse)
   DrawRectangleRoundedLinesEx(
       (Rectangle){cx-halfSize-3, cy-halfSize-3, halfSize*2+6, halfSize*2+6},
       0.3f, 6, 3.0f, (Color){255,255,255,alpha}
   )
   ```

---

## TASK 17: DrawGame Function

**Signature**: `void DrawGame(const GameState *g)`

**Algorithm**:
```
BeginDrawing()
    // 1. Background gradient
    DrawRectangleGradientV(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        (Color){15, 10, 40, 255}, (Color){30, 20, 60, 255})

    // 2. Title
    DrawText("MATCH 3", WINDOW_WIDTH/2 - MeasureText("MATCH 3", 40)/2, 20, 40,
        (Color){255, 220, 100, 255})

    // 3. Score
    char scoreBuf[64]
    sprintf(scoreBuf, "Score: %d", g->score)
    DrawText(scoreBuf, WINDOW_WIDTH/2 - MeasureText(scoreBuf, 28)/2, 72, 28, WHITE)

    // 4. Combo indicator (only if combo > 1)
    if (g->combo > 1) {
        char comboBuf[32]
        sprintf(comboBuf, "x%d COMBO!", g->combo)
        DrawText(comboBuf, WINDOW_WIDTH/2 - MeasureText(comboBuf,24)/2, 100, 24,
            (Color){255, 100, 100, 255})
    }

    // 5. Grid background
    DrawRectangleRounded(
        (Rectangle){GRID_OFFSET_X-6, GRID_OFFSET_Y-6,
                     GRID_COLS*CELL_SIZE+12, GRID_ROWS*CELL_SIZE+12},
        0.02f, 6, (Color){255,255,255,20})

    // 6. Grid cell backgrounds (checkerboard)
    for row = 0 to 7:
        for col = 0 to 7:
            x = GRID_OFFSET_X + col * CELL_SIZE
            y = GRID_OFFSET_Y + row * CELL_SIZE
            shade = ((row + col) % 2 == 0) ? 25 : 35
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, (Color){shade, shade, shade+15, 255})

    // 7. Draw each gem
    for row = 0 to 7:
        for col = 0 to 7:
            gem = g->grid[row][col]
            if gem.type == GEM_NONE: continue
            cx = GRID_OFFSET_X + col*CELL_SIZE + CELL_SIZE/2.0f + gem.offsetX
            cy = GRID_OFFSET_Y + row*CELL_SIZE + CELL_SIZE/2.0f + gem.offsetY
            isSelected = (row == g->selectedRow && col == g->selectedCol)
            DrawGem(gem.type, cx, cy, gem.scale, isSelected)

EndDrawing()
```

---

## TASK 18: main Function

```c
int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Match 3");
    SetTargetFPS(60);

    GameState game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        UpdateGame(&game);
        DrawGame(&game);
    }

    CloseWindow();
    return 0;
}
```

---

## TASK 19: Makefile

```makefile
CC = gcc
CFLAGS = -Wall -std=c99 -I./raylib_src/src
LDFLAGS = -L./raylib_src/src -lraylib -lm -lpthread -ldl -lrt -lX11 -lGL

match3: match3.c
	$(CC) $(CFLAGS) -o match3 match3.c $(LDFLAGS)

clean:
	rm -f match3

run: match3
	./match3
```

**Use tabs (not spaces) before the recipe lines.**

---

## TASK 20: Build & Run

```bash
cd /home/rajat/C
make
./match3
```

---

## COMPLETE FUNCTION ORDER IN match3.c

Write them in this exact order so forward declarations aren't needed:
1. `#include` and `#define` (Task 1)
2. Enums (Task 2)
3. Structs (Task 3)
4. `GetGemColor()` (Task 4)
5. `GetGemHighlight()` (Task 4)
6. `InitGame()` (Task 5)
7. `HandleInput()` (Task 6)
8. `SwapGemData()` (Task 7)
9. `FindAndMarkMatches()` (Task 10)
10. `UpdateSwap()` (Task 8)
11. `UpdateSwapBack()` (Task 9)
12. `UpdateRemove()` (Task 11)
13. `ApplyGravity()` (Task 12)
14. `FillEmptyCells()` (Task 13)
15. `UpdateFall()` (Task 14)
16. `UpdateGame()` (Task 15)
17. `DrawGem()` (Task 16)
18. `DrawGame()` (Task 17)
19. `main()` (Task 18)
