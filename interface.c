#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "player.h"

extern const void __start_ao, __stop_ao;
extern const void __start_dc, __stop_dc;

const struct output_device *find_output_device(const char *name)
{
    const struct output_device **ao = (const struct output_device **)&__start_ao;

    while ((uintptr_t)ao < (uintptr_t)&__stop_ao)
    {
        if (!strcmp((*ao)->name, name))
            return *ao;

        ao++;
    }

    return NULL;
}

struct file_type *get_file_type(FILE *fp)
{
    const struct decoder **dc = (const struct decoder **)&__start_dc;

    struct file_type *ft = NULL;

    while ((uintptr_t)dc < (uintptr_t)&__stop_dc)
    {
        if ((*dc)->check_file(fp, &ft))
        {
            ft->decoder = *dc;
            return ft;
        }

        dc++;
    }

    return NULL;
}

