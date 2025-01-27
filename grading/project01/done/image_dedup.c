#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <vips/vips.h>

#include <stdlib.h>
#include <string.h>

int do_name_and_content_dedup(struct imgfs_file* imgfs_file, uint32_t index)
{
    M_REQUIRE_NON_NULL(imgfs_file);
    if (index >= imgfs_file->header.max_files) {
        return ERR_IMAGE_NOT_FOUND;
    }

    struct img_metadata *target_metadata = &imgfs_file->metadata[index];
    if (target_metadata->is_valid == EMPTY) {
        return ERR_NONE;
    }

    int found = 0;
    for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
        if (i != index && imgfs_file->metadata[i].is_valid != EMPTY) {

            struct img_metadata *current_metadata = &imgfs_file->metadata[i];

            if (strncmp(current_metadata->img_id, target_metadata->img_id, MAX_IMG_ID) == 0) {
                return ERR_DUPLICATE_ID;
            }

            if (memcmp(current_metadata->SHA, target_metadata->SHA, SHA256_DIGEST_LENGTH) == 0) {
                for (int res = 0; res < NB_RES; res++) {
                    target_metadata->offset[res] = current_metadata->offset[res];
                    target_metadata->size[res] = current_metadata->size[res];
                }
                found = 1;
            }
        }
    }

    if (found == 0) {
        target_metadata->offset[ORIG_RES] = 0;
    }

    return ERR_NONE;
}
