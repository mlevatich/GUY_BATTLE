#include "global_utils.h"
#include "sprite.h"
#include "level.h"
#include "interface.h"

bool loadGame()
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    
    // Initialize sound
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return false;
    
    // Create window
    window = SDL_CreateWindow("GUY BATTLE", 20, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) return false;
    
    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) return false;
    
    // Load music
    main_theme = Mix_LoadMUS("Sound/Twilight_of_the_Gods.wav");
    if(!main_theme) return false;
    
    // Load sound effects
    // NONE
    
    // Initialize renderer color and image loading
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    
    // Load sprite sheet
    loadSpriteSheet();
    
    // Load level backgrounds and foregrounds
    loadLevels();
    
    // Load meta information for sprites
    loadSpriteInfo();
    
    // Load interface
    loadToolbar();
    
    return true;
}

void quitGame()
{
    // Free sprite metainfo
    freeMetaInfo();
    
    // Free remaining active sprites
    freeSprite_all();
    
    // Free sprite sheet
    freeSpriteSheet();
    
    // Free backgrounds and foregrounds
    freeLevels();
    
    // Free toolbar
    freeToolbar();
    
    // Free music
    Mix_FreeMusic(main_theme);
    
    // Free sound effects
    // NONE
    
    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = NULL;
    window = NULL;
    
    // Free SDL
    Mix_Quit();
    SDL_Quit();
}

// Helper function to set the level and teleport guys
void setLevel(Sprite guy, Sprite guy2, int level, int mode)
{
    switchLevel(level);
    int* starts = getStartingPositions(level);
    teleportSprite(guy,  starts[0], starts[1]);
    if(mode != AI) teleportSprite(guy2, starts[2], starts[3]);
}

// Helper function to reset the game to title screen
void resetGame(Sprite guy, Sprite guy2, int* mode, int* selection, int* vs_or_ai, int* score)
{
    *mode = TITLE;
    *selection = VS;
    *vs_or_ai = VS;
    *score = 0;
    setLevel(guy, guy2, FOREST, TITLE);
    resetGuys(guy, guy2);
}

