#include "plutovg-private.h"
#include "plutovg-utils.h"

#include <stdio.h>
#include <assert.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "plutovg-stb-truetype.h"

static int plutovg_text_iterator_length(const void* data, plutovg_text_encoding_t encoding)
{
    int length = 0;
    switch(encoding) {
    case PLUTOVG_TEXT_ENCODING_LATIN1:
    case PLUTOVG_TEXT_ENCODING_UTF8: {
        const uint8_t* text = data;
        while(*text++)
            length++;
        break;
    } case PLUTOVG_TEXT_ENCODING_UTF16: {
        const uint16_t* text = data;
        while(*text++)
            length++;
        break;
    } case PLUTOVG_TEXT_ENCODING_UTF32: {
        const uint32_t* text = data;
        while(*text++)
            length++;
        break;
    } default:
        assert(false);
    }

    return length;
}

void plutovg_text_iterator_init(plutovg_text_iterator_t* it, const void* text, int length, plutovg_text_encoding_t encoding)
{
    if(length == -1)
        length = plutovg_text_iterator_length(text, encoding);
    it->text = text;
    it->length = length;
    it->encoding = encoding;
    it->index = 0;
}

bool plutovg_text_iterator_has_next(const plutovg_text_iterator_t* it)
{
    return it->index < it->length;
}

plutovg_codepoint_t plutovg_text_iterator_next(plutovg_text_iterator_t* it)
{
    plutovg_codepoint_t codepoint = 0;
    switch(it->encoding) {
    case PLUTOVG_TEXT_ENCODING_LATIN1: {
        const uint8_t* text = it->text;
        codepoint = text[it->index++];
        break;
    } case PLUTOVG_TEXT_ENCODING_UTF8: {
        static const uint8_t trailing[256] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
        };

        static const uint32_t offsets[6] = {
            0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080
        };

        const uint8_t* text = it->text;
        uint8_t trailing_offset = trailing[text[it->index]];
        uint32_t offset_value = offsets[trailing_offset];
        while(trailing_offset > 0 && it->index < it->length - 1) {
            codepoint += text[it->index++];
            codepoint <<= 6;
            trailing_offset--;
        }

        codepoint += text[it->index++];
        codepoint -= offset_value;
        break;
    } case PLUTOVG_TEXT_ENCODING_UTF16: {
        const uint16_t* text = it->text;
        codepoint = text[it->index++];
        if(((codepoint) & 0xfffffc00) == 0xd800) {
            if(it->index < it->length && (((codepoint) & 0xfffffc00) == 0xdc00)) {
                uint16_t trail = text[it->index++];
                codepoint = (codepoint << 10) + trail - ((0xD800u << 10) - 0x10000u + 0xDC00u);
            }
        }

        break;
    } case PLUTOVG_TEXT_ENCODING_UTF32: {
        const uint32_t* text = it->text;
        codepoint = text[it->index++];
        break;
    } default:
        assert(false);
    }

    return codepoint;
}

#if defined(_WIN32)

#include <windows.h>

typedef CRITICAL_SECTION plutovg_mutex_t;

#define plutovg_mutex_init(mutex) InitializeCriticalSection(mutex)
#define plutovg_mutex_lock(mutex) EnterCriticalSection(mutex)
#define plutovg_mutex_unlock(mutex) LeaveCriticalSection(mutex)
#define plutovg_mutex_destroy(mutex) DeleteCriticalSection(mutex)

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)

#include <threads.h>

typedef mtx_t plutovg_mutex_t;

#define plutovg_mutex_init(mutex) mtx_init(mutex, mtx_recursive)
#define plutovg_mutex_lock(mutex) mtx_lock(mutex)
#define plutovg_mutex_unlock(mutex) mtx_unlock(mutex)
#define plutovg_mutex_destroy(mutex) mtx_destroy(mutex)

#else

typedef int plutovg_mutex_t;

#define plutovg_mutex_init(mutex) ((void)(mutex))
#define plutovg_mutex_lock(mutex) ((void)(mutex))
#define plutovg_mutex_unlock(mutex) ((void)(mutex))
#define plutovg_mutex_destroy(mutex) ((void)(mutex))

#endif

