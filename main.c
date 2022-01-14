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

#include "main.h"

//
// globals
//

// numLivesChoice indexes into this array for the starting
// number of lives per player
// 0 = infinite lives
const int g_InitialLives[] = {0, 1, 3, 5, 9};

GAME g_Game = {0};
ASSETS g_Assets = {0};
FLICKY_SPRITES g_FlickySprites[MAX_FLICKY_SPRITES] = {0};
FLICKY g_FlickyTitleSprites[MAX_FLICKY_SPRITES] = {0};
FLICKY g_Players[MAX_PLAYERS] = {0};
PIPE g_Pipes[MAX_PIPES] = {0};
POWERUP g_PowerUps[MAX_POWER_UPS] = {0};
unsigned int g_StartingPositions[MAX_PLAYERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void jo_main(void)
{
    g_Game.gameState = GAMESTATE_UNINITIALIZED;

    // extend the heap to use LWRAM as well otherwise our sprites won't fit in
    // main memory
    jo_add_memory_zone((unsigned char *)LWRAM, LWRAM_HEAP_SIZE);

    jo_core_init(JO_COLOR_Black);

    // load assets from the cd
    loadSpriteAssets();
    loadPCMAssets();

    g_Game.randomFlicky = jo_random(MAX_PLAYERS) - 1;
    g_Game.floorPosition_x = FLOOR_POSITION_X;
    g_Game.numLivesChoice = 2; // default to 3 lives

    //
    // callbacks for the various game states
    //

    // ABC + start handler
    jo_core_set_restart_game_callback(abcStart_gamepad);

    // SSMTF logo
    jo_core_add_callback(ssmtfScreen_draw);

    // title screen
    jo_core_add_callback(titleScreen_draw);
    jo_core_add_callback(titleScreen_input);

    // gameplay
    jo_core_add_callback(gameplay_draw);
    jo_core_add_callback(gameplay_input);
    jo_core_add_callback(gameplay_checkForCollisions);

    // game over/pause
    jo_core_add_callback(gameOver_input);
    jo_core_add_callback(gameOver_draw);

    // debug info
    //jo_core_add_callback(debugInfo);

    g_Game.gameState = GAMESTATE_SSMTF_LOGO;

    jo_core_run();
}

//
// callbacks
//

// prints debug info on the screen
void debugInfo(void)
{
    /*
        jo_printf(0, 0, "Sprite usage: %d RAM usage: %d      ", jo_sprite_usage_percent(), jo_memory_usage_percent());

        jo_printf(3, 2, "0 (%d, %d)      ", g_Players[0].x_pos, g_Players[0].y_pos);
        jo_printf(3, 3, "P (%d, %d)      ", g_Pipes[0].x_pos, g_Pipes[0].y_pos);
        jo_printf(3, 4, "y_speed (%d)      ", g_Players[0].y_speed);
        jo_printf(3, 5, "num pipes (%d)    ", getNumberofPipes());
        jo_printf(3, 6, "difficulty (%d)    ", getDifficulty());

        //

    */
    jo_fixed_point_time();
    slPrintFX(delta_time, slLocate(3,4));

    /*
    jo_printf(3, 2, "x: %d y: %d z: %d t: %d    ", g_PowerUps[0].x_pos, g_PowerUps[0].y_pos, g_PowerUps[0].z_pos, g_PowerUps[0].type);
    jo_printf(3, 3, "sg: %d l: %d ss: %d        ", g_Players[0].reverseGravityTimer, g_Players[0].lightningTimer, g_Players[0].stoneSneakersTimer);

    // did the player hit start
    if(jo_is_pad1_key_pressed(JO_KEY_Z))
    {
        if(g_Game.input.pressedStart == false)
        {
            g_Game.input.pressedStart = true;

            applyPowerUp(&g_Players[0], &g_PowerUps[0]);

            return;
        }
    }
    */
}

// exits to title screen if player one presses ABC+Start
void abcStart_gamepad(void)
{
    transitionToTitleScreen();
    return;
}

// draws the Sega Saturn Multiplayer Task Force logo
void ssmtfScreen_draw(void)
{
    if(g_Game.gameState != GAMESTATE_SSMTF_LOGO)
    {
        return;
    }

    // wait a bit before starting the music track
    if(g_Game.frame == 30)
    {
        jo_audio_play_cd_track(LIFE_UP_TRACK, LIFE_UP_TRACK, false);
    }

    g_Game.frame++;

    if(g_Game.frame < SSMTF_LOGO_TIMER - 60)
    {
        // SSMTF text
        jo_sprite_draw3D(g_Assets.SSMTF1Sprite, 0, -42, 500);
        jo_sprite_draw3D(g_Assets.SSMTF2Sprite, 0, -14, 500);
        jo_sprite_draw3D(g_Assets.SSMTF3Sprite, 0, 14, 500);
        jo_sprite_draw3D(g_Assets.SSMTF4Sprite, 0, 42, 500);
    }

    // check if the timer has expired
    if(g_Game.frame > SSMTF_LOGO_TIMER)
    {
        transitionToTitleScreen();
        return;
    }

    return;
}

// randomize the location of the title screen flickies
void randomizeTitleFlicky(PFLICKY flicky)
{
    flicky->x_pos = -500 + jo_random(300);
    flicky->y_pos = 500 - jo_random(250);

    flicky->angle = -8;
    flicky->z_pos = 540;
    flicky->flapping = jo_random(2) - 1;

    flicky->frameTimer = FLICKY_FLAPPING_SPEED + jo_random(FLICKY_FLAPPING_SPEED);
}

// draws the flickys on the title screen
void drawTitleFlickys(void)
{
    unsigned int i = 0;

    for(i = 0; i < COUNTOF(g_FlickyTitleSprites); i++)
    {
        PFLICKY temp = &g_FlickyTitleSprites[i];

        temp->frameTimer--;

        if(temp->frameTimer <= 0)
        {
            temp->flapping = !temp->flapping;
            temp->frameTimer = FLICKY_FLAPPING_SPEED;
        }

        // draw the Flicky
        if(temp->flapping == false)
        {
            jo_sprite_draw3D_and_rotate(g_FlickySprites[i%12].up, temp->x_pos, temp->y_pos, temp->z_pos, temp->angle);
        }
        else
        {
            jo_sprite_draw3D_and_rotate(g_FlickySprites[i%12].down, temp->x_pos, temp->y_pos, temp->z_pos, temp->angle);
        }

        temp->x_pos += 1;
        temp->y_pos -= 1;

        // reset the Flicky if we are off screen
        if(temp->x_pos > 300)
        {
            randomizeTitleFlicky(temp);
        }

        // check if we need to flap
        temp->frameTimer--;
        if(temp->frameTimer < 0)
        {
            temp->flapping = !temp->flapping;
            temp->frameTimer = FLICKY_FLAPPING_SPEED + jo_random(FLICKY_FLAPPING_SPEED);
        }
    }
}

// draws the title screen and menu
void titleScreen_draw(void)
{
    if(g_Game.gameState != GAMESTATE_TITLE_SCREEN)
    {
        return;
    }

    g_Game.frame++;

    // version #
    jo_printf(33, 28, "%s", VERSION);

    // Flicky's Flock title screen
    jo_sprite_draw3D(g_Assets.titleSprite, 0, 0, 500);

    // floor
    drawTitleFlickys();

    // lives, position, and start text
    jo_sprite_draw3D(g_Assets.livesSprite, -100, TITLE_SCREEN_OPTIONS_Y, 500);
    jo_sprite_draw3D(g_Assets.positionSprite, -10, TITLE_SCREEN_OPTIONS_Y, 500);
    jo_sprite_draw3D(g_Assets.startSprite, 100, TITLE_SCREEN_OPTIONS_Y, 500);

    int livesSprite = 0;
    switch(g_Game.numLivesChoice)
    {
        case 0:
            livesSprite = g_Assets.livesInfSprite;
            break;

        case 1:
            livesSprite = g_Assets.lives1Sprite;
            break;

        case 2:
            livesSprite = g_Assets.lives3Sprite;
            break;

        case 3:
            livesSprite = g_Assets.lives5Sprite;
            break;

        case 4:
            livesSprite = g_Assets.lives9Sprite;
            break;

        default:
            livesSprite = g_Assets.lives1Sprite;
            break;
    }

    // number of lives text (infinite, 1, 3, 5, 9)
    jo_sprite_draw3D(livesSprite, -70, TITLE_SCREEN_OPTIONS_Y, 500);

    int startingPositionSprite = 0;
    switch(g_Game.startingPositionChoice)
    {
        case 0:
            startingPositionSprite = g_Assets.fixedSprite;
            break;

        case 1:
            startingPositionSprite = g_Assets.randomSprite;
            break;

        default:
            startingPositionSprite = g_Assets.fixedSprite;
            break;
    }

    // starting position choice (fixed or rand)
    jo_sprite_draw3D(startingPositionSprite, 25, TITLE_SCREEN_OPTIONS_Y, 500);

    // check if we need to flap
    if(g_Game.frame >= FLICKY_FLAPPING_SPEED)
    {
        g_Game.titleFlapping = !g_Game.titleFlapping;
        g_Game.frame = 0;
    }

    int titleFlickyOffset = 0;
    switch(g_Game.titleScreenChoice)
    {
        case 0:
            titleFlickyOffset = -134;
            break;

        case 1:
            titleFlickyOffset = -36;
            break;

        case 2:
            titleFlickyOffset = 69;
            break;

        default:
            // should be impossible!
            titleFlickyOffset = 0;
            break;
    }

    // menu selection Flicky
    if(g_Game.titleFlapping == false)
    {
        jo_sprite_draw3D(g_FlickySprites[g_Game.randomFlicky].up, titleFlickyOffset, TITLE_SCREEN_OPTIONS_Y, 501);
    }
    else
    {
        jo_sprite_draw3D(g_FlickySprites[g_Game.randomFlicky].down, titleFlickyOffset, TITLE_SCREEN_OPTIONS_Y, 501);
    }

    return;
}

// handles input for the title screen menu
// only player one can control the title screen
void titleScreen_input(void)
{
    int titleScreenChoice = g_Game.titleScreenChoice;
    int numLivesChoice = g_Game.numLivesChoice;
    int startingPositionChoice = g_Game.startingPositionChoice;

    if(g_Game.gameState != GAMESTATE_TITLE_SCREEN)
    {
        return;
    }

    // did player one hit start
    if(jo_is_pad1_key_pressed(JO_KEY_START))
    {
        if(g_Game.input.pressedStart == false)
        {
            g_Game.input.pressedStart = true;
            transitionToGameplay(true);
            return;
        }
    }
    else
    {
        g_Game.input.pressedStart = false;
    }

    // did player one hit a direction key
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
    {
        if(g_Game.input.pressedLeft == false)
        {
            titleScreenChoice--;
        }
        g_Game.input.pressedLeft = true;
    }
    else
    {
        g_Game.input.pressedLeft = false;
    }

    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
    {
        if(g_Game.input.pressedRight == false)
        {
            titleScreenChoice++;
        }
        g_Game.input.pressedRight = true;
    }
    else
    {
        g_Game.input.pressedRight = false;
    }

    if (jo_is_pad1_key_pressed(JO_KEY_UP))
    {
        if(titleScreenChoice == 0 && g_Game.input.pressedUp == false)
        {
            numLivesChoice++;
        }
        g_Game.input.pressedUp = true;
    }
    else
    {
        g_Game.input.pressedUp = false;
    }

    if (jo_is_pad1_key_pressed(JO_KEY_DOWN))
    {
        if(titleScreenChoice == 0 && g_Game.input.pressedDown == false)
        {
            numLivesChoice--;
        }
        g_Game.input.pressedDown = true;
    }
    else
    {
        g_Game.input.pressedDown = false;
    }

    // validate the title screen choices
    if(titleScreenChoice < 0)
    {
        titleScreenChoice = 2;
    }

    if(titleScreenChoice > 2)
    {
        titleScreenChoice = 0;
    }
    g_Game.titleScreenChoice = titleScreenChoice;

    // did the player hit ABC
    if (jo_is_pad1_key_pressed(JO_KEY_A) ||
        jo_is_pad1_key_pressed(JO_KEY_B) ||
        jo_is_pad1_key_pressed(JO_KEY_C))
    {
        if(g_Game.titleScreenChoice == 2)
        {
            if(g_Game.input.pressedABC == false)
            {
                g_Game.gameState = GAMESTATE_GAMEPLAY;
                g_Game.input.pressedABC = true;
                transitionToGameplay(true);
                return;
            }
        }
        else if(g_Game.titleScreenChoice == 1)
        {
            if(g_Game.input.pressedABC == false)
            {
                g_Game.input.pressedABC = true;
                startingPositionChoice++;
            }
        }
        else if(g_Game.titleScreenChoice == 0)
        {
            if(g_Game.input.pressedABC == false)
            {
                g_Game.input.pressedABC = true;
                numLivesChoice++;
            }
        }
    }
    else
    {
        g_Game.input.pressedABC = false;
    }

    // validate the number of lives
    if(numLivesChoice < 0)
    {
        numLivesChoice = 4;
    }

    if(numLivesChoice > 4)
    {
        numLivesChoice = 0;
    }
    g_Game.numLivesChoice = numLivesChoice;

    // validate the position choice
    if(startingPositionChoice < 0)
    {
        startingPositionChoice = 1;
    }

    if(startingPositionChoice > 1)
    {
        startingPositionChoice = 0;
    }
    g_Game.startingPositionChoice = startingPositionChoice;

    return;
}

// draws the gameplay sprites
void gameplay_draw(void)
{
    bool allDead = false;
    int topScore = 0;
    int hunds = 0;
    int tens = 0;
    int ones = 0;

    if(g_Game.gameState != GAMESTATE_GAMEPLAY)
    {
        return;
    }

    g_Game.frame++;

    // draw players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        g_Players[i].spawnFrameTimer++;
        if(g_Players[i].spawnFrameTimer >= SPAWN_FRAME_TIMER)
        {
            // 5 seconds have passed, player must play
            g_Players[i].hasFlapped = true;
        }
        else
        {
            // the player just spawned, skip every 4th frame so they flash
            // don't flash invulnerable on the first spawn
            if(g_Players[i].numDeaths > 0 && jo_random(100) < 15)
            {
                continue;
            }
        }

        // only draw flying player or dying players
        if(g_Players[i].state == FLICKYSTATE_FLYING)
        {
            // if reverse gravity is enabled, flip the player upside down
            if(g_Players[i].reverseGravityTimer > 0)
            {
                jo_sprite_enable_vertical_flip();
            }

            // if lightning is enabled, shrink the player
            if(g_Players[i].lightningTimer > 0)
            {
                jo_sprite_change_sprite_scale(0.75);
            }

            // if flying, draw player with their angle
            if(g_Players[i].flapping)
            {
                jo_sprite_draw3D_and_rotate(g_FlickySprites[g_Players[i].spriteID].up, g_Players[i].x_pos, g_Players[i].y_pos, g_Players[i].z_pos, g_Players[i].angle);
            }
            else
            {
                jo_sprite_draw3D_and_rotate(g_FlickySprites[g_Players[i].spriteID].down, g_Players[i].x_pos, g_Players[i].y_pos, g_Players[i].z_pos, g_Players[i].angle);
            }

            if(g_Players[i].reverseGravityTimer > 0)
            {
                jo_sprite_disable_vertical_flip();
            }

            if(g_Players[i].lightningTimer > 0)
            {
                jo_sprite_change_sprite_scale(1);
            }
        }
        else if(g_Players[i].state == FLICKYSTATE_DYING)
        {
            // if lightning is enabled, shrink the player
            if(g_Players[i].lightningTimer > 0)
            {
                jo_sprite_change_sprite_scale(0.75);
            }

            jo_sprite_draw3D(g_FlickySprites[g_Players[i].spriteID].death, g_Players[i].x_pos, g_Players[i].y_pos, g_Players[i].z_pos);

            if(g_Players[i].lightningTimer > 0)
            {
                jo_sprite_change_sprite_scale(1);
            }

            // death animination flies up and pauses
            if(g_Players[i].frameTimer >= DEATH_FRAME_TIMER_FLYING)
            {
                // only adjust the height if the player is below the max
                // if I don't do this they will float off the top of the screen
                if(g_Players[i].y_pos > MAX_DEATH_HEIGHT)
                {
                    g_Players[i].y_pos -= 2;
                }
            }

            g_Players[i].frameTimer--;
            if(g_Players[i].frameTimer <= 0)
            {
                g_Players[i].state = FLICKYSTATE_DEAD;
                g_Players[i].frameTimer = FLICKY_FLAPPING_SPEED + jo_random(FLICKY_FLAPPING_SPEED);
                g_Players[i].numDeaths++;
            }
        }
    }

    // check if all players are dead
    allDead = areAllPlayersDead();
    if(allDead == true)
    {
        g_Game.frameDeathTimer++;
    }
    else
    {
        g_Game.frameDeathTimer = 0;
    }

    // check if all players are dead for enough time to end the game
    if(g_Game.frameDeathTimer >= ALL_DEAD_FRAME_TIMER)
    {
        transitionToGameOverOrPause(false);
        return;
    }

    // draw the pipes
    for(int i = 0; i < MAX_PIPES; i++)
    {
        if(g_Pipes[i].state != PIPESTATE_INITIALIZED)
        {
            continue;
        }

        for(int j = 0; j < g_Pipes[i].numSections; j++)
        {

            if(j == 0 )
            {
                jo_sprite_draw3D(g_Assets.pipeTopSprite, g_Pipes[i].x_pos, g_Pipes[i].y_pos + (j*16) + 8, 504); // top of the bottom pipe
            }
            //else
            {
                jo_sprite_draw3D(g_Assets.pipeSprite, g_Pipes[i].x_pos, g_Pipes[i].y_pos + (j*16), 505); // bottom pipe
            }

            if(j == g_Pipes[i].numSections -1)
            {
                jo_sprite_enable_vertical_flip();
                jo_sprite_draw3D(g_Assets.pipeTopSprite, g_Pipes[i].x_pos, g_Pipes[i].top_y_pos + (j*16) - 8, 504); // top pipe
                jo_sprite_disable_vertical_flip();
            }
            //else
            {
                jo_sprite_draw3D(g_Assets.pipeSprite, g_Pipes[i].x_pos, g_Pipes[i].top_y_pos + (j*16), 505); // bottom of the top pipe
            }
        }

        // shift the pipe to the left
        g_Pipes[i].x_pos--;
        if(g_Pipes[i].x_pos < -256)
        {
            initPipe(&g_Pipes[i]);
        }
    }

    // draw the powerups
    for(int i = 0; i < MAX_POWER_UPS; i++)
    {
        if(g_PowerUps[i].state != POWERUP_INITIALIZED)
        {
            continue;
        }

        jo_sprite_draw3D(g_Assets.powerUpSprites[g_PowerUps[i].type], g_PowerUps[i].x_pos, g_PowerUps[i].y_pos, g_PowerUps[i].z_pos);

        // shift the power-up to the left
        g_PowerUps[i].x_pos--;
        if(g_PowerUps[i].x_pos < -256)
        {
            initPowerUp(&g_PowerUps[i]);
        }
    }

    // draw the floor
    drawFloor();

    // shift the floor
    g_Game.floorPosition_x--;
    if(g_Game.floorPosition_x <= -480)
    {
        g_Game.floorPosition_x = -240;
    }

    // draw the score
    topScore = getTopScore();
    hunds = (topScore/100);
    tens = (topScore/10) % 10;
    ones = topScore % 10;

    // check if the player passed 100
    if(topScore >= VICTORY_CONDITION)
    {
        g_Game.topScore = topScore;
        transitionToGameOverOrPause(false);
        return;
    }

    if(hunds != 0)
    {
        jo_sprite_draw3D(g_Assets.largeDigitSprites[hunds], -16, -76, 502);
        jo_sprite_draw3D(g_Assets.largeDigitSprites[tens], 0, -76, 502);
        jo_sprite_draw3D(g_Assets.largeDigitSprites[ones], 16, -76, 502);
    }
    else if(tens != 0)
    {
        jo_sprite_draw3D(g_Assets.largeDigitSprites[tens], -8, -76, 502);
        jo_sprite_draw3D(g_Assets.largeDigitSprites[ones], 8, -76, 502);
    }
    else
    {
        jo_sprite_draw3D(g_Assets.largeDigitSprites[ones], 0, -76, 502);
    }

    return;
}

