#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "image_content.h"
#include "image_dedup.h"
#include "util.h"   // for _unused
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int do_insert(const char* image_buffer, size_t image_size, const char* img_id, struct imgfs_file* imgfs_file)
{
    // Ensure that none of the required parameters are NULL.
    M_REQUIRE_NON_NULL(imgfs_file);
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(image_buffer);

    // Check if the file system has reached its maximum capacity for stored files.
    if (imgfs_file->header.nb_files >= imgfs_file->header.max_files) return ERR_IMGFS_FULL;

    // Look for a free index where the new image metadata can be stored.
    uint32_t free_index = UINT32_MAX;
    uint32_t i = 0;
    while (i < imgfs_file->header.max_files && free_index == UINT32_MAX) {
        if (imgfs_file->metadata[i].is_valid == EMPTY) {
            free_index = i;
        }
        i++;
    }

    // If no free index is found, the file system is full.
    if (free_index == UINT32_MAX) return ERR_IMGFS_FULL;

    // Store current metadata state for possible restoration in case of deduplication failure.
    struct img_metadata oldmetadata = imgfs_file->metadata[free_index];

    // Pointer to the metadata entry where the new image data will be stored.
    struct img_metadata* metadata = &imgfs_file->metadata[free_index];

    // Calculate the SHA-256 hash of the image buffer for deduplication.
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)image_buffer, image_size, hash);
    memcpy(metadata->SHA, hash, SHA256_DIGEST_LENGTH);

    // Set the image ID and original size in the metadata.
    strncpy(metadata->img_id, img_id, MAX_IMG_ID);
    metadata->size[ORIG_RES] = (uint32_t) image_size;

    // Retrieve and store the image resolution.
    int resolution_err = get_resolution(&metadata->orig_res[1], &metadata->orig_res[0], image_buffer, image_size);
    if (resolution_err != ERR_NONE) {
        return resolution_err;
    }

    // Initialize other metadata fields.
    metadata->size[THUMB_RES] = 0;
    metadata->size[SMALL_RES] = 0;
    metadata->is_valid = NON_EMPTY;
    metadata->offset[ORIG_RES] = 0;
    metadata->offset[SMALL_RES] = 0;
    metadata->offset[THUMB_RES] = 0;
    metadata->unused_16 = 0;

    // Perform deduplication checks; revert changes if deduplication fails.
    int dedup_status = do_name_and_content_dedup(imgfs_file, free_index);
    if (dedup_status != ERR_NONE) {
        memcpy(&imgfs_file->metadata[free_index], &oldmetadata, sizeof(struct img_metadata));
        return dedup_status;
    }

    // Write the image data to the file if it's new (deduplication has not modified the offset).
    if (metadata->offset[ORIG_RES] == 0) {
        if (fseek(imgfs_file->file, 0, SEEK_END) != 0) return ERR_IO;
        metadata->offset[ORIG_RES] = (uint64_t) ftell(imgfs_file->file);
        if (fwrite(image_buffer, 1, image_size, imgfs_file->file) != image_size) return ERR_IO;
    }

    // Update file system header information.
    imgfs_file->header.nb_files++;
    imgfs_file->header.version++;

    // Write updated file system header back to disk.
    if (fseek(imgfs_file->file, 0, SEEK_SET) != 0) return ERR_IO;
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) return ERR_IO;
    if (fseek(imgfs_file->file, sizeof(struct imgfs_header) + free_index * sizeof(struct img_metadata), SEEK_SET) != 0) return ERR_IO;
    if (fwrite(metadata, sizeof(struct img_metadata), 1, imgfs_file->file) != 1) return ERR_IO;

    return ERR_NONE;
}
