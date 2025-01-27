/**
 * @file imgfscmd_functions.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// default values
static const uint32_t default_max_files = 128;
static const uint16_t default_thumb_res =  64;
static const uint16_t default_small_res = 256;

// max values
static const uint16_t MAX_THUMB_RES = 128;
static const uint16_t MAX_SMALL_RES = 512;

/************************
 * Displays some explanations.
 ************************ */
int help(int useless _unused, char** useless_too _unused)
{
    /* ************************
     * TODO WEEK 08: WRITE YOUR CODE HERE.
     * ************************
     */
    printf("imgfscmd [COMMAND] [ARGUMENTS]\n"
           "  help: displays this help.\n"
           "  list <imgFS_filename>: list imgFS content.\n"
           "  create <imgFS_filename> [options]: create a new imgFS.\n"
           "      options are:\n"
           "          -max_files <MAX_FILES>: maximum number of files.\n"
           "                                  default value is %d\n"
           "                                  maximum value is %u\n"
           "          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "          -small_res <X_RES> <Y_RES>: resolution for small images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "  delete <imgFS_filename> <imgID>: delete image imgID from imgFS.\n",
           default_max_files, UINT32_MAX,
           default_thumb_res, default_thumb_res, MAX_THUMB_RES, MAX_THUMB_RES,
           default_small_res, default_small_res, MAX_SMALL_RES, MAX_SMALL_RES);

    return ERR_NONE;
}

/************************
 * Opens imgFS file and calls do_list().
 ************************ */
int do_list_cmd(int argc, char** argv)
{
    /* ************************
     * TODO WEEK 07: WRITE YOUR CODE HERE.
     * ************************
     */
    M_REQUIRE_NON_NULL(argv);
    if (argc > 1) return ERR_INVALID_COMMAND;
    if (argc < 1) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file imgfs_file;

    int open = do_open(argv[0], "rb", &imgfs_file);
    if (open != ERR_NONE) {
        return open;
    }

    char* json_output = NULL;
    int list = do_list(&imgfs_file, STDOUT, &json_output);
    if (list != ERR_NONE) {
        do_close(&imgfs_file);
        return list;
    }

    do_close(&imgfs_file);

    return ERR_NONE;
}

/************************
 * Prepares and calls do_create command.
************************ */
int do_create_cmd(int argc, char** argv)
{

    puts("Create");
    /* ************************
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * ************************
     */
    M_REQUIRE_NON_NULL(argv);

    if (argc < 1) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    struct imgfs_file imgfs_file;

    imgfs_file.header.max_files = default_max_files;
    imgfs_file.header.resized_res[0] = default_thumb_res;
    imgfs_file.header.resized_res[1] = default_thumb_res;
    imgfs_file.header.resized_res[2] = default_small_res;
    imgfs_file.header.resized_res[3] = default_small_res;

    const char* imgfs_filename = argv[0];

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-max_files", MIN(strlen(argv[i]), strlen("-max_files")) + 1) == 0) {
            if (i + 1 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint32_t max_files = atouint32(argv[++i]);
            if (max_files == 0 || max_files > UINT32_MAX) {
                return ERR_MAX_FILES;
            }
            imgfs_file.header.max_files = max_files;
        } else if (strncmp(argv[i], "-thumb_res", MIN(strlen(argv[i]), strlen("-thumb_res")) + 1) == 0) {
            if (i + 2 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint16_t width = atouint16(argv[++i]);
            uint16_t height = atouint16(argv[++i]);
            if (width == 0 || width > MAX_THUMB_RES || height == 0 || height > MAX_THUMB_RES) {
                return ERR_RESOLUTIONS;
            }
            imgfs_file.header.resized_res[0] = width;
            imgfs_file.header.resized_res[1] = height;
        } else if (strncmp(argv[i], "-small_res", MIN(strlen(argv[i]), strlen("-small_res")) + 1) == 0) {
            if (i + 2 >= argc) {
                return ERR_NOT_ENOUGH_ARGUMENTS;
            }
            uint16_t width = atouint16(argv[++i]);
            uint16_t height = atouint16(argv[++i]);
            if (width == 0 || width > MAX_SMALL_RES || height == 0 || height > MAX_SMALL_RES) {
                return ERR_RESOLUTIONS;
            }
            imgfs_file.header.resized_res[2] = width;
            imgfs_file.header.resized_res[3] = height;
        } else {
            return ERR_INVALID_ARGUMENT;
        }
    }

    int create = do_create(imgfs_filename, &imgfs_file);
    if (create != ERR_NONE) {
        return create;
    }

    do_close(&imgfs_file);

    return ERR_NONE;
}
/************************
 * Deletes an image from the imgFS.
 */
int do_delete_cmd(int argc, char** argv)
{
    /* ********
     * TODO WEEK 08: WRITE YOUR CODE HERE (and change the return if needed).
     * ********
     */
    M_REQUIRE_NON_NULL(argv);
    if (argc > 2) return ERR_INVALID_ARGUMENT;
    else if ( argc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file imgfs_file;

    const char* id = argv[1];

    if (id==NULL || strlen(id)>MAX_IMG_ID) {
        return ERR_INVALID_IMGID;
    }

    int open = do_open(argv[0], "rb+", &imgfs_file);
    if (open != ERR_NONE) {
        return open;
    }

    int delete = do_delete(id, &imgfs_file);
    if (delete != ERR_NONE) {
        do_close(&imgfs_file);
        return delete;
    }
    do_close(&imgfs_file);

    return ERR_NONE;
}