// handles the input during gameplay
void gameplay_input(void)
{
    if(g_Game.gameState != GAMESTATE_GAMEPLAY)
    {
        return;
    }

    // did player one pause the game?
    if (jo_is_pad1_key_pressed(JO_KEY_START))
    {
        if(g_Game.input.pressedStart == false)
        {
            transitionToGameOverOrPause(true);
        }
        g_Game.input.pressedStart = true;
    }
    else
    {
        g_Game.input.pressedStart = false;
    }

    // check inputs for all players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        bool flapping = false;

        if(g_Players[i].state == FLICKYSTATE_UNINITIALIZED ||
            g_Players[i].state == FLICKYSTATE_DYING )
        {
            continue;
        }

        // L and R trigger allow the player to change their Flicky
        if (jo_is_input_key_down(g_Players[i].playerID, JO_KEY_L))
        {
            if(g_Players[i].input.pressedLT == false)
            {
                g_Players[i].spriteID = getNextFlickySprite(g_Players[i].spriteID, -1);
            }
            g_Players[i].input.pressedLT = true;
        }
        else
        {
            g_Players[i].input.pressedLT = false;
        }

        if (jo_is_input_key_down(g_Players[i].playerID, JO_KEY_R))
        {
            if(g_Players[i].input.pressedRT == false)
            {
                g_Players[i].spriteID = getNextFlickySprite(g_Players[i].spriteID, 1);
            }
            g_Players[i].input.pressedRT = true;
        }
        else
        {
            g_Players[i].input.pressedRT = false;
        }

        // did the player flap
        // if the player is dead, flapping respawns them
        if (jo_is_input_key_down(g_Players[i].playerID, JO_KEY_A) ||
            jo_is_input_key_down(g_Players[i].playerID, JO_KEY_B) ||
            jo_is_input_key_down(g_Players[i].playerID, JO_KEY_C))
        {
            if(g_Players[i].input.pressedABC == false)
            {
                g_Players[i].input.pressedABC = true;
                g_Players[i].hasFlapped = true;

                // if the player was dead and hit ABC, respawn them
                if(g_Players[i].state == FLICKYSTATE_DEAD)
                {
                    spawnPlayer(i, true);
                    continue;
                }

                flapping = true;
            }
        }
        else
        {
            g_Players[i].input.pressedABC = false;
        }

        // calculate the player's y speed
        if(flapping == true)
        {
            g_Players[i].flapping = !g_Players[i].flapping;
            g_Players[i].frameTimer = FLICKY_FLAPPING_SPEED;
            g_Players[i].y_speed = FLAP_Y_SPEED;

            //
            // power-ups
            //

            // stone sneakers make jumps heavier
            if(g_Players[i].stoneSneakersTimer > 0)
            {
                g_Players[i].y_speed += 4;
            }

            // lightning makes jumps floatier
            if(g_Players[i].lightningTimer > 0)
            {
                g_Players[i].y_speed -= 3;
            }

            // reverse gravity swaps up and down
            if(g_Players[i].reverseGravityTimer > 0)
            {
                g_Players[i].y_speed *= -1;
            }
        }
        else
        {
            // player didn't flap
            g_Players[i].frameTimer--;

            if(g_Players[i].frameTimer <= 0)
            {
                g_Players[i].flapping = !g_Players[i].flapping;
                g_Players[i].frameTimer = FLICKY_FLAPPING_SPEED;
            }
        }

        // adjust the players height, but only if they are active
        if(g_Players[i].hasFlapped == true)
        {
            if(g_Players[i].reverseGravityTimer == 0)
            {
                // reverse gravity not enabled
                g_Players[i].y_pos += g_Players[i].y_speed/5 * 1;
                g_Players[i].y_speed += FALLING_CONSTANT * 1;
            }
            else
            {
                // reverse gravity enabled
                g_Players[i].y_pos += g_Players[i].y_speed/5 * 1;
                g_Players[i].y_speed -= FALLING_CONSTANT * 1;
            }
        }

        // validate speeds
        if(g_Players[i].y_speed > MAX_Y_SPEED)
        {
            g_Players[i].y_speed = MAX_Y_SPEED;
        }

        if(g_Players[i].reverseGravityTimer > 0)
        {
            if(g_Players[i].y_speed < MAX_Y_SPEED * -1)
            {
                g_Players[i].y_speed = MAX_Y_SPEED * -1;
            }
        }

        g_Players[i].angle = calculateFlickyAngle(g_Players[i].y_speed);

        if(g_Players[i].x_pos > SCREEN_RIGHT)
        {
            g_Players[i].x_pos = SCREEN_RIGHT;
        }

        if(g_Players[i].x_pos < SCREEN_LEFT)
        {
            g_Players[i].x_pos = SCREEN_LEFT;
        }

        if(g_Players[i].y_pos > SCREEN_BOTTOM)
        {
            g_Players[i].y_pos = SCREEN_BOTTOM;
        }

        if(g_Players[i].y_pos < SCREEN_TOP)
        {
            g_Players[i].y_pos = SCREEN_TOP;
        }

        //
        // decrement power-up timers
        //
        if(g_Players[i].stoneSneakersTimer > 0)
        {
            g_Players[i].stoneSneakersTimer--;
        }

        if(g_Players[i].lightningTimer > 0)
        {
            g_Players[i].lightningTimer--;
        }

        if(g_Players[i].reverseGravityTimer > 0)
        {
            g_Players[i].reverseGravityTimer--;

            if(g_Players[i].reverseGravityTimer == 0)
            {
                // reverse gravity is jarring, so 0 out their speed first
                g_Players[i].y_speed = 0;
            }
        }
    }

    return;
}

