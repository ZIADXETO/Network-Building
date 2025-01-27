#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <vips/vips.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int lazily_resize(int resolution, struct imgfs_file* imgfs_file, size_t index)
{
    // Check for null pointers to avoid dereferencing null.
    M_REQUIRE_NON_NULL(imgfs_file);

    // Validate the image index and its validity within the filesystem metadata array.
    if (!(index < imgfs_file->header.max_files && imgfs_file->metadata[index].is_valid))
        return ERR_INVALID_IMGID;

    // Check if the requested resolution is within the valid range.
    if (resolution < 0 || resolution >= NB_RES) {
        return ERR_INVALID_IMGID;
    }

    // Get a pointer to the metadata of the image at the specified index.
    struct img_metadata* metadata = &imgfs_file->metadata[index];

    // Check if the image metadata indicates the image is valid and non-empty.
    if (metadata->is_valid != NON_EMPTY) return ERR_INVALID_IMGID;

    // If the image is already resized at this resolution, exit function successfully.
    if (metadata->size[resolution] != 0) return ERR_NONE;

    // Seek to the position of the original resolution image in the filesystem.
    if (fseek(imgfs_file->file, (long) metadata->offset[ORIG_RES], SEEK_SET) != 0)
        return ERR_IO;

    // Allocate memory for reading the original image.
    void *orig_buf = calloc(1, metadata->size[ORIG_RES]);
    if (orig_buf == NULL) return ERR_OUT_OF_MEMORY;

    // Read the original image data from the file.
    if (fread(orig_buf, metadata->size[ORIG_RES], 1, imgfs_file->file) != 1) {
        free(orig_buf);
        return ERR_IO;
    }

    // Load the image from the buffer using the VIPS library.
    VipsImage *in_image, *out_image;
    if (vips_jpegload_buffer(orig_buf, metadata->size[ORIG_RES], &in_image, NULL) != 0) {
        free(orig_buf);
        return ERR_IMGLIB;
    }

    // Determine the target dimensions for the resized image.
    int target_width = imgfs_file->header.resized_res[resolution * 2];
    int target_height = imgfs_file->header.resized_res[resolution * 2 + 1];

    // Create a thumbnail image of the target resolution.
    if (vips_thumbnail_image(in_image, &out_image, target_width, "height", target_height, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(in_image));
        free(orig_buf);
        return ERR_IMGLIB;
    }

    // Unreference the original VIPS image object as it is no longer needed.
    g_object_unref(VIPS_OBJECT(in_image));

    // Save the resized image to a new buffer.
    void *buf = NULL;
    size_t len;
    if (vips_jpegsave_buffer(out_image, &buf, &len, NULL) != 0) {
        g_object_unref(VIPS_OBJECT(out_image));
        free(orig_buf);
        return ERR_IMGLIB;
    }

    // Clean up the resized image VIPS object.
    g_object_unref(VIPS_OBJECT(out_image));

    // Seek to the end of the file to append the resized image.
    if (fseek(imgfs_file->file, 0, SEEK_END) != 0) {
        g_free(buf);
        return ERR_IO;
    }

    // Write the resized image buffer to the file.
    if (fwrite(buf, len, 1, imgfs_file->file) != 1) {
        g_free(buf);
        return ERR_IO;
    }

    // Update the metadata with the new offset and size of the resized image.
    metadata->offset[resolution] = ((unsigned long) ftell(imgfs_file->file)) - len;
    metadata->size[resolution] = (uint32_t) len;

    // Free the buffer containing the resized image.
    g_free(buf);

    // Seek to the metadata position in the file and write the updated metadata.
    if (fseek(imgfs_file->file, (long) (sizeof(struct imgfs_header) + sizeof(struct img_metadata) * index), SEEK_SET) != 0)
        return ERR_IO;
    if (fwrite(metadata, sizeof(struct img_metadata), 1, imgfs_file->file) != 1)
        return ERR_IO;

    // Clean up the original image buffer.
    free(orig_buf);

    // Return success.
    return ERR_NONE;
}

int get_resolution(uint32_t *height, uint32_t *width,
                   const char *image_buffer, size_t image_size)
{
    M_REQUIRE_NON_NULL(height);
    M_REQUIRE_NON_NULL(width);
    M_REQUIRE_NON_NULL(image_buffer);

    VipsImage* original = NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const int err = vips_jpegload_buffer((void*) image_buffer, image_size,
                                         &original, NULL);
#pragma GCC diagnostic pop
    if (err != ERR_NONE) return ERR_IMGLIB;

    *height = (uint32_t) vips_image_get_height(original);
    *width  = (uint32_t) vips_image_get_width (original);

    g_object_unref(VIPS_OBJECT(original));
    return ERR_NONE;
}

