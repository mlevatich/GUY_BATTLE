#include "global_utils.h"
#include "sprite.h"

// Struct for sprite meta information
typedef struct sprite_metainfo {
    short width;
    short height;
    short sheet_position;
    char* frame_sections;
    char type;
    char id;
}* SpriteInfo;

// Struct for spell meta information
typedef struct spell_metainfo {
    char cast_time;
    char finish_time;
    short cooldown;
}* SpellInfo;

// Struct for a currently active sprite
struct sprite {
    
    // Meta info
    SpriteInfo meta;
    
    // Positional info
    double x_pos;
    double y_pos;
    double x_vel;
    double y_vel;
    bool direction_facing;
    short angle;
    
    // Action info
    char hp;
    char colliding;
    char casting;
    short* cooldowns;
    char spell;
    char action;
    bool action_change;
    double frame;
};

// Struct for a linked list of sprites
typedef struct ele {
    Sprite sp;
    struct ele* next;
}* SpriteList;

// Possible action states for a sprite
enum action_types
{ MOVE, COLLIDE, IDLE, JUMP, CAST_FIREBALL, CAST_ICESHOCK };

// Sprite types
enum types
{ HUMANOID, PARTICLE, SPELL };

int numSprites = 4;         // Number of distinct sprites in the game
int numSpells = 2;          // Number of distinct spells in the game

SDL_Texture* spriteSheet;   // Texture containing all sprites
SpriteList activeSprites;   // Linked list of currently active sprites
SpriteInfo* sprite_info;    // List of meta info structs for sprites
SpellInfo* spell_info;      // List of meta info structs for spells

// Assign meta info fields for a spell
static SpellInfo initSpell(char cast_time, char finish_time, short cooldown)
{
    SpellInfo this_spell = (SpellInfo) malloc(sizeof(struct spell_metainfo));
    this_spell->cast_time = cast_time;
    this_spell->finish_time = finish_time;
    this_spell->cooldown = cooldown;
    return this_spell;
}

// Assign meta info fields for a sprite
static SpriteInfo initSprite(char id, char type, short w, short h,  short sheet_pos, char* frame_sections)
{
    SpriteInfo this_sprite = (SpriteInfo) malloc(sizeof(struct sprite_metainfo));
    this_sprite->id = id;
    this_sprite->type = type;
    this_sprite->width = w;
    this_sprite->height = h;
    this_sprite->sheet_position = sheet_pos;
    this_sprite->frame_sections = frame_sections;
    return this_sprite;
}

// Fill the meta info lists with complete meta info for each sprite in the game
void loadSpriteInfo()
{
    // Make space for meta info structs
    sprite_info = (SpriteInfo*)malloc(sizeof(SpriteInfo)*numSprites);
    spell_info =(SpellInfo*)malloc(sizeof(SpellInfo)*numSpells);
    
    // Initialize meta info for sprites
    char* f1=(char*)malloc(sizeof(char)*3); memcpy(f1, (char[]){0, 2, 5}, 3);
    sprite_info[FIREBALL] = initSprite(FIREBALL, SPELL, 23, 10, 60, f1);
    
    char* f2=(char*)malloc(sizeof(char)*3); memcpy(f2, (char[]){0, 2, 5}, 3);
    sprite_info[ICESHOCK] = initSprite(ICESHOCK, SPELL, 23, 10, 70, f2);
    
    char* f3=(char*)malloc(sizeof(char)*3); memcpy(f3, (char[]){0, 2, 2}, 3);
    sprite_info[ICESHOCK_PARTICLE] = initSprite(ICESHOCK_PARTICLE, PARTICLE, 5, 5, 80, f3);
    
    char* f4=(char*)malloc(sizeof(char)*7); memcpy(f4, (char[]){0, 4, 5, 10, 14, 22, 30}, 7);
    sprite_info[GUY] = initSprite(GUY, HUMANOID, 28, 60, 0, f4);
    
    // Initialize meta info for spells
    spell_info[FIREBALL] = initSpell(32, 8, 120);
    spell_info[ICESHOCK] = initSpell(32, 8, 300);
}

// Get x coordinate of a sprite's center
static double x_center(Sprite sp)
{
    return (sp->x_pos + (double)sp->meta->width/2);
}