// checks for collisions between the Flicky and the ground and pipes
// also checks if the Flicky scored
// skips collision detection if Flicky is in an invulnerable state
void gameplay_checkForCollisions(void)
{
    bool result = false;

    if(g_Game.gameState != GAMESTATE_GAMEPLAY)
    {
        return;
    }

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // don't check for collisions if the player is uninitialized or dead
        if(g_Players[i].state == FLICKYSTATE_UNINITIALIZED ||
            g_Players[i].state == FLICKYSTATE_DYING ||
            g_Players[i].state == FLICKYSTATE_DEAD)
        {
            continue;
        }

        // if the player hasn't flapped yet they are still invulnerable
        if(g_Players[i].hasFlapped == false)
        {
            continue;
        }

        // if the player just spawned, don't do collision detection
        if(g_Players[i].spawnFrameTimer < SPAWN_FRAME_TIMER)
        {
            continue;
        }

        // check if the player hit the ground
        if(g_Players[i].y_pos > GROUND_COLLISION)
        {
            killPlayer(i);
            continue;
        }

        // loop through all the pipes
        for(int j = 0; j < MAX_PIPES; j++)
        {
            if(g_Pipes[j].state != PIPESTATE_INITIALIZED)
            {
                continue;
            }

            // check if the player hit the bottom pipe
            result = checkForFlickyPipeCollisions(&g_Players[i], &g_Pipes[j], false);
            if(result == true)
            {
                killPlayer(i);
                continue;
            }

            // check if the player hit the top pipe
            result = checkForFlickyPipeCollisions(&g_Players[i], &g_Pipes[j], true);
            if(result == true)
            {
                killPlayer(i);
                continue;
            }

            // no collision, did the player score?
            if(g_Players[i].x_pos == g_Pipes[j].x_pos)
            {
                // player is halfway throught the pipe, give them a point
                g_Players[i].numPoints++;
                adjustDifficulty();
            }
        }

        // loop through all the powerups
        for(int j = 0; j < MAX_POWER_UPS; j++)
        {
            if(g_PowerUps[j].state != POWERUP_INITIALIZED)
            {
                continue;
            }

            // check if the player hit the powerup
            result = checkForFlickyPowerUpCollisions(&g_Players[i], &g_PowerUps[j]);
            if(result == true)
            {
                applyPowerUp(&g_Players[i], &g_PowerUps[j]);
                continue;
            }
        }
    }

    return;
}

