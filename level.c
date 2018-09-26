#include "global_utils.h"
#include "level.h"

// Struct for background information
typedef struct background {
    
    // Meta info about the background
    SDL_Texture* image;
    int width;
    int height;
    int x_init;
    int y_init;
    double xv_init;
    double yv_init;
    char drift_type;
    
    // State info about the background
    double x;
    double y;
    double x_vel;
    double y_vel;
    
}* Background;

// Struct for foreground information
typedef struct foreground {
    SDL_Texture* image;
    int* platforms;
    int* walls;
    int* starting_positions;
}* Foreground;

// Types of background behavior
enum drift_types
{ SCROLL, DRIFT };

// All relevant information about loaded backgrounds and foregrounds
Background* backgrounds = NULL;
Foreground* foregrounds = NULL;
int numBackgrounds = 2;
int numForegrounds = 2;
int currentBackground = FOREST;
int currentForeground = FOREST;

// Assign background fields
static Background initBackground(const char* path, char drift_type, int width, int height, int x, int y, double x_vel, double y_vel)
{
    Background this_background = (Background) malloc(sizeof(struct background));
    this_background->image = loadTexture(path);
    this_background->width = width;     this_background->height = height;
    this_background->x_init = x;        this_background->y_init = y;
    this_background->x = x;             this_background->y = y;
    this_background->xv_init = x_vel;   this_background->yv_init = y_vel;
    this_background->x_vel = x_vel;     this_background->y_vel = y_vel;
    this_background->drift_type = drift_type;
    if(drift_type == SCROLL) {
        this_background->y_vel = 0;
        this_background->yv_init = 0;
    }
    return this_background;
}

// Assign foreground fields
static Foreground initForeground(const char* path, int* platforms, int* walls, int* starts)
{
    Foreground this_foreground = (Foreground) malloc(sizeof(struct foreground));
    this_foreground->image = loadTexture(path);
    this_foreground->platforms = platforms;
    this_foreground->walls = walls;
    this_foreground->starting_positions = starts;
    return this_foreground;
}

// Load all backgrounds and foregrounds into memory
void loadLevels()
{
    // Make space for backgrounds and foregrounds
    backgrounds = (Background*) malloc(numBackgrounds * sizeof(Background));
    foregrounds = (Foreground*) malloc(numForegrounds * sizeof(Foreground));
    
    // Initialize backgrounds
    backgrounds[FOREST] = initBackground("Art/forest_background.bmp", SCROLL, 1400, SCREEN_HEIGHT, 0, 0, 0.1, 0);
    backgrounds[VOLCANO] = initBackground("Art/volcano_background.bmp", DRIFT, 1400, 1000, 200, 150, 0.1, -0.1);
    
    // Initialize foregrounds
    int numPlatforms; int numWalls;
    
    // Forest (in game, Cultist Clearing)
    numPlatforms = 5; numWalls = 2;
    int* forest_platforms = (int*) malloc(sizeof(int) * (numPlatforms*3 + 1));
    memcpy(forest_platforms, (int[]){numPlatforms, 660, 0, 1024, 250, 60, 154, 575, 300, 724, 250, 870, 964, 100, 1024, 1124},
           sizeof(int)*(numPlatforms*3 + 1));
    int* forest_walls = (int*) malloc(sizeof(int) * (numWalls*3 + 1));
    memcpy(forest_walls, (int[]){numWalls, 60, 0, SCREEN_HEIGHT, 964, 0, SCREEN_HEIGHT}, sizeof(int)*(numWalls*3 + 1));
    int* forest_starts = (int*) malloc(sizeof(int) * 4);
    memcpy(forest_starts, (int[]){100, 190, 896, 190}, sizeof(int) * 4);
    foregrounds[FOREST] = initForeground("Art/forest_foreground.bmp", forest_platforms, forest_walls, forest_starts);
    
    // Volcano (in game, Phoenix Mountain)
    numPlatforms = 4; numWalls = 0;
    int* volcano_platforms = (int*) malloc(sizeof(int) * (numPlatforms*3 + 1));
    memcpy(volcano_platforms, (int[]){numPlatforms, 452, 200, 824, 352, 224, 324, 352, 700, 800, 100, 1024, 1124},
           sizeof(int)*(numPlatforms*3 + 1));
    int* volcano_walls = (int*) malloc(sizeof(int) * (numWalls*3 + 1));
    memcpy(volcano_walls, (int[]){numWalls}, sizeof(int)*(numWalls*3 + 1));
    int* volcano_starts = (int*) malloc(sizeof(int) * 4);
    memcpy(volcano_starts, (int[]){250, 290, 747, 290}, sizeof(int) * 4);
    foregrounds[VOLCANO] = initForeground("Art/volcano_foreground.bmp", volcano_platforms, volcano_walls, volcano_starts);
}

