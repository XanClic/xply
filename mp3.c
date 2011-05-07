/******************************************************************************
 * Copyright (c) 2011 Hanna Reitz                                             *
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

#include <mpg123.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "player.h"

struct mp3_format
{
    struct file_type ft;

    mpg123_handle *mh;
    double tpf;
};

static bool is_mp3(FILE *fp, struct file_type **ft)
{
    mpg123_init();

    int err;
    mpg123_handle *mh = mpg123_new(NULL, &err);

    if (mh == NULL)
    {
        fprintf(stderr, "Could not create mpg123 handle: %s\n", mpg123_plain_strerror(err));
        return false;
    }

    long pos = ftell(fp);

    if (mpg123_open_fd(mh, fileno(fp)) != MPG123_OK)
    {
        fseek(fp, pos, SEEK_SET);
        return false;
    }

    mpg123_scan(mh);

    struct mpg123_frameinfo fi;
    mpg123_info(mh, &fi);

    if (mpg123_format(mh, fi.rate, MPG123_STEREO, MPG123_ENC_SIGNED_16) != MPG123_OK)
    {
        fseek(fp, pos, SEEK_SET);
        return false;
    }

    off_t length = mpg123_length(mh);

    struct mp3_format *fmt = calloc(sizeof(*fmt), 1);
    *ft = &fmt->ft;
    fmt->mh = mh;
    fmt->tpf = mpg123_tpf(mh);
    fmt->ft.channels = 2;
    fmt->ft.position = 0;
    fmt->ft.length = (length + (fi.rate / 2)) / fi.rate;
    fmt->ft.sample_rate = fi.rate;
    fmt->ft.sample_size = 2;
    fmt->ft.sample_type = ST_SIGNED_INTEGER_LE;
    fmt->ft.bitrate = (double)fi.abr_rate;

    return true;
}

static size_t dec_mp3(struct file_type *ft, size_t out_frames, void *out_buf)
{
    size_t cdone = 0;
    struct mp3_format *mp3 = (struct mp3_format *)ft;
    int ret;

    do
    {
        size_t bdone;
        ret = mpg123_read(mp3->mh, out_buf, (out_frames - cdone) * 4, &bdone);
        cdone += bdone / 4;
    }
    while ((cdone < out_frames) && (ret != MPG123_DONE) && (ret <= 0));

    ft->position = (volatile int)(mpg123_tellframe(mp3->mh) * mp3->tpf);

    return cdone;
}

static const struct decoder mp3 = {
    .name = "MP3",
    .check_file = &is_mp3,
    .decode = &dec_mp3
};

DEFINE_DC(&mp3)