// Shared by both GAMESTATE_GAME_OVER and GAMESTATE_PAUSE
// Draws the score and a two choice menu at the bottom
// Game over can retry or exit
// Paused can continue or exit
void gameOver_draw(void)
{
    FLICKY sortedPlayers[MAX_PLAYERS];

    if(g_Game.gameState != GAMESTATE_GAME_OVER &&
        g_Game.gameState != GAMESTATE_VICTORY &&
        g_Game.gameState != GAMESTATE_PAUSED)
    {
        return;
    }

    g_Game.frame++;

    if(g_Game.frame >= FLICKY_FLAPPING_SPEED)
    {
        g_Game.titleFlapping = !g_Game.titleFlapping;
        g_Game.frame = 0;
    }

    // copy the players array and sort by score
    validateScores();
    memcpy(sortedPlayers, g_Players, sizeof(sortedPlayers));
    sortPlayersByScore(sortedPlayers);

    // Game over displays a "GAME OVER" sprite
    if(g_Game.gameState == GAMESTATE_GAME_OVER || g_Game.gameState == GAMESTATE_VICTORY)
    {
        jo_sprite_draw3D(g_Assets.gameOverSprite, 0, 68, 500);
    }
    else
    {
        jo_sprite_draw3D(g_Assets.pauseSprite, 0, 68, 500);
    }

    // score table
    jo_sprite_draw3D(g_Assets.seperatorHorizontalSprite, -65, -70, 500);
    jo_sprite_draw3D(g_Assets.seperatorHorizontalSprite, +65, -70, 500);
    jo_sprite_draw3D(g_Assets.seperatorVerticalSprite, 0, -17, 500);

    // heading
    jo_sprite_draw3D(g_Assets.tableRSprite, GAME_OVER_LEFT_SPRITE_POS -14, -79, 500);
    jo_sprite_draw3D(g_Assets.tableRSprite, GAME_OVER_RIGHT_SPRITE_POS -14, -79, 500);

    jo_sprite_draw3D(g_Assets.tableCSprite, GAME_OVER_LEFT_SPRITE_POS + 1, -79, 500);
    jo_sprite_draw3D(g_Assets.tableCSprite, GAME_OVER_RIGHT_SPRITE_POS + 1, -79, 500);

    jo_sprite_draw3D(g_Assets.tableSSprite, GAME_OVER_LEFT_SPRITE_POS + 27, -79, 500);
    jo_sprite_draw3D(g_Assets.tableSSprite, GAME_OVER_RIGHT_SPRITE_POS + 27, -79, 500);

    jo_sprite_draw3D(g_Assets.tablePSprite, GAME_OVER_LEFT_SPRITE_POS + 59, -79, 500);
    jo_sprite_draw3D(g_Assets.tablePSprite, GAME_OVER_RIGHT_SPRITE_POS + 59, -79, 500);

    jo_sprite_draw3D(g_Assets.tableDSprite, GAME_OVER_LEFT_SPRITE_POS + 91, -79, 500);
    jo_sprite_draw3D(g_Assets.tableDSprite, GAME_OVER_RIGHT_SPRITE_POS + 91, -79, 500);

    // draw players
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        int sprite_x_pos;
        int sprite_y_pos;
        int ones;
        int tens;
        int hunds;

        g_Players[i].frameTimer--;

        if(g_Players[i].frameTimer <= 0)
        {
            g_Players[i].flapping = !g_Players[i].flapping;
            g_Players[i].frameTimer = FLICKY_FLAPPING_SPEED;
        }

        // sprite offsets
        if(i < 6)
        {
            sprite_x_pos = GAME_OVER_LEFT_SPRITE_POS;
            sprite_y_pos = -60 + (i*20);
        }
        else
        {
            sprite_x_pos = GAME_OVER_RIGHT_SPRITE_POS;
            sprite_y_pos = -60 + ((i-6)*20);
        }

        // player rank 1-12
        ones = (i+1)%10;
        tens = (i+1)/10;

        jo_sprite_draw3D(g_Assets.smallDigitSprites[ones], sprite_x_pos - 14,sprite_y_pos + 1, 500);
        if(tens)
        {
            jo_sprite_draw3D(g_Assets.smallDigitSprites[tens], sprite_x_pos - 23,sprite_y_pos + 1, 500);
        }

        // player sprite
        if(g_Players[i].flapping == false)
        {
            jo_sprite_draw3D(g_FlickySprites[sortedPlayers[i].spriteID].up, sprite_x_pos,sprite_y_pos, 500);
        }
        else
        {
            jo_sprite_draw3D(g_FlickySprites[sortedPlayers[i].spriteID].down, sprite_x_pos,sprite_y_pos, 500);
        }

        // score
        ones = sortedPlayers[i].totalScore % 10;
        tens = (sortedPlayers[i].totalScore / 10) % 10;
        hunds = (sortedPlayers[i].totalScore / 100) % 10;

        jo_sprite_draw3D(g_Assets.smallDigitSprites[ones], sprite_x_pos + 36,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[tens], sprite_x_pos + 27,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[hunds], sprite_x_pos + 18,sprite_y_pos + 1, 500);

        // number of points
        ones = sortedPlayers[i].numPoints % 10;
        tens = (sortedPlayers[i].numPoints / 10) % 10;
        hunds = (sortedPlayers[i].numPoints / 100) % 10;

        jo_sprite_draw3D(g_Assets.smallDigitSprites[ones], sprite_x_pos + 68,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[tens], sprite_x_pos + 59,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[hunds], sprite_x_pos + 50,sprite_y_pos + 1, 500);

        // number of deaths
        ones = sortedPlayers[i].numDeaths % 10;
        tens = (sortedPlayers[i].numDeaths / 10) % 10;
        hunds = (sortedPlayers[i].numDeaths / 100) % 10;

        jo_sprite_draw3D(g_Assets.smallDigitSprites[ones], sprite_x_pos + 100,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[tens], sprite_x_pos + 91,sprite_y_pos + 1, 500);
        jo_sprite_draw3D(g_Assets.smallDigitSprites[hunds], sprite_x_pos + 82,sprite_y_pos + 1, 500);
    }

    // draw the floor
    drawFloor();

    // menu Flicky
    int titleFlickyOffset = 0;
    switch(g_Game.titleScreenChoice)
    {
        case 0:
        {
            titleFlickyOffset = -133;
            break;
        }
        case 1:
        {
            if(g_Game.gameState == GAMESTATE_GAME_OVER || g_Game.gameState == GAMESTATE_VICTORY)
            {
                titleFlickyOffset = 88;
            }
            else
            {
                titleFlickyOffset = 58;
            }
            break;
        }
        default:
        {
            // should be impossible!
            titleFlickyOffset = 0;
            break;
        }
    }

    if(g_Game.titleFlapping == false)
    {
        jo_sprite_draw3D(g_FlickySprites[g_Game.randomFlicky].up, titleFlickyOffset, 67, 501);
    }
    else
    {
        jo_sprite_draw3D(g_FlickySprites[g_Game.randomFlicky].down, titleFlickyOffset, 67, 501);
    }

    // menu options
    // game over displays retry and exit
    // pause displays continue and exit
    if(g_Game.gameState == GAMESTATE_GAME_OVER || g_Game.gameState == GAMESTATE_VICTORY)
    {
        // retry text
        jo_sprite_draw3D(g_Assets.retrySprite, -100, 67, 500);
        jo_sprite_draw3D(g_Assets.exitSprite, 115, 67, 500);
    }
    else
    {
        // continue text
        jo_sprite_draw3D(g_Assets.continueSprite, -90, 67, 500);
        jo_sprite_draw3D(g_Assets.exitSprite, 85, 67, 500);
    }

    return;
}