// Get y coordinate of a sprite's center
static double y_center(Sprite sp)
{
    return (sp->y_pos + (double)sp->meta->height/2);
}

// Return true if a sprite is touching the ground
static bool touching_ground(Sprite sp, int* platforms)
{
    int middle = x_center(sp);
    return (sp->y_pos + sp->meta->height >= platforms[1]) && (middle > platforms[2] && middle < platforms[3]);
}

// Return -1 unless sprite is touching a wall
static int touching_wall(Sprite sp, int* walls)
{
    int numWalls = walls[0];
    for(int i = 1; i < numWalls*3 + 1; i += 3) {
        // AABB check - if it passes, there's a wall collision
        if(walls[i] < sp->x_pos + sp->meta->width && walls[i] > sp->x_pos &&
           walls[i+1] < sp->y_pos + sp->meta->height && walls[i+2] > sp->y_pos) {
            if(fabs(walls[i] - sp->x_pos) < fabs(walls[i] - (sp->x_pos + sp->meta->width))) {
                return walls[i];
            }
            else {
                return walls[i] - sp->meta->width;
            }
        }
    }
    return -1;
}

// Render a sprite from the sprite sheet to the screen
static void renderSprite(Sprite sp)
{
    SDL_RendererFlip flipType = SDL_FLIP_NONE;
    if (sp->direction_facing == LEFT) {
        flipType = SDL_FLIP_HORIZONTAL;
    }
    SDL_Rect clip = {sp->meta->width*(int)sp->frame, sp->meta->sheet_position, sp->meta->width, sp->meta->height};
    SDL_Rect renderQuad = {(int)sp->x_pos, (int)sp->y_pos, sp->meta->width, sp->meta->height};
    SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, flipType);
    
    if(debug) {
        SDL_Rect origin_clip = {78, 65, 2, 2};
        SDL_Rect origin_renderQuad = {(int)sp->x_pos, (int)sp->y_pos, 2, 2};
        SDL_RenderCopyEx(renderer, spriteSheet, &origin_clip, &origin_renderQuad, 0, NULL, SDL_FLIP_NONE);
        
        SDL_Rect center_clip = origin_clip;
        SDL_Rect center_renderQuad = {(int)x_center(sp), (int)y_center(sp), 2, 2};
        SDL_RenderCopyEx(renderer, spriteSheet, &center_clip, &center_renderQuad, 0, NULL, SDL_FLIP_NONE);
    }
}

// Free a sprite
static void freeSprite(struct ele* e)
{
    if(e->sp->meta->type == HUMANOID) {
        free(e->sp->cooldowns);
    }
    free(e->sp);
    free(e);
}

// Detect and handle collisions in this frame for a sprite
static void detectCollision(Sprite sp, int* platforms, int* walls)
{
    if(!sp->colliding) {
        if(sp->meta->type == HUMANOID) {
            // sp->colliding = 13;
            // sp->x_vel = -3;
            // sp->y_vel = -2;
        }
        if((sp->meta->type == SPELL || sp->meta->type == PARTICLE) && (touching_ground(sp, platforms) || touching_wall(sp, walls) != -1)) {
            if(sp->meta->type == PARTICLE) {
                sp->colliding = 2;
            }
            else {
                sp->colliding = 20;
                sp->x_vel *= 0.05;
                sp->y_vel *= 0.05;
            }
        }
    }
}

// Change action state for a sprite
static void changeAction(Sprite sp)
{
    if(sp->colliding) {
        sp->action_change = (sp->action != COLLIDE);
        sp->action = COLLIDE;
    }
    else if(sp->casting) {
        if(sp->spell == FIREBALL) {
            sp->action_change = (sp->action != CAST_FIREBALL);
            sp->action = CAST_FIREBALL;
        }
        else if(sp->spell == ICESHOCK) {
            sp->action_change = (sp->action != CAST_ICESHOCK);
            sp->action = CAST_ICESHOCK;
        }
    }
    else if(sp->x_vel == 0 && sp->y_vel == 0) {
        sp->action_change = (sp->action != IDLE);
        sp->action = IDLE;
    }
    else if(sp->meta->type == HUMANOID && sp->y_vel != 0) {
        sp->action_change = (sp->action != JUMP);
        sp->action = JUMP;
    }
    else {
        sp->action_change = (sp->action != MOVE);
        sp->action = MOVE;
    }
}

