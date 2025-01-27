#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <vips/vips.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index)
{

    M_REQUIRE_NON_NULL(imgfs_file);
    if (index < 0 ) return ERR_INVALID_ARGUMENT;
    if (index >= imgfs_file->header.nb_files) return ERR_INVALID_IMGID;
    if (resolution != THUMB_RES && resolution != SMALL_RES && resolution != ORIG_RES) {
        return ERR_INVALID_IMGID;
    }

    struct img_metadata* metadata = &imgfs_file->metadata[index];

    if (metadata->size[resolution] != 0) return ERR_NONE;

    if (fseek(imgfs_file->file,(long) metadata->offset[ORIG_RES], SEEK_SET) != 0) return ERR_IO;

    void *orig_buf = calloc(1, metadata->size[ORIG_RES]);
    if (orig_buf == NULL) return ERR_OUT_OF_MEMORY;

    if (fread(orig_buf, metadata->size[ORIG_RES], 1, imgfs_file->file) != 1) {
        free(orig_buf);
        orig_buf = NULL;
        return ERR_IO;
    }

    VipsImage *in_image, *out_image;
    if (vips_jpegload_buffer(orig_buf, metadata->size[ORIG_RES], &in_image, NULL) != 0) {
        free(orig_buf);
        orig_buf = NULL;
        return ERR_DEBUG;
    }

    int target_width = imgfs_file->header.resized_res[(resolution) * 2];
    int target_height = imgfs_file->header.resized_res[(resolution) * 2 + 1];

    if (vips_thumbnail_image(in_image, &out_image, target_width, "height", target_height, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(in_image));
        free(orig_buf);
        in_image = NULL;
        orig_buf = NULL;
        return ERR_DEBUG;
    }

    g_object_unref(VIPS_OBJECT(in_image));
    in_image = NULL;

    void *buf = NULL;
    size_t len;

    if (vips_jpegsave_buffer(out_image, &buf, &len, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(out_image));
        free(orig_buf);
        out_image = NULL;
        orig_buf = NULL;
        return ERR_DEBUG;
    }

    g_object_unref(VIPS_OBJECT(out_image));
    out_image = NULL;

    if (fseek(imgfs_file->file, 0, SEEK_END)) {
        g_free(buf);
        buf = NULL;
        return ERR_IO;
    }
    if (fwrite(buf, len, 1, imgfs_file->file)!=1) {
        g_free(buf);
        buf = NULL;
        return ERR_IO;
    }

    metadata->offset[resolution] = ((unsigned long) ftell(imgfs_file->file)) - len;
    metadata->size[resolution] =(uint32_t) len;

    g_free(buf);
    buf = NULL;

    if (fseek(imgfs_file->file,(long) (sizeof(struct imgfs_header) + sizeof(struct img_metadata)*index), SEEK_SET)) return ERR_IO;
    if (fwrite(metadata, sizeof(struct img_metadata), 1, imgfs_file->file)!=1) return ERR_IO;

    free(orig_buf);
    orig_buf = NULL;

    imgfs_file->metadata = metadata;

    return ERR_NONE;
}
