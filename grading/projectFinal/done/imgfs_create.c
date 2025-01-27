#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "error.h"



int do_create(const char* imgfs_filename, struct imgfs_file* imgfs_file)
{
    // Validate input parameters to ensure they are not null.
    M_REQUIRE_NON_NULL(imgfs_filename);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Set default values in the file system header.
    strncpy(imgfs_file->header.name, CAT_TXT, MAX_IMGFS_NAME+1);  // Note: using CAT_TXT seems like a placeholder. Check correctness.
    imgfs_file->header.version = 0;  // Starting version number.
    imgfs_file->header.nb_files = 0;  // No files initially.
    imgfs_file->header.unused_32 = 0;  // Clear unused data.
    imgfs_file->header.unused_64 = 0;  // Clear unused data.

    // Attempt to create the file for the file system.
    imgfs_file->file = fopen(imgfs_filename, "wb");
    if (imgfs_file->file == NULL) {
        return ERR_IO;  // Return error if file cannot be opened/created.
    }

    // Write the initialized header to the file.
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        fclose(imgfs_file->file);  // Close the file if write fails.
        imgfs_file->file = NULL;  // Nullify the file pointer.
        return ERR_IO;  // Return write error.
    }

    // Allocate memory for the metadata section based on the maximum number of files.
    imgfs_file->metadata = calloc(imgfs_file->header.max_files, sizeof(struct img_metadata));
    if (imgfs_file->metadata == NULL) {
        fclose(imgfs_file->file);  // Close the file if memory allocation fails.
        imgfs_file->file = NULL;  // Nullify the file pointer.
        return ERR_OUT_OF_MEMORY;  // Return memory allocation error.
    }

    // Write the empty metadata for each file slot to the file.
    size_t metadata_count = imgfs_file->header.max_files;
    if (fwrite(imgfs_file->metadata, sizeof(struct img_metadata), metadata_count, imgfs_file->file) != metadata_count) {
        free(imgfs_file->metadata);  // Free metadata memory.
        fclose(imgfs_file->file);  // Close the file.
        imgfs_file->file = NULL;  // Nullify the file pointer.
        imgfs_file->metadata = NULL;  // Nullify the metadata pointer.
        return ERR_IO;  // Return write error.
    }

    // Print confirmation of successful operations.
    printf("%zu item(s) written\n", 1 + metadata_count);

    return ERR_NONE;  // Return success.
}
