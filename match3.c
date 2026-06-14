#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
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

enum {
    GEM_NONE   = 0,
    GEM_RED    = 1,
    GEM_BLUE   = 2,
    GEM_GREEN  = 3,
    GEM_YELLOW = 4,
    GEM_PURPLE = 5,
    GEM_ORANGE = 6
};

typedef enum {
    STATE_IDLE,
    STATE_SWAP,
    STATE_CHECK,
    STATE_REMOVE,
    STATE_FALL,
    STATE_SWAP_BACK
} GamePhase;

typedef struct {
    int   type;      // GEM_NONE..GEM_ORANGE
    float offsetX;   // pixel offset for swap animation
    float offsetY;   // pixel offset for fall/swap animation
    float scale;     // 1.0 = full size, 0.0 = invisible (for remove anim)
    bool  matched;   // true = flagged for removal
} Gem;

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

void InitGame(GameState *g) {
    g->score = 0;
    g->combo = 0;
    g->phase = STATE_IDLE;
    g->selectedRow = -1;
    g->selectedCol = -1;
    
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int type;
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
            
            g->grid[row][col].type = type;
            g->grid[row][col].offsetX = 0.0f;
            g->grid[row][col].offsetY = 0.0f;
            g->grid[row][col].scale = 1.0f;
            g->grid[row][col].matched = false;
        }
    }
}

void HandleInput(GameState *g) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    
    int mouseX = GetMouseX();
    int mouseY = GetMouseY();
    
    // Check raw pixel bounds BEFORE integer division to avoid
    // negative-division-truncation bug (e.g. (-1)/64 == 0 in C)
    if (mouseX < GRID_OFFSET_X || mouseY < GRID_OFFSET_Y) return;
    
    int col = (mouseX - GRID_OFFSET_X) / CELL_SIZE;
    int row = (mouseY - GRID_OFFSET_Y) / CELL_SIZE;
    
    if (row >= GRID_ROWS || col >= GRID_COLS) return;
    
    if (g->selectedRow == -1) {
        g->selectedRow = row;
        g->selectedCol = col;
        return;
    }
    
    if (row == g->selectedRow && col == g->selectedCol) {
        g->selectedRow = -1;
        g->selectedCol = -1;
        return;
    }
    
    int dr = abs(row - g->selectedRow);
    int dc = abs(col - g->selectedCol);
    
    if ((dr == 1 && dc == 0) || (dr == 0 && dc == 1)) {
        g->swapR1 = g->selectedRow;
        g->swapC1 = g->selectedCol;
        g->swapR2 = row;
        g->swapC2 = col;
        g->swapProgress = 0.0f;
        g->combo = 0;
        g->phase = STATE_SWAP;
        g->selectedRow = -1;
        g->selectedCol = -1;
    } else {
        g->selectedRow = row;
        g->selectedCol = col;
    }
}

void SwapGemData(GameState *g) {
    Gem temp = g->grid[g->swapR1][g->swapC1];
    g->grid[g->swapR1][g->swapC1] = g->grid[g->swapR2][g->swapC2];
    g->grid[g->swapR2][g->swapC2] = temp;
    
    g->grid[g->swapR1][g->swapC1].offsetX = 0;
    g->grid[g->swapR1][g->swapC1].offsetY = 0;
    g->grid[g->swapR2][g->swapC2].offsetX = 0;
    g->grid[g->swapR2][g->swapC2].offsetY = 0;
}

void UpdateSwap(GameState *g) {
    g->swapProgress += SWAP_SPEED * GetFrameTime();
    if (g->swapProgress > 1.0f) g->swapProgress = 1.0f;
    
    float dx = (g->swapC2 - g->swapC1) * CELL_SIZE * g->swapProgress;
    float dy = (g->swapR2 - g->swapR1) * CELL_SIZE * g->swapProgress;
    
    g->grid[g->swapR1][g->swapC1].offsetX = dx;
    g->grid[g->swapR1][g->swapC1].offsetY = dy;
    
    g->grid[g->swapR2][g->swapC2].offsetX = -dx;
    g->grid[g->swapR2][g->swapC2].offsetY = -dy;
    
    if (g->swapProgress >= 1.0f) {
        SwapGemData(g);
        g->phase = STATE_CHECK;
    }
}

