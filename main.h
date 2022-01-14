/*
 * * Flicky's Flocky
 ** 12 player Flappy Bird clone for the Sega Saturn
 ** by Slinga
 ** https://github.com/slinga-homebrew/Flickys-Flock/
 ** MIT License
 */

/*
 * * Jo Sega Saturn Engine
 ** Copyright (c) 2012-2017, Johannes Fetz (johannesfetz@gmail.com)
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **     * Redistributions of source code must retain the above copyright
 **       notice, this list of conditions and the following disclaimer.
 **     * Redistributions in binary form must reproduce the above copyright
 **       notice, this list of conditions and the following disclaimer in the
 **       documentation and/or other materials provided with the distribution.
 **     * Neither the name of the Johannes Fetz nor the
 **       names of its contributors may be used to endorse or promote products
 **       derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 ** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
 ** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 ** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 ** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jo/jo.h>
#include "STRING.H"

#define VERSION             "v1.0.1"

#define MAX_PLAYERS         12
#define MAX_FLICKY_SPRITES  12
#define MAX_PIPES           6
#define MAX_POWER_UPS       3

#define GAMESTATE_UNINITIALIZED 0
#define GAMESTATE_SSMTF_LOGO    1
#define GAMESTATE_TITLE_SCREEN  2
#define GAMESTATE_GAMEPLAY      3
#define GAMESTATE_PAUSED        4
#define GAMESTATE_GAME_OVER     5
#define GAMESTATE_VICTORY       6

#define FLICKYSTATE_UNINITIALIZED 0
#define FLICKYSTATE_FLYING        1
#define FLICKYSTATE_DYING         2
#define FLICKYSTATE_DEAD          3

#define PIPESTATE_UNINITIALIZED   0
#define PIPESTATE_INITIALIZED     1

#define POWERUP_UNINITIALIZED     0
#define POWERUP_INITIALIZED       1

// top left is negative
// bottom right is positive
// center is 0,0
#define SCREEN_TOP                -120
#define SCREEN_BOTTOM             51
#define SCREEN_LEFT               -160
#define SCREEN_RIGHT              160
#define GROUND_COLLISION          50

#define FLOOR_POSITION_X            -240
#define FLOOR_POSITION_Y            92
#define FLOOR_WIDTH                 240
#define TITLE_SCREEN_OPTIONS_Y      79
#define GAME_OVER_LEFT_SPRITE_POS   -110
#define GAME_OVER_RIGHT_SPRITE_POS  30

// various frame timers
#define SSMTF_LOGO_TIMER          380
#define FLICKY_FLAPPING_SPEED     15
#define SPAWN_FRAME_TIMER         150
#define ALL_DEAD_FRAME_TIMER      150
#define FLICKY_DEATH_FRAME_TIMER  90
#define DEATH_FRAME_TIMER_FLYING  60
#define BLOCK_INPUT_FRAMES        60
#define REVERSE_GRAVITY_TIMER     600 // how long for each power-up to last
#define LIGHTNING_TIMER           600
#define STONE_SNEAKERS_TIMER      600

// power-ups
#define POWERUP_ONE_UP             0
#define POWERUP_REVERSE_GRAVITY    1
#define POWERUP_LIGHTNING          2
#define POWERUP_ROBOTNIK           3
#define POWERUP_STONE_SNEAKERS     4
#define NUM_POWER_UPS              5

// gameplay constants
#define FALLING_CONSTANT          1
#define FLAP_Y_SPEED              -15
#define MAX_Y_SPEED               20  // don't fall faster than this
#define MAX_DEATH_HEIGHT          -84 // max height to raise the dead Flicky sprite

// CD sound tracks
#define TITLE_TRACK               2
#define LIFE_UP_TRACK             3
#define GAMEPLAY_TRACK            4
#define GAMEOVER_TRACK            5

// utility definitions
#define LWRAM 0x00200000 // start of LWRAM memory. Doesn't appear to be used
#define LWRAM_HEAP_SIZE 0x40000 // number of bytes to extend heap by
#define COUNTOF(x) sizeof(x)/sizeof(*x)

// misc
#define VICTORY_CONDITION         100
#define NUM_DIGITS                10
#define NUM_TABLE_LETTERS         5
#define NUM_FLICKY_SPRITES        MAX_FLICKY_SPRITES*3

//
// structure definitions
//

// records whether or not an input has been pressed that frame
typedef struct _INPUTCACHE
{
    bool pressedUp;
    bool pressedDown;
    bool pressedLeft;
    bool pressedRight;
    bool pressedABC;
    bool pressedZ;
    bool pressedStart;
    bool pressedLT;
    bool pressedRT;
} INPUTCACHE, *PINPUTCACHE;

// the player(s)
typedef struct _FLICKY
{
    int playerID; // index into the array of players
    int spriteID; // index into the array of Flicky sprites. Changeable
    int state;

    int numPoints;
    int numDeaths;
    int totalScore; // numPoints - numDeaths

    int x_pos;
    int y_pos;
    int z_pos;
    int y_speed; // falling\climbing speed
    int angle; // angle the sprite is rotated

    // controller input
    INPUTCACHE input;

    bool flapping;
    bool hasFlapped; // if the player has flapped yet,
                     // used to protect the player on spawn

    // various timers
    int frameTimer;           // frame timer used on a per player basis
    int spawnFrameTimer;      // how long the player has been alive for
                              // needed for spawn invulnerability
    int reverseGravityTimer;  // how many frames to swap gravity for the palyer
    int lightningTimer;       // how many frames to shrink player
    int stoneSneakersTimer;   // how many frames to decrease player's jumping

} FLICKY, *PFLICKY;

// Each Flicky has three sprites
typedef struct _FLICKY_SPRITES
{
    unsigned int death; // death sprite
    unsigned int up;   // wings up sprite
    unsigned int down; // wings down sprite
} FLICKY_SPRITES, *PFLICKY_SPRITES;

// the pipe
typedef struct _PIPE
{
    int state;
    int x_pos;
    int y_pos;
    int z_pos;
    int top_y_pos;   // y position of the top half of the pipe
    int gap;         // how large the gap between top and bottom is
    int numSections; // number of times to repeat the sprites
} PIPE, *PPIPE;

// the power-up
typedef struct _powerup
{
    int state;
    int type; // one of the POWERUP #def
    int x_pos;
    int y_pos;
    int z_pos;
} POWERUP, *PPOWERUP;

typedef struct _GAME
{
    // game state variables
    int gameState;
    int frame;
    int frameDeathTimer;      // how many frames since all players were dead
    int frameBlockInputTimer; // how many frames input has been blocked for
    int floorPosition_x;

    // random Flicky to use for menus
    int randomFlicky;
    bool titleFlapping;

    // title screen choices
    int titleScreenChoice;
    int numLivesChoice;
    int startingPositionChoice;

    int topScore;

    // hack to cache controller inputs
    INPUTCACHE input;

} GAME, *PGAME;

// holds sprite and audio assets
typedef struct _assets
{
    // SSMTF logo sprites
    int SSMTF1Sprite;
    int SSMTF2Sprite;
    int SSMTF3Sprite;
    int SSMTF4Sprite;

    // title screen sprites
    int titleSprite;
    int startSprite;
    int livesSprite;
    int livesInfSprite;
    int lives1Sprite;
    int lives3Sprite;
    int lives5Sprite;
    int lives9Sprite;
    int positionSprite;
    int randomSprite;
    int fixedSprite;

    // gameplay sprites
    int pipeSprite;
    int pipeTopSprite;
    int floorSprite;

    // game over/pause sprites
    int largeDigitSprites[NUM_DIGITS];
    int smallDigitSprites[NUM_DIGITS];
    int seperatorHorizontalSprite;
    int seperatorVerticalSprite;
    int gameOverSprite;
    int pauseSprite;
    int tableCSprite;
    int tableDSprite;
    int tablePSprite;
    int tableRSprite;
    int tableSSprite;
    int retrySprite;
    int exitSprite;
    int continueSprite;

    // power-ups
    int powerUpSprites[NUM_POWER_UPS];

    // background
    jo_img background;

    // audio assets
    jo_sound deathPCM;
    jo_sound reverseGravityPCM;
    jo_sound lightningPCM;
    jo_sound stoneSneakersPCM;
} ASSETS, *PASSETS;

//
// function prototypes
//

// callbacks
void debugInfo(void);
void abcStart_gamepad(void);
void ssmtfScreen_draw(void);
void titleScreen_draw(void);
void titleScreen_input(void);
void gameplay_draw(void);
void gameplay_input(void);
void gameplay_checkForCollisions(void);
void gameOver_draw(void);
void gameOver_input(void);

// loading resources
void loadSpriteAssets(void);
void loadPCMAssets(void);
void setBackground(void);

// transitioning between states
void transitionToTitleScreen(void);
void transitionToGameplay(bool resetPlayers);
void transitionToGameOverOrPause(bool pause);

// Flicky functions
void initPlayers(void);
void spawnPlayer(int playerID, bool deductLife);
void killPlayer(int playerID);
bool areAllPlayersDead(void);
int calculateFlickyAngle(int speed);
int getNextFlickySprite(int spriteID, int offset);
void getStartingPosition(int playerID, int* x_pos, int* y_pos, int* z_pos);

// pipes, floor
void initPipe(PPIPE pipe);
int getNextPipePosition(void);
int getNumberofPipes(void);
void drawFloor(void);

// powerups
void initPowerUp(PPOWERUP powerup);
int getNextPowerUpPosition(void);
void applyPowerUp(PFLICKY player, PPOWERUP powerup);
void extraLifePlayer(int playerID);

// collisions
bool checkForFlickyPowerUpCollisions(PFLICKY player, PPOWERUP powerup);
bool checkForFlickyPipeCollisions(PFLICKY player, PPIPE pipe, bool top);
bool checkForPowerUpPipeCollisions(PPOWERUP powerup, PPIPE pipe, bool top);

// scoring
void clearScores(void);
int getTopScore(void);
void sortPlayersByScore(PFLICKY players);
void validateScores(void);
int getDifficulty(void);
void adjustDifficulty(void);

// misc
void shuffleArray(unsigned int* array, unsigned int size);
