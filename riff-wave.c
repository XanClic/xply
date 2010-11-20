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
 ******************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "player.h"


struct wave_header
{
    char fmt_chunk[4];
    uint32_t fmt_chunk_len;
    uint16_t tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample;
} __attribute__ ((packed));

struct wave_format
{
    struct file_type ft;

    size_t size;
    size_t pos;
    int bytes_per_second;
    FILE *fp;
};


static bool is_wave(FILE *fp, struct file_type **ft)
{
    char chunk_name[4];
    struct wave_header wav_fmt;
    int i;
    long pos;
    size_t ifsize;
    struct wave_format *fmt;

    pos = ftell(fp);

    fread(chunk_name, 1, 4, fp);
    if (!strncmp(chunk_name, "RIFF", 4))
    {
        fseek(fp, 4, SEEK_CUR);
        fread(chunk_name, 1, 4, fp);
        if (!strncmp(chunk_name, "WAVE", 4))
        {
            fread(&wav_fmt, 1, sizeof(wav_fmt), fp);
            for (i = 0; i < 4; i++)
                wav_fmt.fmt_chunk[i] = toupper(wav_fmt.fmt_chunk[i]);

            if (strncmp(wav_fmt.fmt_chunk, "FMT ", 4) || (wav_fmt.fmt_chunk_len != 16))
            {
                fseek(fp, pos, SEEK_SET);
                return false;
            }

            if ((wav_fmt.tag != 1) ||
                ((wav_fmt.channels != 1) &&
                 (wav_fmt.channels != 2)) ||
                ((wav_fmt.bits_per_sample != 8) &&
                 (wav_fmt.bits_per_sample != 16)))
            {
                fseek(fp, pos, SEEK_SET);
                return false;
            }

            do
            {
                fread(chunk_name, 1, 4, fp);
                fread(&ifsize, 1, 4, fp);
                if (strncmp(chunk_name, "data", 4))
                    fseek(fp, ifsize, SEEK_CUR);
            }
            while (strncmp(chunk_name, "data", 4) && !feof(fp));

            if (feof(fp))
            {
                fseek(fp, pos, SEEK_SET);
                return false;
            }
        }
        else
        {
            fseek(fp, pos, SEEK_SET);
            return false;
        }
    }
    else
    {
        fseek(fp, pos, SEEK_SET);
        return false;
    }

    fmt = calloc(sizeof(*fmt), 1);
    *ft = &fmt->ft;
    fmt->ft.channels = wav_fmt.channels;
    fmt->ft.position = 0;
    fmt->ft.length = ifsize / wav_fmt.bytes_per_second;
    fmt->ft.sample_rate = wav_fmt.sample_rate;
    fmt->ft.sample_size = wav_fmt.bits_per_sample / 8;
    fmt->ft.sample_type = ST_SIGNED_INTEGER_LE;
    fmt->ft.bitrate = (double)wav_fmt.bytes_per_second / 250.;
    fmt->size = ifsize;
    fmt->pos = 0;
    fmt->bytes_per_second = wav_fmt.bytes_per_second;
    fmt->fp = fp;

    return true;
}

static size_t decode(struct file_type *ft, size_t out_frames, void *out_buf)
{
    struct wave_format *fmt = (struct wave_format *)ft;
    size_t now;

    now = fmt->size / (ft->channels * ft->sample_size);
    if (now > out_frames)
        now = out_frames;

    fread(out_buf, ft->channels * ft->sample_size, now, fmt->fp);
    fmt->size -= now * ft->channels * ft->sample_size;
    fmt->pos  += now * ft->channels * ft->sample_size;

    ft->position = fmt->pos / fmt->bytes_per_second;

    return now;
}

static const struct decoder wave = {
    .name = "RIFF WAVE",
    .check_file = &is_wave,
    .decode = &decode
};

DEFINE_DC(&wave)