void UpdateSwapBack(GameState *g) {
    g->swapProgress += SWAP_SPEED * GetFrameTime();
    if (g->swapProgress > 1.0f) g->swapProgress = 1.0f;
    
    float remaining = 1.0f - g->swapProgress;
    
    g->grid[g->swapR1][g->swapC1].offsetX = (g->swapC2 - g->swapC1) * CELL_SIZE * remaining;
    g->grid[g->swapR1][g->swapC1].offsetY = (g->swapR2 - g->swapR1) * CELL_SIZE * remaining;
    g->grid[g->swapR2][g->swapC2].offsetX = (g->swapC1 - g->swapC2) * CELL_SIZE * remaining;
    g->grid[g->swapR2][g->swapC2].offsetY = (g->swapR1 - g->swapR2) * CELL_SIZE * remaining;
    
    if (g->swapProgress >= 1.0f) {
        g->grid[g->swapR1][g->swapC1].offsetX = 0;
        g->grid[g->swapR1][g->swapC1].offsetY = 0;
        g->grid[g->swapR2][g->swapC2].offsetX = 0;
        g->grid[g->swapR2][g->swapC2].offsetY = 0;
        g->phase = STATE_IDLE;
    }
}

bool FindAndMarkMatches(GameState *g) {
    bool matchFound = false;
    
    // Horizontal scan
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS - 2; col++) {
            int type = g->grid[row][col].type;
            if (type != GEM_NONE 
                && g->grid[row][col+1].type == type 
                && g->grid[row][col+2].type == type) {
                
                int k = col;
                while (k < GRID_COLS && g->grid[row][k].type == type) {
                    g->grid[row][k].matched = true;
                    k++;
                }
                matchFound = true;
            }
        }
    }
    
    // Vertical scan
    for (int col = 0; col < GRID_COLS; col++) {
        for (int row = 0; row < GRID_ROWS - 2; row++) {
            int type = g->grid[row][col].type;
            if (type != GEM_NONE 
                && g->grid[row+1][col].type == type 
                && g->grid[row+2][col].type == type) {
                
                int k = row;
                while (k < GRID_ROWS && g->grid[k][col].type == type) {
                    g->grid[k][col].matched = true;
                    k++;
                }
                matchFound = true;
            }
        }
    }
    
    if (matchFound) {
        g->combo++;
        int count = 0;
        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                if (g->grid[row][col].matched) count++;
            }
        }
        g->score += count * 10 * g->combo;
        g->phase = STATE_REMOVE;
    } else if (g->combo == 0) {
        SwapGemData(g);
        
        g->grid[g->swapR1][g->swapC1].offsetX = (g->swapC2 - g->swapC1) * CELL_SIZE;
        g->grid[g->swapR1][g->swapC1].offsetY = (g->swapR2 - g->swapR1) * CELL_SIZE;
        g->grid[g->swapR2][g->swapC2].offsetX = (g->swapC1 - g->swapC2) * CELL_SIZE;
        g->grid[g->swapR2][g->swapC2].offsetY = (g->swapR1 - g->swapR2) * CELL_SIZE;
        
        g->swapProgress = 0.0f;
        g->phase = STATE_SWAP_BACK;
    } else {
        g->combo = 0;
        g->phase = STATE_IDLE;
    }
    
    return matchFound;
}

// Forward declarations because the blueprint has UpdateRemove before these functions
void ApplyGravity(GameState *g);
void FillEmptyCells(GameState *g);

void UpdateRemove(GameState *g) {
    bool allDone = true;
    
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            if (g->grid[row][col].matched) {
                g->grid[row][col].scale -= REMOVE_SPEED * GetFrameTime();
                if (g->grid[row][col].scale <= 0.0f) {
                    g->grid[row][col].scale = 0.0f;
                    g->grid[row][col].type = GEM_NONE;
                    g->grid[row][col].matched = false;
                } else {
                    allDone = false;
                }
            }
        }
    }
    
    if (allDone) {
        ApplyGravity(g);
        FillEmptyCells(g);
        g->phase = STATE_FALL;
    }
}

void ApplyGravity(GameState *g) {
    for (int col = 0; col < GRID_COLS; col++) {
        int writeRow = GRID_ROWS - 1;
        for (int row = GRID_ROWS - 1; row >= 0; row--) {
            if (g->grid[row][col].type != GEM_NONE) {
                if (writeRow != row) {
                    g->grid[writeRow][col] = g->grid[row][col];
                    g->grid[writeRow][col].offsetY = (float)(row - writeRow) * CELL_SIZE;
                    
                    g->grid[row][col].type = GEM_NONE;
                    g->grid[row][col].matched = false;
                    g->grid[row][col].scale = 1.0f;
                    g->grid[row][col].offsetY = 0.0f;
                }
                writeRow--;
            }
        }
    }
}

