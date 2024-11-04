#include <plutovg.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#include <intrin.h>
static inline int clz(unsigned int x) {
    unsigned long r = 0;
    if (_BitScanReverse(&r, x))
        return 31 - r;
    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#elif defined(__GNUC__)
#define PVG_FT_MSB(x)  (31 - __builtin_clz(x))
#else
static inline int clz(unsigned int x) {
    for (int i = 31; i >= 0; i--)
    {
        if (x >> i)
        {
            return 31 - i;
        }
    }

    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#endif

int main(void)
{
#if defined(_WIN32)
    printf("_WIN32\n");
#endif
#if defined(_MSC_VER)
    printf("_MSC_VER\n");
#endif
#if defined(__APPLE__)
    printf("__APPLE__\n");
#endif
#if defined(__clang__)
    printf("__clang__\n");
#endif
#if defined(__GNUC__)
    printf("__GNUC__\n");
#endif
    char buffer[256];
    FILE *fp;

    // Open a pipe to run fc-list and read its output
    fp = popen("fc-list : family", "r");
    if (fp == NULL) {
        perror("popen failed");
        return 1;
    }

    // Read and print each line from fc-list
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s", buffer);
    }

    // Close the pipe
    pclose(fp);

    return 0;
    
    printf("%d\n", PVG_FT_MSB(0));
    printf("%d\n", PVG_FT_MSB(10));
    printf("%d\n", PVG_FT_MSB(100));
    printf("%d\n", PVG_FT_MSB(1000));
    
    const int width = 150;
    const int height = 150;

    plutovg_surface_t* surface = plutovg_surface_create(width, height);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    
    float center_x = width / 2.f;
    float center_y = height / 2.f;
    float face_radius = 70;
    float eye_radius = 10;
    float mouth_radius = 50;
    float eye_offset_x = 25;
    float eye_offset_y = 20;
    float eye_x = center_x - eye_offset_x;
    float eye_y = center_y - eye_offset_y;

    plutovg_canvas_save(canvas);
    plutovg_canvas_arc(canvas, center_x, center_y, face_radius, 0, 2 * PLUTOVG_PI, 0);
    plutovg_canvas_set_rgb(canvas, 1, 1, 0);
    plutovg_canvas_fill_preserve(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 5);
    plutovg_canvas_stroke(canvas);
    plutovg_canvas_restore(canvas);

    plutovg_canvas_save(canvas);
    plutovg_canvas_arc(canvas, eye_x, eye_y, eye_radius, 0, 2 * PLUTOVG_PI, 0);
    plutovg_canvas_arc(canvas, center_x + eye_offset_x, eye_y, eye_radius, 0, 2 * PLUTOVG_PI, 0);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_fill(canvas);
    plutovg_canvas_restore(canvas);

    plutovg_canvas_save(canvas);
    plutovg_canvas_arc(canvas, center_x, center_y, mouth_radius, 0, PLUTOVG_PI, 0);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 5);
    plutovg_canvas_stroke(canvas);
    plutovg_canvas_restore(canvas);

    plutovg_surface_write_to_png(surface, "smiley.png");
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
    return 0;
}