// Change angle and directon of a sprite
static void changeOrientation(Sprite sp)
{
    if(sp->meta->type != HUMANOID && sp->x_vel < 0) {
        sp->direction_facing = LEFT;
    }
    else if(sp->meta->type != HUMANOID && sp->x_vel > 0) {
        sp->direction_facing = RIGHT;
    }
    if(sp->meta->id == ICESHOCK) {
        sp->angle = (short)((180/(4.0*atan(1.0)))*atan(sp->y_vel/sp->x_vel));
    }
}

// Calculate physics and update state for a sprite
static int moveSprite(Sprite sp, double increment, int* platforms, int* walls)
{
    // Update the sprite's frame
    if(sp->action == JUMP) {
        increment *= 1.5;
    }
    if(sp->action == CAST_FIREBALL || sp->action == CAST_ICESHOCK) {
        increment *= 2.5;
    }
    if(sp->action == COLLIDE) {
        increment *= 1.5;
    }
    if(sp->action_change) {
        sp->frame = sp->meta->frame_sections[(int)sp->action];
        if(sp->action == MOVE && sp->meta->id == GUY) {
            sp->frame++;
        }
    }
    else {
        sp->frame += increment;
    }
    if(sp->frame >= sp->meta->frame_sections[sp->action+1]) {
        sp->frame = sp->meta->frame_sections[(int)sp->action];
    }
    sp->action_change = false;
    
    // Update the sprite's rotation and direction
    changeOrientation(sp);
    
    // Update the sprite's position
    sp->x_pos += sp->x_vel;
    sp->y_pos += sp->y_vel;
    
    // Stop human sprites at any walls
    if(sp->meta->type == HUMANOID) {
        int walltouch = touching_wall(sp, walls);
        if(walltouch != -1) {
            sp->x_vel = 0;
            sp->x_pos = walltouch;
        }
    }
    
    if(sp->meta->type == HUMANOID) {
        // Update x velocity due to friction / air resistance
        if(fabsf((float)sp->x_vel) < 0.0000001) {
            sp->x_vel = 0;
        }
        else {
            sp->x_vel += convert(sp->x_vel < 0.0f)*0.1;
        }
        
        // Update y velocity due to gravity (terminal velocity of 50)
        sp->y_vel = min(sp->y_vel + 0.5, 50);
        
        // Stop sprite at any platforms
        int numPlatforms = platforms[0];
        int middle = x_center(sp);
        for(int i = 1; i < numPlatforms*3 + 1; i += 3) {
            // Platform land check
            if(sp->y_vel >= 0 && fabs(platforms[i] - (sp->y_pos + sp->meta->height)) <= fabs(sp->y_vel) &&
               middle > platforms[i+1] && middle < platforms[i+2]) {
                sp->y_vel = 0;
                sp->y_pos = platforms[i] - sp->meta->height;
                break;
            }
        }
    }
    else if(sp->meta->id == FIREBALL && !sp->colliding) {
        // Fireball accelerates over time, isn't affected by gravity
        sp->x_vel += convert(sp->x_vel > 0.0f)*0.1;
    }
    else if((sp->meta->id == ICESHOCK || sp->meta->id == ICESHOCK_PARTICLE) && !sp->colliding) {
        // Iceshock is affected slightly by gravity and air resistance
        sp->y_vel += 0.2;
        sp->x_vel += convert(sp->x_vel < 0.0f) * 0.03;
    }
    return 0;
}

// Checks if an active sprite is dead and needs to be unloaded
static bool isDead(Sprite sp)
{
    // If a sprite is too far off screen, unload it
    if(sp->x_pos < -500 || sp->x_pos > SCREEN_WIDTH+500 || sp->y_pos <= -500 || sp->y_pos >= SCREEN_HEIGHT+100) {
        return 1;
    }
    
    // If a spell has finished its collision animation, unlaod it
    if((sp->meta->type == SPELL || sp->meta->type == PARTICLE) && sp->colliding == 1) {
        return 1;
    }
    return 0;
}

