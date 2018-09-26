/*
 Interface control
 
 The interface includes toolbar elements (health bar, cooldown meters, logo, etc) and
 text elements (selectable menu options and non-selectable text displayed to the screen).
 */

// List of game states
enum modes
{ OPENING, TITLE, CONTROLS, STAGE_SELECT, VS, AI, PAUSE, GAME_OVER };

// Load the toolbar texture, toolbar elements, and selection text into memory
void loadToolbar(void);

// Free the toolbar elements, toolbar texture, and selection text from memory
void freeToolbar(void);

// Render all of the current mode's toolbar and text elements to the screen
void renderInterface(int mode, long long frame, int guy_hp, int guy2_hp, double* guy_cds, double* guy2_cds, int score);

// Move the text selection arrow
int hover(char mode, int direction);