typedef struct plutovg_glyph {
    plutovg_codepoint_t codepoint;
    stbtt_vertex* vertices;
    int nvertices;
    int index;
    int advance_width;
    int left_side_bearing;
    int x1;
    int y1;
    int x2;
    int y2;
    struct plutovg_glyph* next;
} plutovg_glyph_t;

typedef struct {
    plutovg_glyph_t** glyphs;
    size_t size;
    size_t capacity;
} plutovg_glyph_cache_t;

struct plutovg_font_face {
    plutovg_ref_count_t ref_count;
    int ascent;
    int descent;
    int line_gap;
    int x1;
    int y1;
    int x2;
    int y2;
    stbtt_fontinfo info;
    plutovg_mutex_t mutex;
    plutovg_glyph_cache_t cache;
    plutovg_destroy_func_t destroy_func;
    void* closure;
};

static void plutovg_glyph_cache_init(plutovg_glyph_cache_t* cache)
{
    cache->glyphs = NULL;
    cache->size = 0;
    cache->capacity = 0;
}

static void plutovg_glyph_cache_finish(plutovg_glyph_cache_t* cache, plutovg_font_face_t* face)
{
    plutovg_mutex_lock(&face->mutex);

    if(cache->glyphs) {
        for(size_t i = 0; i < cache->capacity; ++i) {
            plutovg_glyph_t* glyph = cache->glyphs[i];
            while(glyph) {
                plutovg_glyph_t* next = glyph->next;
                stbtt_FreeShape(&face->info, glyph->vertices);
                free(glyph);
                glyph = next;
            }
        }

        free(cache->glyphs);
        cache->glyphs = NULL;
        cache->capacity = 0;
        cache->size = 0;
    }

    plutovg_mutex_unlock(&face->mutex);
}

#define GLYPH_CACHE_INIT_CAPACITY 128

static plutovg_glyph_t* plutovg_glyph_cache_get(plutovg_glyph_cache_t* cache, plutovg_font_face_t* face, plutovg_codepoint_t codepoint)
{
    plutovg_mutex_lock(&face->mutex);

    if(cache->glyphs == NULL) {
        assert(cache->size == 0);
        cache->glyphs = calloc(GLYPH_CACHE_INIT_CAPACITY, sizeof(plutovg_glyph_t*));
        cache->capacity = GLYPH_CACHE_INIT_CAPACITY;
    }

    size_t index = codepoint & (cache->capacity - 1);
    plutovg_glyph_t* glyph = cache->glyphs[index];
    while(glyph && glyph->codepoint != codepoint) {
        glyph = glyph->next;
    }

    if(glyph == NULL) {
        glyph = malloc(sizeof(plutovg_glyph_t));
        glyph->codepoint = codepoint;
        glyph->index = stbtt_FindGlyphIndex(&face->info, codepoint);
        glyph->nvertices = stbtt_GetGlyphShape(&face->info, glyph->index, &glyph->vertices);
        stbtt_GetGlyphHMetrics(&face->info, glyph->index, &glyph->advance_width, &glyph->left_side_bearing);
        if(!stbtt_GetGlyphBox(&face->info, glyph->index, &glyph->x1, &glyph->y1, &glyph->x2, &glyph->y2)) {
            glyph->x1 = glyph->y1 = glyph->x2 = glyph->y2 = 0;
        }

        glyph->next = cache->glyphs[index];
        cache->glyphs[index] = glyph;
        cache->size += 1;

        if(cache->size > (cache->capacity * 3 / 4)) {
            size_t newcapacity = cache->capacity << 1;
            plutovg_glyph_t** newglyphs = calloc(newcapacity, sizeof(plutovg_glyph_t*));

            for(size_t i = 0; i < cache->capacity; ++i) {
                plutovg_glyph_t* entry = cache->glyphs[i];
                while(entry) {
                    plutovg_glyph_t* next = entry->next;
                    size_t newindex = entry->codepoint & (newcapacity - 1);
                    entry->next = newglyphs[newindex];
                    newglyphs[newindex] = entry;
                    entry = next;
                }
            }

            free(cache->glyphs);
            cache->glyphs = newglyphs;
            cache->capacity = newcapacity;
        }
    }

    plutovg_mutex_unlock(&face->mutex);
    return glyph;
}

