//
// Reimplementations of Jo Engine functions for either performance or
// compatability reasons
//
#include <stdbool.h>
#include "jo/sgl_prototypes.h"
#include "jo/conf.h"
#include "jo/types.h"
#include "jo/sega_saturn.h"
#include "jo/smpc.h"
#include "jo/core.h"
#include "jo/tools.h"
#include "jo/vdp1_command_pipeline.h"
#include "jo/colors.h"
#include "jo/malloc.h"
#include "jo/fs.h"
#include "jo/image.h"
#include "jo/sprites.h"
#include "jo/math.h"
#include "jo/list.h"
#include "jo/3d.h"
#include "jo/audio.h"

//
// functions to replace jo_audio_play_sound so that I can use a different pitch then default.
// Jo Engine doesn't appear to expose the __jo_internal_pcm table for editing
// see: https://github.com/johannes-fetz/joengine/issues/36
//
static PCM __my_jo_internal_pcm[JO_SOUND_MAX_CHANNEL] =
{
    {(_Mono | _PCM8Bit), 0, 127, 0, 0x69ce, 0, 0, 0, 0},
    {(_Mono | _PCM8Bit), 2, 127, 0, 0x69ce, 0, 0, 0, 0},
    {(_Mono | _PCM8Bit), 4, 127, 0, 0x69ce, 0, 0, 0, 0},
    {(_Mono | _PCM8Bit), 6, 127, 0, 0x69ce, 0, 0, 0, 0},
    {(_Mono | _PCM8Bit), 8, 127, 0, 0x69ce, 0, 0, 0, 0},
    {(_Mono | _PCM8Bit), 10, 127, 0, 0x69ce, 0, 0, 0, 0},
};

void my_jo_audio_play_sound_on_channel(jo_sound * const sound, const unsigned char channel)
{
#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
    if (channel >= JO_SOUND_MAX_CHANNEL)
    {
        jo_core_error("channel (%d) is too high (max=%d)", channel, JO_SOUND_MAX_CHANNEL);
        return;
    }
#endif
    if (slPCMStat(&__my_jo_internal_pcm[(int)channel]))
        return ;
    slSndFlush();
    sound->current_playing_channel = channel;
    __my_jo_internal_pcm[(int)channel].mode = (Uint8)sound->mode;
    slPCMOn(&__my_jo_internal_pcm[(int)channel], sound->data, sound->data_length);
}

void my_jo_audio_play_sound(jo_sound * const sound)
{
    register unsigned int   i;

#ifdef JO_DEBUG
    if (sound == JO_NULL)
    {
        jo_core_error("sound is null");
        return;
    }
#endif
    if (slSndPCMNum(sound->mode) < 0)
    {
#ifdef JO_DEBUG
        jo_core_error("No available channel");
#endif
        return ;
    }
    for (JO_ZERO(i); i < JO_SOUND_MAX_CHANNEL; ++i)
    {
        if (!slPCMStat(&__my_jo_internal_pcm[i]))
        {
            my_jo_audio_play_sound_on_channel(sound, i);
            return ;
        }
    }
}