// handles input for the game over/pause screen
// only player one can control
void gameOver_input(void)
{
    int titleScreenChoice = g_Game.titleScreenChoice;

    if (g_Game.gameState != GAMESTATE_GAME_OVER &&
        g_Game.gameState != GAMESTATE_VICTORY &&
        g_Game.gameState != GAMESTATE_PAUSED)
    {
        return;
    }

    // ignore input for a few frames
    // this prevents a flappy player 1 from cancelling out the victory condition
    if(g_Game.frameBlockInputTimer <= BLOCK_INPUT_FRAMES)
    {
        g_Game.frameBlockInputTimer++;
        return;
    }

    // did player one hit a direction
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))
    {
        if(g_Game.input.pressedLeft == false)
        {
            titleScreenChoice--;
        }
        g_Game.input.pressedLeft = true;
    }
    else
    {
        g_Game.input.pressedLeft = false;
    }

    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT))
    {
        if(g_Game.input.pressedRight == false)
        {
            titleScreenChoice++;
        }
        g_Game.input.pressedRight = true;
    }
    else
    {
        g_Game.input.pressedRight = false;
    }

    // did player one press Z to clear the scores
    if (jo_is_pad1_key_pressed(JO_KEY_Z))
    {
        if(g_Game.input.pressedZ == false)
        {
            clearScores();
        }
        g_Game.input.pressedZ = true;
    }
    else
    {
        g_Game.input.pressedZ = false;
    }

    // validate choice
    if(titleScreenChoice < 0)
    {
        titleScreenChoice = 1;
    }

    if(titleScreenChoice > 1)
    {
        titleScreenChoice = 0;
    }
    g_Game.titleScreenChoice = titleScreenChoice;

    // did player one press ABC or Start
    if (jo_is_pad1_key_pressed(JO_KEY_A) ||
        jo_is_pad1_key_pressed(JO_KEY_B) ||
        jo_is_pad1_key_pressed(JO_KEY_C) ||
        jo_is_pad1_key_pressed(JO_KEY_START))
    {
        if(g_Game.titleScreenChoice == 0)
        {
            if(g_Game.input.pressedABC == false &&
                g_Game.input.pressedStart == false)
            {
                if(g_Game.gameState == GAMESTATE_GAME_OVER || g_Game.gameState == GAMESTATE_VICTORY)
                {
                    transitionToGameplay(true);
                }
                else
                {
                    transitionToGameplay(false);
                }
                return;
            }
        }
        else if(g_Game.titleScreenChoice == 1)
        {
            if(g_Game.input.pressedABC == false &&
                g_Game.input.pressedStart == false)
            {
                g_Game.input.pressedABC = true;
                g_Game.input.pressedStart = true;
                transitionToTitleScreen();
            }
        }
    }
    else
    {
        g_Game.input.pressedABC = false;
        g_Game.input.pressedStart = false;
    }

    return;
}

//
// loading resources
//

// loads sprites from the CD to the g_Assets struct
void loadSpriteAssets()
{
    jo_tile scoreDigits[NUM_DIGITS] = {0};
    jo_tile pointsDigits[NUM_DIGITS] = {0};
    jo_tile tableLetters[NUM_TABLE_LETTERS] = {0};
    jo_tile flickieSprites[NUM_FLICKY_SPRITES] = {0};
    jo_tile powerUpSprites[NUM_POWER_UPS] = {0};

    for(int i = 0; i < NUM_DIGITS; i++)
    {
        scoreDigits[i].x = i*16;
        scoreDigits[i].y = 0;
        scoreDigits[i].height = 16;
        scoreDigits[i].width = 16;

        pointsDigits[i].x = i*8;
        pointsDigits[i].y = 0;
        pointsDigits[i].height = 8;
        pointsDigits[i].width = 8;
    }

    for(int i = 0; i < NUM_TABLE_LETTERS; i++)
    {
        tableLetters[i].x = i*8;
        tableLetters[i].y = 0;
        tableLetters[i].height = 8;
        tableLetters[i].width = 8;
    }

    for(int i = 0; i < NUM_FLICKY_SPRITES; i++)
    {
        flickieSprites[i].x = i*24;
        flickieSprites[i].y = 0;
        flickieSprites[i].height = 24;
        flickieSprites[i].width = 24;
    }

    for(int i = 0; i < NUM_POWER_UPS; i++)
    {
        powerUpSprites[i].x = i*16;
        powerUpSprites[i].y = 0;
        powerUpSprites[i].height = 16;
        powerUpSprites[i].width = 16;
    }

    //
    // Performance hack: instead of repeatedly switching and unswitching dirs in jo_sprite_add_tga()
    // do it only once here. Otherwise the TEX dir is repeatedly loaded and unloaded which is
    // very slow. This increases boot time by around ~4 seconds. Credit to ReyeMe.
    //
    jo_fs_cd("TEX");

    // SSMTF logo
    g_Assets.SSMTF1Sprite = jo_sprite_add_tga(NULL, "SSMTF1.TGA", JO_COLOR_Transparent);
    g_Assets.SSMTF2Sprite = jo_sprite_add_tga(NULL, "SSMTF2.TGA", JO_COLOR_Transparent);
    g_Assets.SSMTF3Sprite = jo_sprite_add_tga(NULL, "SSMTF3.TGA", JO_COLOR_Transparent);
    g_Assets.SSMTF4Sprite = jo_sprite_add_tga(NULL, "SSMTF4.TGA", JO_COLOR_Transparent);

    // title screen
    g_Assets.titleSprite = jo_sprite_add_tga(NULL, "TITLE.TGA", JO_COLOR_Transparent);
    g_Assets.livesSprite = jo_sprite_add_tga(NULL, "LIVES.TGA", JO_COLOR_Transparent);
    g_Assets.livesInfSprite = jo_sprite_add_tga(NULL, "LIVES0.TGA", JO_COLOR_Transparent);
    g_Assets.lives1Sprite = jo_sprite_add_tga(NULL, "LIVES1.TGA", JO_COLOR_Transparent);
    g_Assets.lives3Sprite = jo_sprite_add_tga(NULL, "LIVES3.TGA", JO_COLOR_Transparent);
    g_Assets.lives5Sprite = jo_sprite_add_tga(NULL, "LIVES5.TGA", JO_COLOR_Transparent);
    g_Assets.lives9Sprite = jo_sprite_add_tga(NULL, "LIVES9.TGA", JO_COLOR_Transparent);
    g_Assets.startSprite = jo_sprite_add_tga(NULL, "START.TGA", JO_COLOR_Transparent);
    g_Assets.positionSprite = jo_sprite_add_tga(NULL, "POS.TGA", JO_COLOR_Transparent);
    g_Assets.randomSprite = jo_sprite_add_tga(NULL, "RAND.TGA", JO_COLOR_Transparent);
    g_Assets.fixedSprite = jo_sprite_add_tga(NULL, "FIXED.TGA", JO_COLOR_Transparent);

    // background
    g_Assets.background.data = NULL;
    jo_tga_loader(&g_Assets.background, NULL, "BG.TGA", JO_COLOR_Transparent);

    // gameplay sprites
    g_Assets.floorSprite = jo_sprite_add_tga(NULL, "FLOOR.TGA", JO_COLOR_Transparent);
    g_Assets.pipeSprite = jo_sprite_add_tga(NULL, "PIPE.TGA", JO_COLOR_Transparent);
    g_Assets.pipeTopSprite = jo_sprite_add_tga(NULL, "PIPETOP.TGA", JO_COLOR_Transparent);

    // pause/game over sprites
    g_Assets.seperatorHorizontalSprite = jo_sprite_add_tga(NULL, "SEPH.TGA", JO_COLOR_Transparent);
    g_Assets.seperatorVerticalSprite = jo_sprite_add_tga(NULL, "SEPV.TGA", JO_COLOR_Transparent);
    g_Assets.gameOverSprite = jo_sprite_add_tga(NULL, "GAMEO.TGA", JO_COLOR_Transparent);
    g_Assets.pauseSprite = jo_sprite_add_tga(NULL, "PAUSE.TGA", JO_COLOR_Transparent);
    g_Assets.retrySprite = jo_sprite_add_tga(NULL, "GRETR.TGA", JO_COLOR_Transparent);
    g_Assets.exitSprite = jo_sprite_add_tga(NULL, "GEXIT.TGA", JO_COLOR_Transparent);
    g_Assets.continueSprite = jo_sprite_add_tga(NULL, "GCONT.TGA", JO_COLOR_Transparent);

    // score and points digits 0-9
    g_Assets.largeDigitSprites[0] = jo_sprite_add_tga_tileset(NULL, "SDIGIT.TGA", JO_COLOR_Transparent, scoreDigits, COUNTOF(scoreDigits));
    g_Assets.smallDigitSprites[0] = jo_sprite_add_tga_tileset(NULL, "PDIGIT.TGA", JO_COLOR_Transparent, pointsDigits, COUNTOF(pointsDigits));

    for(int i = 0; i < NUM_DIGITS; i++)
    {
        g_Assets.largeDigitSprites[i] = g_Assets.largeDigitSprites[0] + i;
        g_Assets.smallDigitSprites[i] = g_Assets.smallDigitSprites[0] + i;
    }

    // table heading letters
    g_Assets.tableCSprite = jo_sprite_add_tga_tileset(NULL, "TABLEA.TGA", JO_COLOR_Transparent, tableLetters, COUNTOF(tableLetters));
    g_Assets.tableDSprite = g_Assets.tableCSprite + 1;
    g_Assets.tablePSprite = g_Assets.tableCSprite + 2;
    g_Assets.tableRSprite = g_Assets.tableCSprite + 3;
    g_Assets.tableSSprite = g_Assets.tableCSprite + 4;

    // player sprites
    g_FlickySprites[0].death = jo_sprite_add_tga_tileset(NULL, "FLICKY.TGA", JO_COLOR_Transparent, flickieSprites, COUNTOF(flickieSprites));

    for(int i = 0; i < MAX_FLICKY_SPRITES; i++)
    {
        // x3 because there are three sprites for each flicky
        g_FlickySprites[i].death = g_FlickySprites[0].death + (i*3);
        g_FlickySprites[i].up = g_FlickySprites[0].death + (i*3) + 1;
        g_FlickySprites[i].down = g_FlickySprites[0].death + (i*3) + 2;
    }

    // power-up sprites
    g_Assets.powerUpSprites[0] = jo_sprite_add_tga_tileset(NULL, "POWERUPS.TGA", JO_COLOR_Transparent, powerUpSprites, COUNTOF(powerUpSprites));

    for(int i = 0; i < NUM_POWER_UPS; i++)
    {
        g_Assets.powerUpSprites[i] = g_Assets.powerUpSprites[0] + i;
    }

    // back to root dir
    jo_fs_cd("..");

    return;
}