plutovg_font_face_t* plutovg_font_face_load_from_file(const char* filename, int ttcindex)
{
    FILE* fp = fopen(filename, "rb");
    if(fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    if(length == -1L) {
        fclose(fp);
        return NULL;
    }

    void* data = malloc(length);
    if(data == NULL) {
        fclose(fp);
        return NULL;
    }

    fseek(fp, 0, SEEK_SET);
    size_t nread = fread(data, 1, length, fp);
    fclose(fp);

    if(nread != length) {
        free(data);
        return NULL;
    }

    return plutovg_font_face_load_from_data(data, length, ttcindex, free, data);
}

plutovg_font_face_t* plutovg_font_face_load_from_data(const void* data, unsigned int length, int ttcindex, plutovg_destroy_func_t destroy_func, void* closure)
{
    stbtt_fontinfo info;
    int offset = stbtt_GetFontOffsetForIndex(data, ttcindex);
    if(offset == -1 || !stbtt_InitFont(&info, data, offset)) {
        if(destroy_func)
            destroy_func(closure);
        return NULL;
    }

    plutovg_font_face_t* face = malloc(sizeof(plutovg_font_face_t));
    plutovg_init_reference(face);
    face->info = info;
    stbtt_GetFontVMetrics(&face->info, &face->ascent, &face->descent, &face->line_gap);
    stbtt_GetFontBoundingBox(&face->info, &face->x1, &face->y1, &face->x2, &face->y2);
    plutovg_mutex_init(&face->mutex);
    plutovg_glyph_cache_init(&face->cache);
    face->destroy_func = destroy_func;
    face->closure = closure;
    return face;
}

plutovg_font_face_t* plutovg_font_face_reference(plutovg_font_face_t* face)
{
    plutovg_increment_reference(face);
    return face;
}

void plutovg_font_face_destroy(plutovg_font_face_t* face)
{
    if(plutovg_destroy_reference(face)) {
        plutovg_glyph_cache_finish(&face->cache, face);
        plutovg_mutex_destroy(&face->mutex);
        if(face->destroy_func)
            face->destroy_func(face->closure);
        free(face);
    }
}

int plutovg_font_face_get_reference_count(const plutovg_font_face_t* face)
{
    return plutovg_get_reference_count(face);
}

static float plutovg_font_face_get_scale(const plutovg_font_face_t* face, float size)
{
    return stbtt_ScaleForMappingEmToPixels(&face->info, size);
}

void plutovg_font_face_get_metrics(const plutovg_font_face_t* face, float size, float* ascent, float* descent, float* line_gap, plutovg_rect_t* extents)
{
    float scale = plutovg_font_face_get_scale(face, size);
    if(ascent) *ascent = face->ascent * scale;
    if(descent) *descent = face->descent * scale;
    if(line_gap) *line_gap = face->line_gap * scale;
    if(extents) {
        extents->x = face->x1 * scale;
        extents->y = face->y2 * -scale;
        extents->w = (face->x2 - face->x1) * scale;
        extents->h = (face->y1 - face->y2) * -scale;
    }
}

static plutovg_glyph_t* plutovg_font_face_get_glyph(plutovg_font_face_t* face, plutovg_codepoint_t codepoint)
{
    return plutovg_glyph_cache_get(&face->cache, face, codepoint);
}

void plutovg_font_face_get_glyph_metrics(plutovg_font_face_t* face, float size, plutovg_codepoint_t codepoint, float* advance_width, float* left_side_bearing, plutovg_rect_t* extents)
{
    float scale = plutovg_font_face_get_scale(face, size);
    plutovg_glyph_t* glyph = plutovg_font_face_get_glyph(face, codepoint);
    if(advance_width) *advance_width = glyph->advance_width * scale;
    if(left_side_bearing) *left_side_bearing = glyph->left_side_bearing * scale;
    if(extents) {
        extents->x = glyph->x1 * scale;
        extents->y = glyph->y2 * -scale;
        extents->w = (glyph->x2 - glyph->x1) * scale;
        extents->h = (glyph->y1 - glyph->y2) * -scale;
    }
}

static void glyph_traverse_func(void* closure, plutovg_path_command_t command, const plutovg_point_t* points, int npoints)
{
    plutovg_path_t* path = (plutovg_path_t*)(closure);
    switch(command) {
    case PLUTOVG_PATH_COMMAND_MOVE_TO:
        plutovg_path_move_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_LINE_TO:
        plutovg_path_line_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_CUBIC_TO:
        plutovg_path_cubic_to(path, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case PLUTOVG_PATH_COMMAND_CLOSE:
        assert(false);
    }
}

float plutovg_font_face_get_glyph_path(plutovg_font_face_t* face, float size, float x, float y, plutovg_codepoint_t codepoint, plutovg_path_t* path)
{
    return plutovg_font_face_traverse_glyph_path(face, size, x, y, codepoint, glyph_traverse_func, path);
}

float plutovg_font_face_traverse_glyph_path(plutovg_font_face_t* face, float size, float x, float y, plutovg_codepoint_t codepoint, plutovg_path_traverse_func_t traverse_func, void* closure)
{
    float scale = plutovg_font_face_get_scale(face, size);
    plutovg_matrix_t matrix;
    plutovg_matrix_init_translate(&matrix, x, y);
    plutovg_matrix_scale(&matrix, scale, -scale);

    plutovg_point_t points[3];
    plutovg_point_t current_point = {0, 0};
    plutovg_glyph_t* glyph = plutovg_font_face_get_glyph(face, codepoint);
    for(int i = 0; i < glyph->nvertices; i++) {
        switch(glyph->vertices[i].type) {
        case STBTT_vmove:
            points[0].x = glyph->vertices[i].x;
            points[0].y = glyph->vertices[i].y;
            current_point = points[0];
            plutovg_matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_MOVE_TO, points, 1);
            break;
        case STBTT_vline:
            points[0].x = glyph->vertices[i].x;
            points[0].y = glyph->vertices[i].y;
            current_point = points[0];
            plutovg_matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_LINE_TO, points, 1);
            break;
        case STBTT_vcurve:
            points[0].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * glyph->vertices[i].x;
            points[1].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * glyph->vertices[i].y;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            plutovg_matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        case STBTT_vcubic:
            points[0].x = glyph->vertices[i].cx;
            points[0].y = glyph->vertices[i].cy;
            points[1].x = glyph->vertices[i].cx1;
            points[1].y = glyph->vertices[i].cy1;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            plutovg_matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        default:
            assert(false);
        }
    }

    return glyph->advance_width * scale;
}