void FillEmptyCells(GameState *g) {
    for (int col = 0; col < GRID_COLS; col++) {
        int emptyCount = 0;
        for (int row = 0; row < GRID_ROWS; row++) {
            if (g->grid[row][col].type == GEM_NONE) {
                emptyCount++;
            }
        }
        
        int fillIndex = 0;
        for (int row = 0; row < GRID_ROWS; row++) {
            if (g->grid[row][col].type == GEM_NONE) {
                g->grid[row][col].type = GetRandomValue(1, NUM_GEM_TYPES);
                g->grid[row][col].matched = false;
                g->grid[row][col].scale = 1.0f;
                g->grid[row][col].offsetX = 0.0f;
                g->grid[row][col].offsetY = -(float)(emptyCount - fillIndex) * CELL_SIZE;
                fillIndex++;
            }
        }
    }
}

void UpdateFall(GameState *g) {
    bool allSettled = true;
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            if (g->grid[row][col].offsetY < -0.5f) {
                g->grid[row][col].offsetY += FALL_SPEED * GetFrameTime();
                if (g->grid[row][col].offsetY > 0.0f) {
                    g->grid[row][col].offsetY = 0.0f;
                } else {
                    allSettled = false;
                }
            }
        }
    }
    
    if (allSettled) {
        g->phase = STATE_CHECK;
    }
}

void UpdateGame(GameState *g) {
    switch (g->phase) {
        case STATE_IDLE:      HandleInput(g);            break;
        case STATE_SWAP:      UpdateSwap(g);             break;
        case STATE_CHECK:     FindAndMarkMatches(g);     break;
        case STATE_REMOVE:    UpdateRemove(g);           break;
        case STATE_FALL:      UpdateFall(g);             break;
        case STATE_SWAP_BACK: UpdateSwapBack(g);         break;
    }
}

void DrawGem(int type, float cx, float cy, float scale, bool selected) {
    if (type == GEM_NONE || scale < 0.01f) return;
    
    float halfSize = (CELL_SIZE - 2*GEM_PADDING) * scale / 2.0f;
    Rectangle rect = {cx - halfSize, cy - halfSize, halfSize*2, halfSize*2};
    
    DrawRectangleRounded((Rectangle){rect.x+2, rect.y+2, rect.width, rect.height}, 0.3f, 6, (Color){0,0,0,60});
    DrawRectangleRounded(rect, 0.3f, 6, GetGemColor(type));
    
    Rectangle hlRect = {cx - halfSize*0.5f, cy - halfSize*0.7f, halfSize*1.0f, halfSize*0.5f};
    DrawRectangleRounded(hlRect, 0.5f, 6, GetGemHighlight(type));
    
    if (selected) {
        float pulse = 0.7f + 0.3f * sinf(GetTime() * 6.0f);
        unsigned char alpha = (unsigned char)(255 * pulse);
        DrawRectangleRoundedLinesEx(
            (Rectangle){cx-halfSize-3, cy-halfSize-3, halfSize*2+6, halfSize*2+6},
            0.3f, 6, 3.0f, (Color){255,255,255,alpha}
        );
    }
}



void DrawGame(const GameState *g) {
    BeginDrawing();
    
    DrawRectangleGradientV(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        (Color){15, 10, 40, 255}, (Color){30, 20, 60, 255});

    DrawText("MATCH 3", WINDOW_WIDTH/2 - MeasureText("MATCH 3", 40)/2, 20, 40,
        (Color){255, 220, 100, 255});

    char scoreBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "Score: %d", g->score);
    DrawText(scoreBuf, WINDOW_WIDTH/2 - MeasureText(scoreBuf, 28)/2, 72, 28, WHITE);

    if (g->combo > 1) {
        char comboBuf[32];
        snprintf(comboBuf, sizeof(comboBuf), "x%d COMBO!", g->combo);
        DrawText(comboBuf, WINDOW_WIDTH/2 - MeasureText(comboBuf,24)/2, 100, 24,
            (Color){255, 100, 100, 255});
    }

    DrawRectangleRounded(
        (Rectangle){GRID_OFFSET_X-6, GRID_OFFSET_Y-6,
                     GRID_COLS*CELL_SIZE+12, GRID_ROWS*CELL_SIZE+12},
        0.02f, 6, (Color){255,255,255,20});

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            float x = GRID_OFFSET_X + col * CELL_SIZE;
            float y = GRID_OFFSET_Y + row * CELL_SIZE;
            unsigned char shade = ((row + col) % 2 == 0) ? 25 : 35;
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, (Color){shade, shade, shade+15, 255});
        }
    }

    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            Gem gem = g->grid[row][col];
            if (gem.type == GEM_NONE) continue;
            
            float cx = GRID_OFFSET_X + col*CELL_SIZE + CELL_SIZE/2.0f + gem.offsetX;
            float cy = GRID_OFFSET_Y + row*CELL_SIZE + CELL_SIZE/2.0f + gem.offsetY;
            bool isSelected = (row == g->selectedRow && col == g->selectedCol);
            
            DrawGem(gem.type, cx, cy, gem.scale, isSelected);
        }
    }

    EndDrawing();
}

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
