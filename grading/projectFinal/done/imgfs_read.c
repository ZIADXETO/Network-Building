#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "image_content.h"
#include "image_dedup.h"
#include "util.h"   // for _unused
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int do_read(const char* img_id, int resolution, char** image_buffer, uint32_t* image_size, struct imgfs_file* imgfs_file)
{
    // Verify that none of the pointers are NULL.
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);
    M_REQUIRE_NON_NULL(image_size);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Search for the image by its ID within the file system's metadata entries.
    int found_index = -1;
    int i = 0;
    while ((uint32_t) i < imgfs_file->header.max_files && found_index == -1) {
        if (strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            found_index = i;
        }
        i++;
    }

    // If no entry is found, return an error indicating the image is not found.
    if (found_index == -1) {
        return ERR_IMAGE_NOT_FOUND;
    }

    // Get a pointer to the metadata for the found image.
    struct img_metadata *metadata = &imgfs_file->metadata[found_index];

    // Check if the requested resolution is available; if not, perform a lazy resize.
    if (metadata->size[resolution] == 0) {
        int err = lazily_resize(resolution, imgfs_file, (size_t) found_index);
        if (err != ERR_NONE) return err;
    }

    // Retrieve the size and offset from the metadata to read the image.
    uint32_t size = metadata->size[resolution];
    uint64_t offset = metadata->offset[resolution];

    // Allocate memory for the image buffer.
    *image_buffer = (char*)calloc(1, size);
    if (*image_buffer == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    // Set the file pointer to the offset and read the image data into the buffer.
    if (fseek(imgfs_file->file, (long) offset, SEEK_SET) != 0 || fread(*image_buffer, 1, size, imgfs_file->file) != size) {
        free(*image_buffer); // Free memory if read fails.
        *image_buffer = NULL; // Nullify pointer to avoid dangling pointer usage.
        return ERR_IO; // Return I/O error if file operations fail.
    }

    // Store the size of the read image data in the provided pointer.
    *image_size = size;
    return ERR_NONE; // Return success.
}

