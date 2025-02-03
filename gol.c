#include<raylib.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<assert.h>

#define sfor(var, n) for (uint32_t var = 0; var < (n); var++)

/* width assumed to be multiple of 8 */
typedef struct {
    uint32_t w; uint32_t h;
    uint8_t * data;
} cells_t;

cells_t * cells_alloc(uint32_t w, uint32_t h) {
    assert(!(w % 8));
    cells_t * ret = malloc(sizeof(cells_t));
    *ret = (cells_t) { w, h, calloc(w/8 * h, 1) };
    return ret;
}

void cells_free(cells_t * cells) { free(cells->data); free(cells); }

bool get_cell(cells_t * cells, int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= (int32_t) cells->w || y >= (int32_t) cells->h) return false;
    uint32_t index, bit;
    index = x/8 + y * (cells->w/8), bit = x % 8;
    return cells->data[index] & (1 << bit);
}

void set_cell(cells_t * cells, bool on, uint32_t x, uint32_t y) {
    if (x >= cells->w || y >= cells->h) return;
    uint32_t index, bit;
    index = x/8 + y * (cells->w/8), bit = x % 8;
    if (on) cells->data[index] |=  (1 << bit);
    else cells->data[index] &= ~(1 << bit);
}

/* won't count higher than 4 */
uint8_t count_neighbors(cells_t * cells, int32_t x, int32_t y) {
    uint32_t count = 0;
    for (int32_t i = x-1; i <= x+1; i++)
        for (int32_t j = y-1; j <= y+1; j++) {
            if (i == x && j == y) continue;
            count += get_cell(cells, i, j);
            if (count == 4) return count;
        }
    return count;
}

void step(cells_t ** cells) {
    static cells_t * new_cells;
    if (!new_cells) new_cells = cells_alloc((*cells)->w, (*cells)->h);
    assert(new_cells->w == (*cells)->w);
    assert(new_cells->h == (*cells)->h);

    sfor(j, (*cells)->h) sfor(i, (*cells)->w) {
        bool current = get_cell(*cells, i, j);
        switch (count_neighbors(*cells, i, j)) {
            case 3: set_cell(new_cells, true, i, j); break;
            case 2: set_cell(new_cells, current, i, j); break;
            default: set_cell(new_cells, false, i, j); break;
        }
    }
    cells_t * tmp = *cells;
    *cells = new_cells;
    new_cells = tmp;
}

Texture2D * cells_to_texture(cells_t * cells) {
    static Texture2D tex;
    static Color * pixels;
    static uint32_t width, height, loaded;
        
    if (!loaded) {
        width = cells->w, height = cells->h;
        pixels = malloc(width * height * sizeof(Color)); 
    }

    sfor(j, height) sfor(i, width)
        pixels[j*width + i] = get_cell(cells, i, j)? BLACK : WHITE;

    if (!loaded) {
        Image im = {
            .data = pixels, .width = width, .height = height,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
        };
        tex = LoadTextureFromImage(im);
        loaded = true;
    } else
        UpdateTexture(tex, pixels);

    return &tex;
}

#define screen_to_cell(xpos, ypos) \
    ((xpos) - texture_pos.x)/texture_scale, ((ypos) - texture_pos.y)/texture_scale
#define update() step(&cells), tex = cells_to_texture(cells)

int main(int argc, char * argv[]) {
    int32_t width, height;
    width = height = 96;

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
        printf("usage: %s [width] [height]\n  left click - spawn cell\n  right click - kill cell\n  tab - step\n  space - play/pause\n", argv[0]), exit(0);

    if (argc == 2) {
        width = atoi(argv[1]), height = width;
        if (!width || width % 8) width = width/8 * 8, fprintf(stderr, "enter a size divisible by 8\n");
    } else if (argc == 3) {
        width = atoi(argv[1]), height = atoi(argv[2]);
        if (!width || width % 8) width = width/8 * 8, fprintf(stderr, "enter a width divisible by 8\n");
        if (!height || height % 8) height = height/8 * 8, fprintf(stderr, "enter a height divisible by 8\n");
    }

    cells_t * cells = cells_alloc(width, height);

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(800, 400, "gol");
    SetTargetFPS(60);

    Texture2D * tex;
    tex = cells_to_texture(cells);

    float texture_scale;
    Vector2 texture_pos;

    uint32_t frames_per_step = 5;
    uint32_t frames_till_next = 0;
    bool playing = false;

    while (!WindowShouldClose()) {
        float screen_height = GetScreenHeight();
        texture_scale = screen_height / (float)cells->h;
        texture_pos = (Vector2) {
            (float)GetScreenWidth()/2 - (float)cells->w * texture_scale/2, 0
        };

        if (IsKeyPressed(KEY_TAB)) update();
        if (IsKeyPressed(KEY_SPACE)) playing = !playing;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            set_cell(cells, true, screen_to_cell(GetMouseX(), GetMouseY()));
            tex = cells_to_texture(cells);
        } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            set_cell(cells, false, screen_to_cell(GetMouseX(), GetMouseY()));
            tex = cells_to_texture(cells);
        }

        if (playing) {
            if (frames_till_next) frames_till_next--;
            else frames_till_next = frames_per_step, update();
        }

        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        DrawTextureEx(*tex, texture_pos, 0, texture_scale, WHITE);
        EndDrawing();
    }

    CloseWindow();
}