float plutovg_font_face_text_extents(plutovg_font_face_t* face, float size, const void* text, int length, plutovg_text_encoding_t encoding, plutovg_rect_t* extents)
{
    plutovg_text_iterator_t it;
    plutovg_text_iterator_init(&it, text, length, encoding);
    plutovg_rect_t* text_extents = NULL;
    float total_advance_width = 0.f;
    while(plutovg_text_iterator_has_next(&it)) {
        plutovg_codepoint_t codepoint = plutovg_text_iterator_next(&it);

        float advance_width;
        if(extents == NULL) {
            plutovg_font_face_get_glyph_metrics(face, size, codepoint, &advance_width, NULL, NULL);
            total_advance_width += advance_width;
            continue;
        }

        plutovg_rect_t glyph_extents;
        plutovg_font_face_get_glyph_metrics(face, size, codepoint, &advance_width, NULL, &glyph_extents);

        glyph_extents.x += total_advance_width;
        total_advance_width += advance_width;
        if(text_extents == NULL) {
            text_extents = extents;
            *text_extents = glyph_extents;
            continue;
        }

        float x1 = plutovg_min(text_extents->x, glyph_extents.x);
        float y1 = plutovg_min(text_extents->y, glyph_extents.y);
        float x2 = plutovg_max(text_extents->x + text_extents->w, glyph_extents.x + glyph_extents.w);
        float y2 = plutovg_max(text_extents->y + text_extents->h, glyph_extents.y + glyph_extents.h);

        text_extents->x = x1;
        text_extents->y = y1;
        text_extents->w = x2 - x1;
        text_extents->h = y2 - y1;
    }

    if(extents && !text_extents) {
        extents->x = 0;
        extents->y = 0;
        extents->w = 0;
        extents->h = 0;
    }

    return total_advance_width;
}

