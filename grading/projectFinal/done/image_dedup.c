#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <vips/vips.h>

#include <stdlib.h>
#include <string.h>

int do_name_and_content_dedup(struct imgfs_file* imgfs_file, uint32_t index)
{
    // Ensure the image file structure is not null.
    M_REQUIRE_NON_NULL(imgfs_file);

    // Check if the provided index is within the valid range of files.
    if (index >= imgfs_file->header.max_files) {
        return ERR_IMAGE_NOT_FOUND;
    }

    // Get the metadata for the image at the provided index.
    struct img_metadata *target_metadata = &imgfs_file->metadata[index];

    // If the target image slot is marked as empty, no deduplication is needed.
    if (target_metadata->is_valid == EMPTY) {
        return ERR_NONE;
    }

    int found = 0; // Flag to indicate if a duplicate has been found.

    // Loop through all metadata entries except the one at the provided index.
    for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
        if (i != index && imgfs_file->metadata[i].is_valid != EMPTY) {
            struct img_metadata *current_metadata = &imgfs_file->metadata[i];

            // Check if the current metadata has the same image ID as the target metadata.
            if (strncmp(current_metadata->img_id, target_metadata->img_id, MAX_IMG_ID) == 0) {
                return ERR_DUPLICATE_ID; // Duplicate image ID found.
            }

            // Check if the current metadata has the same SHA-256 hash as the target metadata.
            if (memcmp(current_metadata->SHA, target_metadata->SHA, SHA256_DIGEST_LENGTH) == 0) {
                // Link duplicate entries by copying over offsets and sizes from the found entry to the target.
                for (int res = 0; res < NB_RES; res++) {
                    target_metadata->offset[res] = current_metadata->offset[res];
                    target_metadata->size[res] = current_metadata->size[res];
                }
                found = 1;
                break; // Stop searching after finding a duplicate.
            }
        }
    }

    // If no duplicate was found, set the original resolution offset to zero, indicating it needs initialization.
    if (found == 0) {
        target_metadata->offset[ORIG_RES] = 0;
    }

    return ERR_NONE; // Return success if no issues are encountered.
}
