#pragma once

//
// Reimplementations of Jo Engine functions for either performance or
// compatability reasons
//
#include <jo/jo.h>
#include "stdbool.h"

// changes the PCM channels to play at 8000hz. The pitch doesn't appear to be accessible without modifying Jo Engine
void my_jo_audio_play_sound(jo_sound * const sound);