typedef struct plutovg_font_face_entry {
    plutovg_font_face_t* face;
    char* family;
    char* filename;
    int ttcindex;
    bool bold;
    bool italic;
    struct plutovg_font_face_entry* next;
} plutovg_font_face_entry_t;

struct plutovg_font_face_cache {
    plutovg_ref_count_t ref_count;
    plutovg_mutex_t mutex;
    plutovg_font_face_entry_t** entries;
    int size;
    int capacity;
    bool is_sorted;
};

plutovg_font_face_cache_t* plutovg_font_face_cache_create(void)
{
    plutovg_font_face_cache_t* cache = malloc(sizeof(plutovg_font_face_cache_t));
    plutovg_init_reference(cache);
    plutovg_mutex_init(&cache->mutex);
    cache->entries = NULL;
    cache->size = 0;
    cache->capacity = 0;
    cache->is_sorted = false;
    return cache;
}

plutovg_font_face_cache_t* plutovg_font_face_cache_reference(plutovg_font_face_cache_t* cache)
{
    plutovg_increment_reference(cache);
    return cache;
}

void plutovg_font_face_cache_destroy(plutovg_font_face_cache_t* cache)
{
    if(plutovg_destroy_reference(cache)) {
        plutovg_mutex_lock(&cache->mutex);
        for(int i = 0; i < cache->size; ++i) {
            plutovg_font_face_entry_t* entry = cache->entries[i];
            do {
                plutovg_font_face_entry_t* next = entry->next;
                free(entry);
                entry = next;
            } while(entry);
        }

        plutovg_mutex_lock(&cache->mutex);
        plutovg_mutex_destroy(&cache->mutex);
        free(cache);
    }
}

int plutovg_font_face_cache_reference_count(const plutovg_font_face_cache_t* cache)
{
    return plutovg_get_reference_count(cache);
}

static void plutovg_font_face_cache_add_entry(plutovg_font_face_cache_t* cache, plutovg_font_face_entry_t* entry)
{
    plutovg_mutex_lock(&cache->mutex);
    for(int i = 0; i < cache->size; ++i) {
        if(strcmp(entry->family, cache->entries[i]->family) == 0) {
            entry->next = cache->entries[i];
            cache->entries[i] = entry;
            goto unlock;
        }
    }

    if(cache->size >= cache->capacity) {
        cache->capacity = cache->capacity == 0 ? 8 : cache->capacity << 2;
        cache->entries = realloc(cache->entries, cache->capacity * sizeof(plutovg_font_face_entry_t*));
    }

    entry->next = NULL;
    cache->entries[cache->size++] = entry;
    cache->is_sorted = false;

unlock:
    plutovg_mutex_unlock(&cache->mutex);
}

void plutovg_font_face_cache_add(plutovg_font_face_cache_t* cache, const char* family, bool bold, bool italic, plutovg_font_face_t* face)
{
    if(family == NULL) family = "";
    size_t family_length = strlen(family) + 1;

    plutovg_font_face_entry_t* entry = malloc(family_length + sizeof(plutovg_font_face_entry_t));
    entry->face = plutovg_font_face_reference(face);
    entry->family = (char*)(entry + 1);
    memcpy(entry->family, family, family_length);

    entry->filename = NULL;
    entry->ttcindex = 0;
    entry->bold = bold;
    entry->italic = italic;

    plutovg_font_face_cache_add_entry(cache, entry);
}

static plutovg_font_face_entry_t* plutovg_font_face_entry_select(plutovg_font_face_entry_t* a, plutovg_font_face_entry_t* b, bool bold, bool italic)
{
    int a_score = (bold == a->bold) + (italic == a->italic);
    int b_score = (bold == b->bold) + (italic == b->italic);
    return a_score > b_score ? a : b;
}

static int plutovg_font_face_entry_compare(const void* a, const void* b)
{
    const plutovg_font_face_entry_t* a_entry = *(const plutovg_font_face_entry_t**)a;
    const plutovg_font_face_entry_t* b_entry = *(const plutovg_font_face_entry_t**)b;
    return strcmp(a_entry->family, b_entry->family);
}

