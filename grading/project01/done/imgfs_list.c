#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "error.h"

int do_list(const struct imgfs_file* imgfs_file, enum do_list_mode output_mode, char** json)
{
    M_REQUIRE_NON_NULL(imgfs_file);
    if (output_mode == STDOUT) {
        print_header(&imgfs_file->header);
        if (imgfs_file->header.nb_files == 0) {
            puts("<< empty imgFS >>");
        } else {
            for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
                if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&imgfs_file->metadata[i]);
                }
            }
        }
        return ERR_NONE;
    } else if (output_mode == JSON) {
        TO_BE_IMPLEMENTED();
        return ERR_NONE;
    } else {
        return ERR_INVALID_ARGUMENT;
    }
}
