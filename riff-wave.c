/******************************************************************************
 * Copyright (c) 2009 Hanna Reitz                                             *
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
                 (wav_fmt.bits_per_sample != 16) &&
                 (wav_fmt.bits_per_sample != 24) &&
                 (wav_fmt.bits_per_sample != 32)))
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
    fmt->ft.sample_rate = (double)wav_fmt.sample_rate / 1000.;
    fmt->ft.sample_size = wav_fmt.bits_per_sample / 8;
    fmt->ft.sample_type = ST_SIGNED_INTEGER_LE;
    fmt->size = ifsize;
    fmt->pos = 0;
    fmt->bytes_per_second = wav_fmt.bytes_per_second;

    return true;
}

#if 0
static size_t wave2raw(FILE* fp, void* format_inf, size_t out_frames, void* out_buf)
{
    struct wave_format* fmt = format_inf;
    size_t now;
    void* tbuf;
#ifndef USE_INLINE_ASM
    size_t count;
    uint8_t* i8buf;
    uint16_t* ibuf;
    uint32_t* i32buf;
    uint16_t* obuf;
#endif

    //TODO Resamplen

    now = fmt->size / (fmt->channels * (fmt->bits_per_sample >> 3));
    if (now > out_frames) {
        now = out_frames;
    }

    if ((fmt->channels == 2) && (fmt->bits_per_sample == 16)) {
        fread(out_buf, 4, now, fp);
        fmt->size -= now * 4;
        fmt->pos += now * 4;
    } else if ((fmt->channels == 1) && (fmt->bits_per_sample == 16)) {
        tbuf = calloc(now, 2);
        fread(tbuf, 2, now, fp);
        fmt->size -= now * 2;
        fmt->pos += now * 2;

#ifdef USE_INLINE_ASM
        __asm__ __volatile__ (".blow_up_to_two:"
                "lodsw;"
                "stosw;"
                "stosw;"
                "loop .blow_up_to_two"::"c"(now),"S"(tbuf),"D"(out_buf));
#else
        count = now;
        ibuf = tbuf;
        obuf = out_buf;
        while (count--) {
            *(obuf++) = *ibuf;
            *(obuf++) = *(ibuf++);
        }
#endif

        free(tbuf);
    } else if (fmt->channels == 2) {
        tbuf = calloc(now, fmt->bits_per_sample >> 2);
        fread(tbuf, fmt->bits_per_sample >> 2, now, fp);
        fmt->size -= now * (fmt->bits_per_sample >> 2);
        fmt->pos += now * (fmt->bits_per_sample >> 2);
        switch (fmt->bits_per_sample)
        {
            case 8:
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_2_8:"
                        "xor %%eax,%%eax;"
                        "lodsb;"
                        "shl $7,%%eax;" //Bitte *nicht* fragen, warum nicht $8
                        "stosw;"
                        "loop .from_2_8"::"c"(now * 2),"S"(tbuf),"D"(out_buf));
#else
                count = now * 2;
                i8buf = tbuf;
                obuf = out_buf;
                while (count--) {
                    *(obuf++) = *(i8buf++) << 7;
                }
#endif
                break;
            case 24:
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_2_24:"
                        "lodsb;"
                        "lodsw;"
                        "stosw;"
                        "loop .from_2_24"
                        ::"c"(now * 2),"S"(tbuf),"D"(out_buf));
#else
                count = now * 2;
                i8buf = tbuf;
                obuf = out_buf;
                i8buf -= 2;
                while (count--) {
                    i8buf += 3;
                    *(obuf++) = i8buf[0] | (i8buf[1] << 8);
                }
#endif
                break;
            case 32:
                //Gibts eigentlich gar nicht. Aber warum nicht? Dann ist das
                //eben der einzige Player mit RIFF-WAVE-PCM-32-Bit-Unter-
                //stützung. Wow.
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_2_32:"
                        "lodsl;"
                        "shr $16,%%eax;"
                        "stosw;"
                        "loop .from_2_32"
                        ::"c"(now * 2),"S"(tbuf),"D"(out_buf));
#else
                count = now * 2;
                i32buf = tbuf;
                obuf = out_buf;
                while (count--) {
                    *(obuf++) = *(i32buf++) >> 16;
                }
#endif
                break;
        }
        free(tbuf);
    }
    else if (fmt->channels == 1)
    {
        tbuf = calloc(now, fmt->bits_per_sample >> 3);
        fread(tbuf, fmt->bits_per_sample >> 3, now, fp);
        fmt->size -= now * (fmt->bits_per_sample >> 3);
        fmt->pos += now * (fmt->bits_per_sample >> 3);
        switch (fmt->bits_per_sample)
        {
            case 8:
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_1_8:"
                        "xor %%eax,%%eax;"
                        "lodsb;"
                        "shl $7,%%eax;" //Bitte *nicht* fragen, warum nicht $8
                        "stosw;"
                        "stosw;"
                        "loop .from_1_8"::"c"(now),"S"(tbuf),"D"(out_buf));
#else
                count = now;
                i8buf = tbuf;
                obuf = out_buf;
                while (count--) {
                    *(obuf++) = *(i8buf) << 7;
                    *(obuf++) = *(i8buf++) << 7;
                }
#endif
                break;
            case 24:
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_1_24:"
                        "lodsb;"
                        "lodsw;"
                        "stosw;"
                        "stosw;"
                        "loop .from_1_24"::"c"(now),"S"(tbuf),"D"(out_buf));
#else
                count = now;
                i8buf = tbuf;
                obuf = out_buf;
                i8buf -= 2;
                while (count--) {
                    i8buf += 3;
                    *(obuf++) = i8buf[0] | (i8buf[1] << 8);
                    *(obuf++) = i8buf[0] | (i8buf[1] << 8);
                }
#endif
                break;
            case 32:
#ifdef USE_INLINE_ASM
                __asm__ __volatile__ (".from_1_32:"
                        "lodsl;"
                        "shr $16,%%eax;"
                        "stosw;"
                        "stosw;"
                        "loop .from_1_32"::"c"(now),"S"(tbuf),"D"(out_buf));
#else
                count = now;
                i32buf = tbuf;
                obuf = out_buf;
                while (count--) {
                    *(obuf++) = *i32buf >> 16;
                    *(obuf++) = *(i32buf++) >> 16;
                }
#endif
                break;
        }
        free(tbuf);
    }

    fmt->gen.position = fmt->pos / fmt->bytes_per_second;

    return now;
}
#endif

static const struct decoder wave = {
    .name = "RIFF WAVE",
    .check_file = &is_wave
};

DEFINE_DC(&wave)