plutovg_font_face_t* plutovg_font_face_cache_get(plutovg_font_face_cache_t* cache, const char* family, bool bold, bool italic)
{
    plutovg_mutex_lock(&cache->mutex);

    if(!cache->is_sorted) {
        qsort(cache->entries, cache->size, sizeof(cache->entries[0]), plutovg_font_face_entry_compare);
        cache->is_sorted = true;
    }

    plutovg_font_face_entry_t entry_key;
    entry_key.family = (char*)(family);

    plutovg_font_face_entry_t* entry_key_ptr = &entry_key;
    plutovg_font_face_entry_t** entry_result = bsearch(
        &entry_key_ptr,
        cache->entries,
        cache->size,
        sizeof(cache->entries[0]),
        plutovg_font_face_entry_compare
    );

    plutovg_font_face_t* face = NULL;
    if(entry_result) {
        plutovg_font_face_entry_t* selected = *entry_result;
        plutovg_font_face_entry_t* entry = selected->next;
        while(entry) {
            selected = plutovg_font_face_entry_select(selected, entry, bold, italic);
            entry = entry->next;
        }

        if(selected->filename && selected->face == NULL)
            selected->face = plutovg_font_face_load_from_file(selected->filename, selected->ttcindex);
        face = selected->face;
    }

    plutovg_mutex_unlock(&cache->mutex);
    return face;
}

#ifdef PLUTOVG_ENABLE_FONT_CACHE_LOAD

#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32

static void* plutovg_mmap(const char* filename, long* length)
{
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return NULL;
    DWORD size = GetFileSize(file, NULL);
    if(size == INVALID_FILE_SIZE) {
        CloseHandle(file);
        return NULL;
    }

    HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if(mapping == NULL) {
        CloseHandle(file);
        return NULL;
    }

    void* data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    CloseHandle(file);

    if(data == NULL)
        return NULL;
    *length = size;
    return data;
}

static void plutovg_unmap(void* data, long length)
{
    UnmapViewOfFile(data);
}

#else

static void* plutovg_mmap(const char* filename, long* length)
{
    int fd = open(filename, O_RDONLY);
    if(fd < 0)
        return NULL;
    struct stat st;
    if(fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }

    if(st.st_size == 0) {
        close(fd);
        return NULL;
    }

    void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(data == MAP_FAILED)
        return NULL;
    *length = st.st_size;
    return data;
}

static void plutovg_unmap(void* data, long length)
{
    munmap(data, length);
}

#endif // _WIN32

int plutovg_font_face_cache_load_file(plutovg_font_face_cache_t* cache, const char* filename)
{
    long length;
    uint8_t* data = plutovg_mmap(filename, &length);
    if(data == NULL) {
        printf("Unable to open file: %s", filename);
        return 0;
    }

    int num_faces = 0;

    int num_fonts = stbtt_GetNumberOfFonts(data);
    for(int index = 0; index < num_fonts; ++index) {
        int offset = stbtt_GetFontOffsetForIndex(data, index);
        if(offset == -1 || !stbtt__isfont(data + offset)) {
            continue;
        }

        stbtt_uint32 nm = stbtt__find_table(data, offset, "name");
        stbtt_int32 nm_count = ttUSHORT(data + nm + 2);

        const uint8_t* family_name = NULL;
        size_t family_length = 0;
        for(stbtt_int32 i = 0; i < nm_count; ++i) {
            stbtt_uint32 loc = nm + 6 + 12 * i;
            if(ttUSHORT(data + loc + 6) != 1) {
                continue;
            }

            stbtt_int32 platform = ttUSHORT(data + loc + 0);
            stbtt_int32 encoding = ttUSHORT(data + loc + 2);
            if(platform == 0 || (platform == 3 && encoding == 1) || (platform == 3 && encoding == 10)) {
                family_name = data + nm + ttUSHORT(data + nm + 4) + ttUSHORT(data + loc + 10);
                family_length = ttUSHORT(data + loc + 8);
                break;
            }
        }

        if(family_length == 0)
            continue;
        size_t name_length = family_length / 2 + 1;
        size_t filename_length = strlen(filename) + 1;

        plutovg_font_face_entry_t* entry = malloc(name_length + filename_length + sizeof(plutovg_font_face_entry_t));
        entry->family = (char*)(entry + 1);
        entry->filename = entry->family + name_length;
        memcpy(entry->filename, filename, filename_length);

        int family_index = 0;
        while(family_length) {
            entry->family[family_index++] = family_name[0] * 256 + family_name[1];
            family_name += 2;
            family_length -= 2;
        }

        entry->family[family_index] = 0;

        entry->face = NULL;
        entry->bold = false;
        entry->italic = false;
        entry->ttcindex = index;

        stbtt_uint32 hd = stbtt__find_table(data, offset, "head");
        stbtt_uint16 style = ttUSHORT(data + hd + 44);
        if(style & 0x1)
            entry->bold = true;
        if(style & 0x2) {
            entry->italic = true;
        }

        plutovg_font_face_cache_add_entry(cache, entry);
        num_faces++;
    }

    plutovg_unmap(data, length);
    return num_faces;
}

