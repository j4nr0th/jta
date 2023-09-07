//
// Created by jan on 24.7.2023.
//

#ifndef JTA_COLOR_MAP_H
#define JTA_COLOR_MAP_H
#include "../common/common.h"
#include "../core/jtaerr.h"


struct jta_scalar_cmap_struct
{
    uint32_t count;         //  Number of value-color pairs
    float* values;          //  Values corresponding to colors. values[0] = 0.0f and values[count - 1] = 1.0f
    vec4* colors; //  Color corresponding to values in the colormap
};

typedef struct jta_scalar_cmap_struct jta_scalar_cmap;

jta_result jta_scalar_cmap_from_csv(const jio_context* io_ctx, const char* filename, jta_scalar_cmap* out_cmap);

void jta_scalar_cmap_free(jta_scalar_cmap* cmap);

jta_color jta_scalar_cmap_color(const jta_scalar_cmap* cmap, float v);

#endif //JTA_COLOR_MAP_H
