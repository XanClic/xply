/******************************************************************************
 * Copyright (c) 2009, 2010 Hanna Reitz                                       *
 *                                                                            *
 * Permission is hereby granted,  free of charge,  to any  person obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use,  copy, modify, merge, publish,  distribute, sublicense, *
 * and/or sell copies  of the  Software,  and to permit  persons to whom  the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING  BUT NOT  LIMITED TO THE WARRANTIES OF MERCHANTABILITY, *
 * FITNESS FOR A PARTICULAR  PURPOSE AND  NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF  OR IN CONNECTION  WITH THE  SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

#include "player.h"

struct vorbis_format
{
    struct file_type ft;

    OggVorbis_File vorbis_file;
    int bitstream;
};

static bool is_vorbis(FILE *fp, struct file_type **ft)
{
    long pos = ftell(fp);
    OggVorbis_File vorbis_file;
    struct vorbis_format *fmt;

    if (ov_open(fp, &vorbis_file, NULL, 0) < 0)
    {
        fseek(fp, pos, SEEK_SET);
        return false;
    }

    vorbis_info *ovi = ov_info(&vorbis_file, -1);

    fmt = calloc(sizeof(*fmt), 1);
    *ft = &fmt->ft;
    memcpy(&fmt->vorbis_file, &vorbis_file, sizeof(vorbis_file));
    fmt->bitstream = 0;
    fmt->ft.channels = ov_info(&fmt->vorbis_file, -1)->channels;
    fmt->ft.position = 0;
    fmt->ft.length = ov_time_total(&fmt->vorbis_file, -1);
    fmt->ft.sample_rate = ovi->rate;
    fmt->ft.sample_size = 2;
    fmt->ft.sample_type = ST_SIGNED_INTEGER_LE;
    fmt->ft.bitrate = (double)ov_bitrate(&fmt->vorbis_file, -1) / 1000.;

    return true;
}

static size_t decode(struct file_type *ft , size_t out_frames, void *out_buf)
{
    struct vorbis_format *fmt = (struct vorbis_format *)ft;
    int sum = 0, now = 1024;

    while (out_frames && now)
    {
        now = ov_read(&fmt->vorbis_file, out_buf, out_frames * ft->channels * ft->sample_size, 0, 2, 1, &fmt->bitstream);

        if (now < 0)
            now = 0;

        int reduced = now / (ft->channels * ft->sample_size);

        sum += reduced;
        out_buf = (uint8_t *)out_buf + now;
        out_frames -= reduced;
    }

    ft->position = (volatile int)ov_time_tell(&fmt->vorbis_file);

    return sum;
}

static const struct decoder vorbis = {
    .name = "Ogg Vorbis",
    .check_file = &is_vorbis,
    .decode = &decode
};

DEFINE_DC(&vorbis)

