#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "error.h"

int do_delete(const char* img_id, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);

    int found = 0;
    uint32_t i = 0;

    while (found!=1 && i<imgfs_file->header.max_files) {
        if (imgfs_file->metadata[i].is_valid == NON_EMPTY &&
            strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            imgfs_file->metadata[i].is_valid = EMPTY;

            fseek(imgfs_file->file, sizeof(struct imgfs_header) + i * sizeof(struct img_metadata), SEEK_SET);
            if (fwrite(&imgfs_file->metadata[i], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
                return ERR_IO;
            }

            found = 1;
        }
        ++i;
    }

    if (!found) {
        return ERR_IMAGE_NOT_FOUND;
    }

    imgfs_file->header.nb_files--;
    imgfs_file->header.version++;

    fseek(imgfs_file->file, 0, SEEK_SET);
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    return ERR_NONE;
}