static bool plutovg_font_face_supports_file(const char* filename)
{
    const char* extension = strrchr(filename, '.');
    if(extension) {
        char ext[4];
        size_t length = strlen(extension);
        if(length <= sizeof(ext)) {
            for(size_t i = 0; i < length; ++i)
                ext[i] = tolower(extension[i]);
            return strcmp(ext, ".ttf") == 0
                || strcmp(ext, ".otf") == 0
                || strcmp(ext, ".ttc") == 0
                || strcmp(ext, ".otc") == 0;
        }
    }

    return false;
}

#ifdef _WIN32

int plutovg_font_face_cache_load_dir(plutovg_font_face_cache_t* cache, const char* dirname)
{
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dirname);

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(search_path, &find_data);
    if(handle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int num_faces = 0;

    do {
        const char* name = find_data.cFileName;
        if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        char path[MAX_PATH * 2];
        snprintf(path, sizeof(path), "%s\\%s", dirname, name);

        if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            num_faces += plutovg_font_face_cache_load_dir(cache, path);
        } else if(plutovg_font_face_supports_file(path)) {
            num_faces += plutovg_font_face_cache_load_file(cache, path);
        }
    } while(FindNextFileA(handle, &find_data));

    FindClose(handle);
    return num_faces;
}

#else

int plutovg_font_face_cache_load_dir(plutovg_font_face_cache_t* cache, const char* dirname)
{
    DIR* dir = opendir(dirname);
    if(dir == NULL) {
        printf("Unable to open dir: %s", dirname);
        return 0;
    }

    int num_faces = 0;

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat st;
        if(stat(path, &st) == -1)
            continue;
        if(S_ISDIR(st.st_mode)) {
            num_faces += plutovg_font_face_cache_load_dir(cache, path);
        } else if(S_ISREG(st.st_mode) && plutovg_font_face_supports_file(path)) {
            num_faces += plutovg_font_face_cache_load_file(cache, path);
        }
    }

    closedir(dir);
    return num_faces;
}

#endif // _WIN32

int plutovg_font_face_cache_load_sys(plutovg_font_face_cache_t* cache)
{
    int num_faces = 0;
#if defined(_WIN32)
    num_faces += plutovg_font_face_cache_load_dir(cache, "C:\\Windows\\Fonts\\");
#elif defined(__APPLE__)
    num_faces += plutovg_font_face_cache_load_dir(cache, "/Library/Fonts");
    num_faces += plutovg_font_face_cache_load_dir(cache, "/System/Library/Fonts");
#elif defined(__linux__)
    num_faces += plutovg_font_face_cache_load_dir(cache, "/usr/share/fonts/");
    num_faces += plutovg_font_face_cache_load_dir(cache, "/usr/local/share/fonts/");
#endif
    return num_faces;
}

#else

int plutovg_font_face_cache_load_file(plutovg_font_face_cache_t* cache, const char* filename)
{
    return -1;
}

int plutovg_font_face_cache_load_dir(plutovg_font_face_cache_t* cache, const char* dirname)
{
    return -1;
}

int plutovg_font_face_cache_load_sys(plutovg_font_face_cache_t* cache)
{
    return -1;
}

#endif // PLUTOVG_ENABLE_FONT_CACHE_LOAD