// loads PCMs from the CD to the g_Assets struct
void loadPCMAssets(void)
{
    jo_audio_load_pcm("DEATH.PCM", JoSoundMono8Bit, &g_Assets.deathPCM);
    g_Assets.deathPCM.sample_rate = 27086;

    jo_audio_load_pcm("GRAVITY.PCM", JoSoundMono8Bit, &g_Assets.reverseGravityPCM);
    g_Assets.reverseGravityPCM.sample_rate = 27086;

    jo_audio_load_pcm("LIGHT.PCM", JoSoundMono8Bit, &g_Assets.lightningPCM);
    g_Assets.lightningPCM.sample_rate = 27086;

    jo_audio_load_pcm("STONE.PCM", JoSoundMono8Bit, &g_Assets.stoneSneakersPCM);
    g_Assets.stoneSneakersPCM.sample_rate = 27086;

    return;
}

// sets the background
void setBackground(void)
{
    jo_set_background_sprite(&g_Assets.background, 0, 0);
    return;
}

//
// transition states
//

// enter GAMESTATE_TITLE_SCREEN
void transitionToTitleScreen(void)
{
    g_Game.frame = 0;
    g_Game.randomFlicky = jo_random(MAX_PLAYERS) - 1;
    g_Game.titleScreenChoice = 0;

    for(unsigned int i = 0; i < MAX_FLICKY_SPRITES; i++)
    {
        randomizeTitleFlicky(&g_FlickyTitleSprites[i]);
    }

    jo_clear_screen();
    setBackground();

    jo_audio_play_cd_track(TITLE_TRACK, TITLE_TRACK, false);
    g_Game.gameState = GAMESTATE_TITLE_SCREEN;

    return;
}

// enter GAMESTATE_GAMEPLAY
void transitionToGameplay(bool newGame)
{
    g_Game.frame = 0;
    g_Game.randomFlicky = jo_random(MAX_PLAYERS) - 1;
    g_Game.titleScreenChoice = 0;
    g_Game.frameDeathTimer = 0;
    g_Game.frameBlockInputTimer = 0;

    jo_clear_screen();
    setBackground();
    g_Game.input.pressedStart = true;
    g_Game.input.pressedABC = true;

    // we reset the players
    if(newGame == true)
    {
        memset(&g_Pipes, 0, sizeof(g_Pipes));
        initPipe(&g_Pipes[0]);

        memset(&g_PowerUps, 0, sizeof(g_PowerUps));
        initPowerUp(&g_PowerUps[0]);

        // randomize starting positions if specified
        if(g_Game.startingPositionChoice == 1)
        {
            shuffleArray(g_StartingPositions, COUNTOF(g_StartingPositions));
        }
        else
        {
            for(unsigned int i = 0; i < MAX_PLAYERS; i++)
            {
                g_StartingPositions[i] = i;
            }
        }

        initPlayers();
        g_Game.topScore = 0;
    }

    jo_audio_play_cd_track(GAMEPLAY_TRACK, GAMEPLAY_TRACK, true);

    g_Game.gameState = GAMESTATE_GAMEPLAY;
    return;
}

// enter the GAMESTATE_GAME_OVER or GAMESTATE_PAUSED
void transitionToGameOverOrPause(bool pause)
{
    // pause the music
    jo_audio_stop_cd();
    g_Game.input.pressedABC = true;
    g_Game.input.pressedStart = true;
    g_Game.frameBlockInputTimer = 0; // block input for 2 seconds

    if(pause == true)
    {
        g_Game.gameState = GAMESTATE_PAUSED;
    }
    else
    {
        if(g_Game.topScore >= VICTORY_CONDITION)
        {
            //jo_core_error("In victory");
            jo_audio_play_cd_track(LIFE_UP_TRACK, LIFE_UP_TRACK, false);
            g_Game.gameState = GAMESTATE_VICTORY;
        }
        else
        {
            jo_audio_play_cd_track(GAMEOVER_TRACK, GAMEOVER_TRACK, false);
            g_Game.gameState = GAMESTATE_GAME_OVER;
        }
    }

    return;
}

//
// Flicky
//

// initializes the players array
void initPlayers()
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        g_Players[i].playerID = i;

        // if the player changed their character before let them keep it
        if(g_Players[i].spriteID == 0)
        {
            g_Players[i].spriteID = i;
        }

        g_Players[i].numPoints = 0;
        g_Players[i].totalScore = 0;
        g_Players[i].numDeaths = 0;

        memset(&g_Players[i].input, 0, sizeof(INPUTCACHE));
        g_Players[i].input.pressedABC = true; // in case player 1 hit ABC on the title screen

        spawnPlayer(i, false);
    }

    return;
}

// spawns a player
// if deductLife is true, checks if the player has enough lives
// before spawning
void spawnPlayer(int playerID, bool deductLife)
{
    int livesLimit = g_InitialLives[g_Game.numLivesChoice];

    // check if the player has a life to lose and
    // we aren't on unlimited lives
    if(deductLife == true && livesLimit != 0)
    {
        if(g_Players[playerID].numDeaths >= livesLimit)
        {
            // player is out of lives, don't respawn
            return;
        }
    }

    getStartingPosition(g_Players[playerID].playerID, &g_Players[playerID].x_pos, &g_Players[playerID].y_pos, &g_Players[playerID].z_pos);
    g_Players[playerID].y_speed = 0;
    g_Players[playerID].angle = 0;

    g_Players[playerID].flapping = false;
    g_Players[playerID].hasFlapped = false;
    g_Players[playerID].frameTimer = FLICKY_FLAPPING_SPEED + jo_random(FLICKY_FLAPPING_SPEED);

    // reset various frame timers
    g_Players[playerID].spawnFrameTimer = 0;
    g_Players[playerID].reverseGravityTimer = 0;
    g_Players[playerID].lightningTimer = 0;
    g_Players[playerID].stoneSneakersTimer = 0;

    g_Players[playerID].state = FLICKYSTATE_FLYING;
}

