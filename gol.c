#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<stdint.h>
#include<stdbool.h>
#include<assert.h>

#include<raylib.h>

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

#define sfor(var, n) for (u32 var = 0; var < (n); var++)

typedef uint32_t u32;
typedef int32_t i32;
typedef uint8_t u8;
typedef float fl;

/* width assumed to be multiple of 8 */
typedef struct {
    u32 w; u32 h;
    u8 * data;
} cells_t;

cells_t * cells_alloc(u32 w, u32 h) {
    assert(!(w % 8));
    cells_t * ret = malloc(sizeof(cells_t));
    *ret = (cells_t) { w, h, calloc(w/8 * h, 1) };
    return ret;
}

void cells_free(cells_t * cells) { free(cells->data); free(cells); }

bool get_cell(cells_t * cells, i32 x, i32 y) {
    if (x < 0 || y < 0 || x >= (i32) cells->w || y >= (i32) cells->h) return false;
    u32 index, bit;
    index = x/8 + y * (cells->w/8), bit = x % 8;
    return cells->data[index] & (1 << bit);
}

void set_cell(cells_t * cells, bool on, u32 x, u32 y) {
    if (x >= cells->w || y >= cells->h) return;
    u32 index, bit;
    index = x/8 + y * (cells->w/8), bit = x % 8;
    if (on) cells->data[index] |=  (1 << bit);
    else cells->data[index] &= ~(1 << bit);
}

void clear(cells_t * cells) {
    memset(cells->data, 0, cells->w/8 * cells->h);
}

/* won't count higher than 4 */
u8 count_neighbors(cells_t * cells, i32 x, i32 y) {
    u32 count = 0;
    for (i32 i = x-1; i <= x+1; i++)
        for (i32 j = y-1; j <= y+1; j++) {
            if (i == x && j == y) continue;
            count += get_cell(cells, i, j);
            if (count == 4) return count;
        }
    return count;
}

void step(cells_t * cells) {
    static cells_t * new_cells;
    if (!new_cells) new_cells = cells_alloc(cells->w, cells->h);
    assert(new_cells->w == cells->w);
    assert(new_cells->h == cells->h);

    sfor(j, cells->h) sfor(i, cells->w) {
        bool current = get_cell(cells, i, j);
        switch (count_neighbors(cells, i, j)) {
            case 3: set_cell(new_cells, true, i, j); break;
            case 2: set_cell(new_cells, current, i, j); break;
            default: set_cell(new_cells, false, i, j); break;
        }
    }
    cells_t tmp = *cells;
    *cells = *new_cells;
    *new_cells = tmp;
}

Texture2D * cells_to_texture(cells_t * cells) {
    static Texture2D tex;
    static u8 * pixels;
    static u32 width, height, loaded;
        
    if (!loaded) {
        width = cells->w, height = cells->h;
        pixels = malloc(width * height);
    }

    sfor(j, height) sfor(i, width)
        pixels[j*width + i] = !get_cell(cells, i, j) * 255;

    if (!loaded) {
        Image im = {
            .data = pixels, .width = width, .height = height,
            .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
            .mipmaps = 1
        };
        tex = LoadTextureFromImage(im);
        loaded = true;
    } else
        UpdateTexture(tex, pixels);

    return &tex;
}

void set_cells(cells_t * cells, bool value, i32 d, i32 x, i32 y) {
    for (i32 i = -d/2; i < (d+1)/2; i++)
    for (i32 j = -d/2; j < (d+1)/2; j++) {
        set_cell(cells, value, x + i, y + j);
    }
}

#define screen_to_cell(xpos, ypos) \
    ((xpos) - texture_pos.x)/texture_scale, ((ypos) - texture_pos.y)/texture_scale
#define update() step(cells), tex = cells_to_texture(cells)