// Unload any active sprites which have died
void unloadSprites()
{
    struct ele* prev = NULL;
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        if(isDead(cursor->sp)) {
            if(prev == NULL) {
                prev = cursor;
                cursor = cursor->next;
                freeSprite(prev);
                activeSprites = cursor;
                prev = NULL;
            }
            else {
                struct ele* e = cursor;
                cursor = cursor->next;
                freeSprite(e);
                prev->next = cursor;
            }
        }
        else {
            prev = cursor;
            cursor = cursor->next;
        }
    }
}

// Initialize a sprite with its on-screen location and stats
Sprite spawnSprite(char id, char hp, double x, double y, double x_vel, double y_vel, int direction)
{
    Sprite sp = (Sprite) malloc(sizeof(struct sprite));
    sp->meta = sprite_info[(int)id]; sp->hp = hp;
    sp->x_pos = x;      sp->y_pos = y;
    sp->x_vel = x_vel;  sp->y_vel = y_vel;
    sp->casting = 0;    sp->spell = 0;
    sp->frame = 0;      sp->colliding = 0;
    sp->action = MOVE;  sp->angle = 0;
    sp->direction_facing = direction;
    if(sp->meta->type == HUMANOID) {
        sp->cooldowns = (short*) calloc(numSpells, sizeof(short));
    }
    else {
        sp->cooldowns = NULL;
    }
    changeOrientation(sp);
    struct ele* new_sprite = (struct ele*) malloc(sizeof(struct ele));
    new_sprite->sp = sp;
    new_sprite->next = NULL;
    if(activeSprites != NULL) {
        new_sprite->next = activeSprites;
    }
    activeSprites = new_sprite;
    return sp;
}

// Attempt to cast a spell after a keyboard input
bool cast_spell(Sprite guy, int spell)
{
    if(!(guy->casting || guy->colliding) && !guy->cooldowns[spell] && guy->action != JUMP) {
        guy->casting = spell_info[spell]->cast_time;
        guy->spell = spell;
        return 1;
    }
    return 0;
}

// Attempt to walk in a direction after a keyboard input
bool walk(Sprite guy, int direction)
{
    if(!(guy->casting || guy->colliding)) {
        if(direction == LEFT) {
            guy->x_vel = max(guy->x_vel - 0.3, -3.0);
        }
        else {
            guy->x_vel = min(guy->x_vel + 0.3, 3.0);
        }
        guy->direction_facing = direction;
        return 1;
    }
    return 0;
}

// Attempt to jump after a keyboard input
bool jump(Sprite guy)
{
    if(!(guy->casting || guy->colliding) && guy->action != JUMP) {
        guy->y_vel += -10.1;
        return 1;
    }
    return 0;
}

// Check if its time to spawn new spells, and spawn them
void launchSpells(Sprite guy, int* score)
{
    if(!guy) return;
    if(guy->spell == FIREBALL && guy->casting == spell_info[FIREBALL]->finish_time) {
        char* fire_fs = (char*)malloc(sizeof(char)*3);
        fire_fs[0] = 0; fire_fs[1] = 2; fire_fs[2] = 5;
        double fire_x_pos = guy->x_pos + convert(guy->direction_facing) * 20;
        double fire_x_vel = convert(guy->direction_facing) * 0.7;
        spawnSprite(FIREBALL, 1, fire_x_pos, guy->y_pos+28, fire_x_vel, 0, RIGHT);
        guy->cooldowns[FIREBALL] = spell_info[FIREBALL]->cooldown;
        if(score) *score += 5;
    }
    if(guy->spell == ICESHOCK && guy->casting == spell_info[ICESHOCK]->finish_time) {
        for(double i = 0; i < 4; i++) {
            if(i == 0 || i == 3) {
                int x_mult = convert(i > 2);
                double ice_xpos = (x_mult*15)+guy->x_pos+guy->meta->width/4-3;
                double ice_ypos = guy->y_pos-5;
                spawnSprite(ICESHOCK, 1, ice_xpos, ice_ypos, x_mult*4, -5, RIGHT);
                for(int j = 0; j < 4; j++) {
                    spawnSprite(ICESHOCK_PARTICLE, 1, ice_xpos + (get_rand() - 0.5) * 10, ice_ypos + (get_rand() - 0.5) * 10,
                               x_mult * 4 * get_rand() + 2 * x_mult, -5 * get_rand() - 2.5, RIGHT);
                }
            }
            else {
                int x_mult = convert(i > 1.5);
                double ice_xpos = (x_mult*5)+guy->x_pos+guy->meta->width/4-3;
                double ice_ypos = guy->y_pos-15;
                spawnSprite(ICESHOCK, 1, ice_xpos, ice_ypos, x_mult*2, -5, RIGHT);
                for(int j = 0; j < 4; j++) {
                    spawnSprite(ICESHOCK_PARTICLE, 1, ice_xpos + (get_rand() - 0.5) * 10, ice_ypos + (get_rand() - 0.5) * 10,
                               x_mult * 2 * get_rand() + 1 * x_mult, -5 * get_rand() - 2.5, RIGHT);
                }
            }
        }
        guy->cooldowns[ICESHOCK] = spell_info[ICESHOCK]->cooldown;
        if(score) *score += 10;
    }
}

