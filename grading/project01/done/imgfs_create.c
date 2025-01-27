#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "error.h"



int do_create(const char* imgfs_filename, struct imgfs_file* imgfs_file)
{
    M_REQUIRE_NON_NULL(imgfs_filename);
    M_REQUIRE_NON_NULL(imgfs_file);

    strncpy(imgfs_file->header.name, CAT_TXT, MAX_IMGFS_NAME+1);
    imgfs_file->header.version = 0;
    imgfs_file->header.nb_files = 0;
    imgfs_file->header.unused_32 = 0;
    imgfs_file->header.unused_64 = 0;

    imgfs_file->file = fopen(imgfs_filename, "wb");
    if (imgfs_file->file == NULL) {
        return ERR_IO;
    }

    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        fclose(imgfs_file->file);
        imgfs_file->file = NULL;
        return ERR_IO;
    }

    imgfs_file->metadata = calloc(imgfs_file->header.max_files, sizeof(struct img_metadata));
    if (imgfs_file->metadata == NULL) {
        fclose(imgfs_file->file);
        imgfs_file->file = NULL;
        return ERR_OUT_OF_MEMORY;
    }

    size_t metadata_count = imgfs_file->header.max_files;
    if (fwrite(imgfs_file->metadata, sizeof(struct img_metadata), metadata_count, imgfs_file->file) != metadata_count) {
        free(imgfs_file->metadata);
        fclose(imgfs_file->file);
        imgfs_file->file = NULL;
        imgfs_file->metadata = NULL;
        return ERR_IO;
    }

    printf("%zu item(s) written\n", 1 + metadata_count);

    return ERR_NONE;
}