void loop(cells_t * cells) {
    const i32 HINT_TIME = 50;
    const fl ZOOM_RATE = 0.125;
    static Texture2D * tex;
    static char hint_text[32];
    static bool playing = false;
    static fl zoom, pan_x, pan_y;
    static u32 frames_per_step = 10, step_timer, hint_timer, brush_size = 1;

    if (!tex) tex = cells_to_texture(cells);
    if (!zoom) zoom = cells->w/64.0;
    
    fl texture_scale;
    Vector2 texture_pos;

    texture_scale = GetScreenHeight() / (fl)cells->h * zoom;
    texture_pos = (Vector2) {
        GetScreenWidth()/2.0 - (fl)cells->w * texture_scale/2 + pan_x * texture_scale,
        GetScreenHeight()/2.0 - (fl)cells->h * texture_scale/2 + pan_y * texture_scale
    };

    if (GetMouseWheelMove()) {
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            frames_per_step = min(max(1, frames_per_step - GetMouseWheelMove()), 20);
            snprintf(hint_text, 32, "sim speed: %3.1f", 10.0/frames_per_step);
            hint_timer = HINT_TIME;
        } else if (IsKeyDown(KEY_LEFT_SHIFT)) {
            brush_size = min(max(1, brush_size + GetMouseWheelMove()), 10);
            snprintf(hint_text, 32, "brush size: %d", brush_size);
            hint_timer = HINT_TIME;
        } else {
            zoom = min(max(0.5, zoom + GetMouseWheelMove() * ZOOM_RATE * zoom), 100);
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
        (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_LEFT_ALT))
    ) {
        Vector2 delta = GetMouseDelta();
        pan_x += delta.x / texture_scale, pan_y += delta.y / texture_scale;
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        set_cells(cells, true, brush_size, screen_to_cell(GetMouseX(), GetMouseY()));
        tex = cells_to_texture(cells);
    } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        set_cells(cells, false, brush_size, screen_to_cell(GetMouseX(), GetMouseY()));
        tex = cells_to_texture(cells);
    }
    
    if (IsKeyPressed(KEY_SPACE)) playing = !playing;
    if (IsKeyPressed(KEY_C)) clear(cells), update();

    if (playing) {
        if (step_timer) step_timer--;
        else step_timer = frames_per_step, update();
    } else if (IsKeyPressed(KEY_TAB)) update();

    BeginDrawing();
    ClearBackground(LIGHTGRAY);
    DrawTextureEx(*tex, texture_pos, 0, texture_scale, WHITE);
    if (hint_timer)
        DrawText(hint_text, 10, 10, 40, BLACK), hint_timer--;
    EndDrawing();

}

#define USAGE \
    "usage: %s [-h|--help] | [[width] [height]]\n"
#define USAGE_EXT\
    " ** width and height are rounded up to the nearest 8\n"\
    "KEYBINDS:\n"\
    " left mouse    - spawn cells\n"\
    " right mouse   - kill cells\n"\
    " alt+left mouse/\n"\
    "  middle mouse - pan\n"\
    " scroll        - zoom\n"\
    " space         - toggle sim\n"\
    " tab           - step sim\n"\
    " C             - kill all cells\n"\
    " lshift+scroll - change brush size\n"\
    " lctrl+scroll  - change sim speed\n"\

#define USAGE_FMT argv[0]

int main(int argc, char * argv[]) {
    int width = 500, height = 500;

    if (argc > 1) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
            return printf(USAGE USAGE_EXT, USAGE_FMT), 0;
        else width = atoi(argv[1]), height = (argc > 2)? atoi(argv[2]) : width;
    }
    if (width <= 0 || height <= 0)
        return printf(USAGE, USAGE_FMT), 0;
    else width = (width+7)/8 * 8, height = (height+7)/8 * 8;

    printf("starting with board size %dx%d\n", width, height);
 
    cells_t * cells = cells_alloc(width, height);

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(1400, 1400, "gol");
    SetTargetFPS(60);

    while (!WindowShouldClose()) loop(cells);
 
    CloseWindow();
}