int main(int argc, char* args[])
{
    // Load game
    if(!loadGame()) {
	printf("error: %s\n", Mix_GetError());
        fprintf(stderr,"Error: Initialization Failed\n");
        return 1;
    }
    
    // Keep track of what state the game is in
    int mode = OPENING;
    int selection = VS;
    int vs_or_ai = VS;
    long long frame = 0;
    int score = 0;
    int* starts = getStartingPositions(getLevel());
    Sprite guy = NULL;
    Sprite guy2 = NULL;
    
    // Game loop
    bool quit = false;
    SDL_Event e;
    while(!quit) {
        int start_time = SDL_GetTicks();
        
        // Some events occur at specific frames in the opening cutscene
        // End opening and give control to the player after 375 frames
        if(frame == 10) {
            Mix_PlayMusic(main_theme, -1);
        }
        if(frame == 100) {
            guy = spawnSprite(GUY, 100, starts[0], starts[1]-300, 0, 0, RIGHT);
        }
        if(frame == 225) {
            guy2 = spawnSprite(GUY, 100, starts[2], starts[3]-300, 0, 0, LEFT);
        }
        if(frame == 375) {
            mode = TITLE;
        }
        
        // Process key presses which are game state changes / menu selections
        while(SDL_PollEvent(&e) != 0) {
            if(e.type == SDL_QUIT) {
                quit = true;
            }
            else if(e.type == SDL_KEYDOWN) {
                int key = e.key.keysym.sym;
                switch(mode) {
                        
                    case OPENING:
                        break;
                        
                    case TITLE:
                        // Select VS, AI, or controls and hit enter
                        if(key == SDLK_RETURN) {
                            if(selection == CONTROLS) {
                                mode = selection;
                            }
                            else {
                                mode = STAGE_SELECT;
                                vs_or_ai = selection;
                                if(vs_or_ai == AI) teleportSprite(guy2, SCREEN_WIDTH+20, 0);
                            }
                        }
                        else if(key == SDLK_UP) {
                            selection = hover(mode, UP);
                        }
                        else if(key == SDLK_DOWN) {
                            selection = hover(mode, DOWN);
                        }
                        break;
                        
                    case STAGE_SELECT:
                        // Select Volcano or Forest and hit enter, or esc to title
                        if(key == SDLK_RETURN) {
                            mode = vs_or_ai;
                        }
                        else if(key == SDLK_ESCAPE) {
                            resetGame(guy, guy2, &mode, &selection, &vs_or_ai, &score);
                        }
                        else if(key == SDLK_UP) {
                            selection = hover(mode, UP);
                            if(getLevel() != selection) setLevel(guy, guy2, FOREST, vs_or_ai);
                        }
                        else if(key == SDLK_DOWN) {
                            selection = hover(mode, DOWN);
                            if(getLevel() != selection) setLevel(guy, guy2, VOLCANO, vs_or_ai);
                        }
                        break;
                        
                    case VS:
                        if(key == SDLK_LSHIFT) mode = GAME_OVER; // DELETE LATER
                        break;
                        
                    case AI:
                        // Pause
                        if(key == SDLK_ESCAPE) mode = PAUSE;
                        else if(key == SDLK_LSHIFT) mode = GAME_OVER; // DELETE LATER
                        break;
                        
                    case CONTROLS:
                        // Exit the controls menu
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) mode = TITLE;
                        break;
                        
                    case PAUSE:
                        // Unpause
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) mode = AI;
                        break;
                        
                    case GAME_OVER:
                        // Return to title
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) resetGame(guy, guy2, &mode, &selection, &vs_or_ai, &score);
                        break;
                        
                    
                        
                }
            }
        }
        
        // Process key presses as actions in battle
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        bool success = 0;
        switch(mode) {
                
            case VS:
                // Input for guy 1
                success = 0;
                if(!success && keys[SDL_SCANCODE_2])                          success = cast_spell(guy, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_1])                          success = cast_spell(guy, FIREBALL);
                if(!success && keys[SDL_SCANCODE_D])                          success = jump(guy);
                if(!success && keys[SDL_SCANCODE_X] && !keys[SDL_SCANCODE_V]) success = walk(guy, LEFT);
                if(!success && keys[SDL_SCANCODE_V] && !keys[SDL_SCANCODE_X]) success = walk(guy, RIGHT);
                
                // Input for guy 2
                success = 0;
                if(!success && keys[SDL_SCANCODE_U])                                 success = cast_spell(guy2, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_Y])                                 success = cast_spell(guy2, FIREBALL);
                if(!success && keys[SDL_SCANCODE_UP])                                success = jump(guy2);
                if(!success && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) success = walk(guy2, LEFT);
                if(!success && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) success = walk(guy2, RIGHT);
                break;
                
            case AI:
                // Input for guy
                success = 0;
                if(!success && keys[SDL_SCANCODE_2])                                 success = cast_spell(guy, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_1])                                 success = cast_spell(guy, FIREBALL);
                if(!success && keys[SDL_SCANCODE_UP])                                success = jump(guy);
                if(!success && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) success = walk(guy, LEFT);
                if(!success && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) success = walk(guy, RIGHT);
                break;
        }
        
        if(mode != PAUSE) {
            // Process ai decisions
            // no ai yet
            
            // Unlaod dead sprites
            unloadSprites();
            
            // Check for and handle collisions
            detectCollision_all(getPlatforms(), getWalls());
            
            // Change action states based on key input and collisions
            changeAction_all();
            
            // Move sprites based on current actions, velocities, and terrain
            // Update the frames for those sprites
            // Update the velocities for those sprites
            moveSprite_all(getPlatforms(), getWalls());
            
            // Spawn any new sprites
            launchSpells(guy, &score);
            launchSpells(guy2, NULL);
            
            // Move the background
            moveBackground();
        }

        // Render changes to screen
        SDL_RenderClear(renderer);
        renderBackground();
        renderForeground();
        renderSprite_all();
        renderInterface(mode, frame, getHealth(guy), getHealth(guy2), getCooldowns(guy), getCooldowns(guy2), score);
        SDL_RenderPresent(renderer);
        
        if(mode != PAUSE) {
            // Update timers (spell cooldowns, casting times, collision times, etc)
            advanceGuyTimers(guy);
            advanceGuyTimers(guy2);
            advanceCollisions();
        }
        
        // Cap framerate at MAX_FPS
        int sleep_time = MS_PER_FRAME - (SDL_GetTicks() - start_time);
        if(sleep_time > 0) {
            SDL_Delay(sleep_time);
        }
        frame++;
    }
    
    // Free resources and exit game
    quitGame();
    return 0;
}
