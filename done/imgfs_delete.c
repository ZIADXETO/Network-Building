#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "error.h"

int do_delete(const char* img_id, struct imgfs_file* imgfs_file)
{
    // Validate the input parameters to ensure they are not null.
    M_REQUIRE_NON_NULL(img_id);
    M_REQUIRE_NON_NULL(imgfs_file);

    // Ensure the file handle is not null.
    if (imgfs_file->file == NULL) {
        return ERR_IO;
    }

    // Check if there are any files to delete.
    if (imgfs_file->header.nb_files == 0) {
        return ERR_IMAGE_NOT_FOUND;
    }

    int found = 0; // This will be set to 1 if the image is found and deleted.
    uint32_t i = 0; // Index to iterate through metadata entries.

    // Loop through metadata entries to find and delete the image.
    while (found != 1 && i < imgfs_file->header.max_files) {
        if (imgfs_file->metadata[i].is_valid == NON_EMPTY &&
            strncmp(imgfs_file->metadata[i].img_id, img_id, MAX_IMG_ID) == 0) {
            imgfs_file->metadata[i].is_valid = EMPTY; // Mark the metadata entry as empty.

            // Update the metadata in the file.
            fseek(imgfs_file->file, sizeof(struct imgfs_header) + i * sizeof(struct img_metadata), SEEK_SET);
            if (fwrite(&imgfs_file->metadata[i], sizeof(struct img_metadata), 1, imgfs_file->file) != 1) {
                return ERR_IO;
            }

            found = 1; // Indicate that the image has been found and marked for deletion.
        }
        ++i;
    }

    // If the image was not found, return an error.
    if (!found) {
        return ERR_IMAGE_NOT_FOUND;
    }

    // Decrement the number of files and increment the version of the file system.
    imgfs_file->header.nb_files--;
    imgfs_file->header.version++;

    // Update the file system header in the file.
    fseek(imgfs_file->file, 0, SEEK_SET);
    if (fwrite(&imgfs_file->header, sizeof(struct imgfs_header), 1, imgfs_file->file) != 1) {
        return ERR_IO;
    }

    return ERR_NONE; // Return success if the image was deleted successfully.
}