// kills the player and plays a death sound
void killPlayer(int playerID)
{
    g_Players[playerID].state = FLICKYSTATE_DYING;
    g_Players[playerID].frameTimer = FLICKY_DEATH_FRAME_TIMER;
    jo_audio_play_sound(&g_Assets.deathPCM);
    return;
}

// gives the player an extra life and plays
// the one up track
void extraLifePlayer(int playerID)
{
    if(g_Players[playerID].numDeaths > 0)
    {
        g_Players[playerID].numDeaths--;
    }

    jo_audio_play_cd_track(LIFE_UP_TRACK, GAMEPLAY_TRACK, false);
}


// returns true if all the player are currently dead
// used to determine when to end the game
bool areAllPlayersDead(void)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        if(g_Players[i].state != FLICKYSTATE_DEAD)
        {
            return false;
        }
    }

    return true;
}

// determines the angle of the Flicky depending on the
// y speed
int calculateFlickyAngle(int speed)
{
    // y speed = -15 to 25

    if(speed < -7)
    {
        return -15;
    }
    else if(speed < -5)
    {
        return -8;
    }
    else if(speed < 5)
    {
        return 0;
    }
    else if(speed < 10)
    {
        return 20;
    }
    else if(speed < 20)
    {
        return 45;
    }
    else
    {
        return 90;
    }

    // should never get here
    return 0;
}

// gets the next or previous Flicky sprite ID
int getNextFlickySprite(int spriteID, int offset)
{
    int newSpriteID = spriteID + offset;

    if(newSpriteID < 0)
    {
        newSpriteID = MAX_FLICKY_SPRITES - 1;
    }

    if(newSpriteID >= MAX_FLICKY_SPRITES)
    {
        newSpriteID = 0;
    }

    return newSpriteID;
}