// Free a background from memory
static void freeBackground(Background bg)
{
    SDL_DestroyTexture(bg->image);
    free(bg);
}

// Free a foreground from memory
static void freeForeground(Foreground fg)
{
    SDL_DestroyTexture(fg->image);
    free(fg->platforms);
    free(fg->walls);
    free(fg->starting_positions);
    free(fg);
}

// Free all backgrounds and foregrounds
void freeLevels()
{
    for(int i = 0; i < numBackgrounds; i++) {
        freeBackground(backgrounds[i]);
    }
    free(backgrounds);
    
    for(int i = 0; i < numForegrounds; i++) {
        freeForeground(foregrounds[i]);
    }
    free(foregrounds);
}

// Render an infinitely scrolling background
static void renderScrollBackground(Background bg)
{
    SDL_Rect quad = {(int)bg->x * -1, (int)bg->y * -1, bg->width, bg->height};
    SDL_RenderCopy(renderer, bg->image, NULL, &quad);
    if(bg->x + SCREEN_WIDTH > bg->width || bg->x < 0) {
        quad.x = (int)bg->x * -1 + convert(bg->x > 0) * bg->width;
        quad.y = (int)bg->y * -1;
        quad.w = bg->width;
        quad.h = bg->height;
        SDL_RenderCopy(renderer, bg->image, NULL, &quad);
    }
    if(bg->x >= bg->width || bg->x <= bg->width * -1) {
        bg->x = 0;
    }
}

// Render a drifting background
static void renderDriftBackground(Background bg)
{
    SDL_Rect quad = {(int) bg->x * -1, (int) bg->y * -1, bg->width, bg->height};
    SDL_RenderCopy(renderer, bg->image, NULL, &quad);
}

// Render the current background
void renderBackground()
{
    if(backgrounds[currentBackground]->drift_type == SCROLL) {
        renderScrollBackground(backgrounds[currentBackground]);
    }
    else {
        renderDriftBackground(backgrounds[currentBackground]);
    }
}

// Render the current foreground
void renderForeground()
{
    SDL_RenderCopy(renderer, foregrounds[currentForeground]->image, NULL, NULL);
}

// Switch the background to a new one
static void switchBackground(int new_background)
{
    currentBackground = new_background;
}

// Switch the foreground to a new one
static void switchForeground(int new_foreground)
{
    currentForeground = new_foreground;
}

// Switch the level to a new one
void switchLevel(int new_level)
{
    switchForeground(new_level);
    switchBackground(new_level);
}

// Animate the background
void moveBackground()
{
    Background bg = backgrounds[currentBackground];
    bg->x += bg->x_vel;
    bg->y += bg->y_vel;
    if(bg->drift_type == DRIFT) {
        int reset_to = 0;
        if(bg->x >= (reset_to = bg->width - SCREEN_WIDTH) || bg->x <= (reset_to = 0)) {
            bg->x_vel *= -1;
            bg->x = reset_to;
        }
        if(bg->y >= (reset_to = bg->height - SCREEN_HEIGHT) || bg->y <= (reset_to = 0)) {
            bg->y_vel *= -1;
            bg->y = reset_to;
        }
    }
}

// Returns the platforms on the current foreground
int* getPlatforms()
{
    return foregrounds[currentForeground]->platforms;
}

// Returns the walls on the current foreground
int* getWalls()
{
    return foregrounds[currentForeground]->walls;
}

// Returns current level
int getLevel()
{
    return currentForeground;
}

int* getStartingPositions(int fg)
{
    return foregrounds[fg]->starting_positions;
}