// Advance cooldown and casting timers
void advanceGuyTimers(Sprite guy)
{
    if(!guy) return;
    if(guy->casting) {
        guy->casting--;
    }
    for(int i = 0; i < numSpells; i++) {
        if(guy->cooldowns[i]) {
            guy->cooldowns[i]--;
        }
    }
}

// Advance collision timers
void advanceCollisions()
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        if(cursor->sp->colliding) {
            cursor->sp->colliding--;
        }
        cursor = cursor->next;
    }
}

// Teleport a sprite to a different location
void teleportSprite(Sprite sp, double x, double y)
{
    if(sp->meta->id == GUY) sp->direction_facing = x < SCREEN_WIDTH / 2;
    sp->x_pos = x;
    sp->y_pos = y;
}

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(Sprite guy)
{
    double* cooldown_percentages = (double*) malloc(sizeof(double) * numSpells);
    if(!guy) return cooldown_percentages;
    for(int i = 0; i < numSpells; i++) {
        cooldown_percentages[i] = (double) guy->cooldowns[i] / (double) spell_info[i]->cooldown;
    }
    return cooldown_percentages;
}

// Get a guy's health remaining
int getHealth(Sprite guy)
{
    if(!guy) return 0;
    return guy->hp;
}

// Reset the health and cooldowns of the guys after a match ends
void resetGuys(Sprite guy, Sprite guy2)
{
    guy->hp = 100;
    guy2->hp = 100;
    for(int i = 0; i < numSpells; i++) {
        guy->cooldowns[i] = 0;
        guy2->cooldowns[i] = 0;
    }
}

// Render all active sprites to the screen
void renderSprite_all()
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        renderSprite(cursor->sp);
        cursor = cursor->next;
    }
}

// Check collisions for all active sprites
void detectCollision_all(int* platforms, int* walls)
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        detectCollision(cursor->sp, platforms, walls);
        cursor = cursor->next;
    }
}

// Change the action state for all active sprites
void changeAction_all()
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        changeAction(cursor->sp);
        cursor = cursor->next;
    }
}

// Move all active sprites
void moveSprite_all(int* platforms, int* walls)
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        moveSprite(cursor->sp, FRAME_INCREMENT, platforms, walls);
        cursor = cursor->next;
    }
}

// Free all sprites and destroy linked list
void freeSprite_all()
{
    struct ele* cursor = activeSprites;
    while(cursor != NULL) {
        struct ele* e = cursor;
        cursor = cursor->next;
        freeSprite(e);
    }
}

// Free sprite and spell meta info
void freeMetaInfo()
{
    // Free sprite metainfo
    for(int i = 0; i < numSprites; i++) {
        free(sprite_info[i]->frame_sections);
        free(sprite_info[i]);
    }
    free(sprite_info);
    
    // Free spell metainfo
    for(int i = 0; i < numSpells; i++) {
        free(spell_info[i]);
    }
    free(spell_info);
}

// Load the sprite sheet texture into memory
void loadSpriteSheet()
{
    spriteSheet = loadTexture("Art/Spritesheet.bmp");
}

// Free the sprite sheet texture from memory
void freeSpriteSheet()
{
    SDL_DestroyTexture(spriteSheet);
}