// shuffles an array of integers
void shuffleArray(unsigned int* array, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size - 1; i++)
    {
        unsigned int j = i + jo_random(0xFFFF) / (0xFFFF / (size - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }

    return;
}

// starting position of the Flicky depending on player ID
void getStartingPosition(int playerID, int* x_pos, int* y_pos, int* z_pos)
{
    int spacing = 28;
    int verticalSpacing = 8;

    playerID = g_StartingPositions[playerID];

    if(playerID < 6)
    {
        *x_pos = 0 - (playerID * spacing);
        *y_pos = -25 + (playerID * verticalSpacing);
    }
    else
    {
        *x_pos = 0 - ((playerID - 5) * spacing) + spacing/2;
        *y_pos = -25 - ((playerID - 5) * verticalSpacing);
    }

    *z_pos = 500 - playerID;

    return;
}

//
// pipes, floor
//

// initializes a pipe
void initPipe(PPIPE pipe)
{
    bool safeSpawn = false;

    if(pipe->state == PIPESTATE_INITIALIZED)
    {
        if(pipe->x_pos > -256)
        {
            return;
        }
    }

    pipe->state = PIPESTATE_UNINITIALIZED;

    // bottom half
    pipe->x_pos = getNextPipePosition();
    pipe->y_pos = -20 + jo_random(60);
    pipe->numSections = 10;

    pipe->gap = 48 + jo_random(40);

    // top half
    pipe->top_y_pos = pipe->y_pos - pipe->gap - pipe->numSections*16;

    if(pipe->top_y_pos < -220)
    {
        int diff = pipe->top_y_pos - -220;

        pipe->y_pos -= diff;

        //jo_printf(2, 3, "top y: %d gap: %d     ", pipe->gap, pipe->top_y_pos);
        pipe->top_y_pos = -220;
    }

    // make sure we don't collide with a pipe
    while(safeSpawn == false)
    {
        int j = 0;
        bool result = false;

        // loop through all the pipes
        for(j = 0; j < MAX_POWER_UPS; j++)
        {
            if(g_PowerUps[j].state != POWERUP_INITIALIZED)
            {
                continue;
            }

            // check if the player hit the bottom pipe
            result = checkForPowerUpPipeCollisions(&g_PowerUps[j], pipe, false);
            if(result == true)
            {
                //jo_core_error("collision detected");
                pipe->x_pos += 32 + jo_random(32);
                break;
            }

            // check if the player hit the bottom pipe
            result = checkForPowerUpPipeCollisions(&g_PowerUps[j], pipe, true);
            if(result == true)
            {
                //jo_core_error("collision detected");
                pipe->x_pos += 32 + jo_random(32);
                break;
            }
        }

        if(j == MAX_POWER_UPS)
        {
            safeSpawn = true;
        }
    }

    pipe->state = PIPESTATE_INITIALIZED;
}

// determines where to place the next pipe
int getNextPipePosition(void)
{
    int x_pos = 0;
    int difficulty = 0;

    for(int i = 0; i < MAX_PIPES; i++)
    {
        if(g_Pipes[i].state != PIPESTATE_INITIALIZED)
        {
            continue;
        }

        if(g_Pipes[i].x_pos > x_pos)
        {
            x_pos = g_Pipes[i].x_pos;
        }
    }

    if(x_pos <= 0)
    {
        // no pipe is active, spawn slightly off screen
        return 256;
    }
    else // x >= 256
    {
        difficulty = getDifficulty();

        // pipe is off-screen, just add a random offset
        return x_pos + (180 - (10 * difficulty)) + jo_random(200 - (10*difficulty));
    }

    // if the furthest pipe is being displayed
    // or not initialized, pick an offscreen
    // value
    if(x_pos < 256)
    {
        x_pos = 256;
    }

    return x_pos;

}

// checks if the Flicky collided with the pipe
// if top == true, checks for the top pipe
bool checkForFlickyPipeCollisions(PFLICKY player, PPIPE pipe, bool top)
{
    int playerWidth = 12;
    int playerHeight = 12;
    int pipeWidth = 58;
    int pipeHeight = 16 * pipe->numSections;

    int pl_x = player->x_pos - 6;
    int pl_y = player->y_pos - 6;
    int pi_x = pipe->x_pos - 25;
    int pi_y = 0;

    // if the player is shrunk, shrink their hitbox
    if(player->lightningTimer > 0)
    {
        playerWidth -= 2;
        playerHeight -= 2;
    }

    if(top == true)
    {
        pi_y = pipe->y_pos - 8;
    }
    else
    {
        pi_y = pipe->top_y_pos - 8;
    }

    if(pl_x < pi_x + pipeWidth &&
        pl_x + playerWidth > pi_x &&
        pl_y < pi_y + pipeHeight &&
        pl_y + playerHeight > pi_y)
    {
        // collision detected
        return true;
    }

    // no collision
    return false;
}

// checks if the power-up collided with the pipe
// if top == true, checks for the top pipe
// useful for not spawning a power-up over a pipe
bool checkForPowerUpPipeCollisions(PPOWERUP powerup, PPIPE pipe, bool top)
{
    int powerupWidth = 32;
    int powerupHeight = 32;
    int pipeWidth = 64;
    int pipeHeight = 16 * pipe->numSections;

    int pu_x = powerup->x_pos - 8;
    int pu_y = powerup->y_pos - 8;
    int pi_x = pipe->x_pos - 25;
    int pi_y = 0;

    if(top == true)
    {
        pi_y = pipe->y_pos - 8;
    }
    else
    {
        pi_y = pipe->top_y_pos - 8;
    }

    if(pu_x < pi_x + pipeWidth &&
        pu_x + powerupWidth > pi_x &&
        pu_y < pi_y + pipeHeight &&
        pu_y + powerupHeight > pi_y)
    {
        // collision detected
        return true;
    }

    // no collision
    return false;
}

// draws the floor
void drawFloor(void)
{
    jo_sprite_draw3D(g_Assets.floorSprite, g_Game.floorPosition_x, FLOOR_POSITION_Y, 504);
    jo_sprite_draw3D(g_Assets.floorSprite, g_Game.floorPosition_x + FLOOR_WIDTH, FLOOR_POSITION_Y, 504);
    jo_sprite_draw3D(g_Assets.floorSprite, g_Game.floorPosition_x + FLOOR_WIDTH*2, FLOOR_POSITION_Y, 504);
    jo_sprite_draw3D(g_Assets.floorSprite, g_Game.floorPosition_x + FLOOR_WIDTH*3, FLOOR_POSITION_Y, 504);
}

// the games difficulty is based on the score
int getDifficulty(void)
{
    int difficulty = getTopScore() / 10;

    if(difficulty < 0)
    {
        return 0;
    }
    else if(difficulty > 10)
    {
        return 10;
    }
    else
    {
        return difficulty;
    }

    return 0;
}

// every 10 points the game gets harder
void adjustDifficulty(void)
{
    int difficulty = getDifficulty();

    switch(difficulty)
    {
        // intentional fall-throughs
        case 10:
        case 9:
        case 8:
        case 7:
        case 6:
        case 5:
        case 4:
            initPipe(&g_Pipes[5]);
            // fall through
        case 3:
            initPipe(&g_Pipes[4]);
            // fall through
        case 2:
            initPipe(&g_Pipes[3]);
            // fall through
        case 1:
            initPipe(&g_Pipes[2]);
            // fall through
        case 0:
            initPipe(&g_Pipes[1]);
            initPipe(&g_Pipes[0]);
            break;
    }

    return;
}

// returns the number of active pipes
int getNumberofPipes(void)
{
    int numPipes = 0;

    for(int i = 0; i < MAX_PIPES; i++)
    {
        if(g_Pipes[i].state == PIPESTATE_INITIALIZED)
        {
            numPipes++;
        }
    }

    return numPipes;
}

//
// power-ups
//

// initialize a power-up
void initPowerUp(PPOWERUP powerup)
{
    bool safeSpawn = false;
    int randOffset = 0;

    if(powerup->state == POWERUP_INITIALIZED)
    {
        // power-up is still possibly on screen,
        // don't do anything
        if(powerup->x_pos > -256)
        {
            return;
        }
    }

    powerup->state = POWERUP_UNINITIALIZED;

    // bottom half
    powerup->x_pos = getNextPowerUpPosition();

    randOffset = jo_random(40);
    if(randOffset < 20)
    {
        powerup->y_pos = 50 - randOffset;
    }
    else
    {
        powerup->y_pos = -80 + randOffset - 20;
    }
    powerup->z_pos = 503;

    // make sure we don't collide with a pipe
    while(safeSpawn == false)
    {
        int j = 0;
        bool result = false;

        // loop through all the pipes
        for(j = 0; j < MAX_PIPES; j++)
        {
            if(g_Pipes[j].state != PIPESTATE_INITIALIZED)
            {
                continue;
            }

            // check if the player hit the bottom pipe
            result = checkForPowerUpPipeCollisions(powerup, &g_Pipes[j], false);
            if(result == true)
            {
                //jo_core_error("collision detected");
                powerup->x_pos += 64 + jo_random(128);
                break;
            }

            // check if the player hit the bottom pipe
            result = checkForPowerUpPipeCollisions(powerup, &g_Pipes[j], true);
            if(result == true)
            {
                //jo_core_error("collision detected");
                powerup->x_pos += 64 + jo_random(128);
                break;
            }
        }

        if(j == MAX_PIPES)
        {
            // safe spawn location
            safeSpawn = true;
        }
    }

    // randomly select the type of powerup
    powerup->type = jo_random(NUM_POWER_UPS) - 1;

    powerup->state = POWERUP_INITIALIZED;
}

// determines where to place the next power-up
int getNextPowerUpPosition(void)
{
    int x_pos = 0;

    for(int i = 0; i < MAX_POWER_UPS; i++)
    {
        if(g_PowerUps[i].state != POWERUP_INITIALIZED)
        {
            continue;
        }

        if(g_PowerUps[i].x_pos > x_pos)
        {
            x_pos = g_PowerUps[i].x_pos;
        }
    }

    if(x_pos <= 0)
    {
        // no powerup is active
        x_pos = 512 + jo_random(200);
    }
    else
    {
        // power-ups exist, just add a random offset
        x_pos = x_pos + 160 + jo_random(200);
    }

    return x_pos;
}

// checks if the Flicky collided with the power-up
bool checkForFlickyPowerUpCollisions(PFLICKY player, PPOWERUP powerup)
{
    int playerWidth = 12;
    int playerHeight = 12;
    int pipeWidth = 16;
    int pipeHeight = 16;

    int pl_x = player->x_pos - 8;
    int pl_y = player->y_pos - 8;
    int pi_x = powerup->x_pos - 8;
    int pi_y = 0;

    // if the player is shrunk, shrink their hitbox
    if(player->lightningTimer > 0)
    {
        playerWidth -= 2;
        playerHeight -= 2;
    }

    pi_y = powerup->y_pos - 8;

    if(pl_x < pi_x + pipeWidth &&
        pl_x + playerWidth > pi_x &&
        pl_y < pi_y + pipeHeight &&
        pl_y + playerHeight > pi_y)
    {
        // collision detected
        return true;
    }

    // no collision
    return false;
}

// apply the power-up to the player (or the power-down
// to his opponents)
void applyPowerUp(PFLICKY player, PPOWERUP powerup)
{
    if(powerup->state != POWERUP_INITIALIZED)
    {
        // what the heck this should never happen
        return;
    }

    powerup->state = POWERUP_UNINITIALIZED;

    switch(powerup->type)
    {
        case POWERUP_ONE_UP:
        {
            // one up only applies to the player who picked it up
            extraLifePlayer(player->playerID);
            break;
        }
        case POWERUP_REVERSE_GRAVITY:
        {
            jo_audio_play_sound(&g_Assets.reverseGravityPCM);

            // apply stone sneakers to all players
            for(int i = 0; i < MAX_PLAYERS; i++)
            {
                if(g_Players[i].state != FLICKYSTATE_FLYING)
                {
                    continue;
                }

                g_Players[i].reverseGravityTimer += REVERSE_GRAVITY_TIMER;

                // swapping gravity is jarring, be nice and set their vertical speed to 0
                g_Players[i].y_speed = 0;
            }
            break;
        }
        case POWERUP_LIGHTNING:
        {
            jo_audio_play_sound(&g_Assets.lightningPCM);

            // apply stone sneakers to all players
            for(int i = 0; i < MAX_PLAYERS; i++)
            {
                if(g_Players[i].state != FLICKYSTATE_FLYING)
                {
                    continue;
                }
                g_Players[i].lightningTimer += LIGHTNING_TIMER;
            }
            break;
        }
        case POWERUP_ROBOTNIK:
        {
            // Robotnik only applies to the player who picked it up
            killPlayer(player->playerID);
            break;
        }
        case POWERUP_STONE_SNEAKERS:
        {
            jo_audio_play_sound(&g_Assets.stoneSneakersPCM);

            // apply stone sneakers to all players
            for(int i = 0; i < MAX_PLAYERS; i++)
            {
                if(g_Players[i].state != FLICKYSTATE_FLYING)
                {
                    continue;
                }
                g_Players[i].stoneSneakersTimer += STONE_SNEAKERS_TIMER;
            }
            break;
        }
        default:
        {
            return;
        }
    }

    // we consumed a powerup, create another
    initPowerUp(powerup);
}

//
// scoring
//

// reset scores
void clearScores(void)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        g_Players[i].numPoints = 0;
        g_Players[i].numDeaths = 0;
        g_Players[i].totalScore = 0;
    }

    return;
}

// loops through all the players and returns the highest score
// the highest score is the one that is displayed during gameplay
int getTopScore(void)
{
    int score = 0;

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        g_Players[i].totalScore = g_Players[i].numPoints - g_Players[i].numDeaths;

        if(g_Players[i].totalScore > score)
        {
            score = g_Players[i].totalScore;
        }
    }

    if(score < 0 || score > 999)
    {
        // should never happen
        score = 0;
    }

    return score;
}

// Sort the players array using insertion sort
void sortPlayersByScore(PFLICKY players)
{
    int i, j;

    FLICKY key = {0};

    for (i = 1; i < MAX_PLAYERS; i++)
    {
        key = players[i];
        j = i - 1;

        while (j >= 0 && players[j].totalScore < key.totalScore)
        {
            players[j + 1] = players[j];
            j = j - 1;
        }
        players[j + 1] = key;
    }

    return;
}

// normalize scores before displaying in game over or pause screen
void validateScores(void)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        // normalize score data
        g_Players[i].numDeaths = (g_Players[i].numDeaths < 0) ? 0 : g_Players[i].numDeaths;
        g_Players[i].numDeaths = (g_Players[i].numDeaths > 999) ? 999 : g_Players[i].numDeaths;

        g_Players[i].numPoints = (g_Players[i].numPoints < 0) ? 0 : g_Players[i].numPoints;
        g_Players[i].numPoints = (g_Players[i].numPoints > 999) ? 999 : g_Players[i].numPoints;

        g_Players[i].totalScore = g_Players[i].numPoints - g_Players[i].numDeaths;
        g_Players[i].totalScore = (g_Players[i].totalScore < 0) ? 0 : g_Players[i].totalScore;
        g_Players[i].totalScore = (g_Players[i].totalScore > 999) ? 999 : g_Players[i].totalScore;
    }

    return;
}
