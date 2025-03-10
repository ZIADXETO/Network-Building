/**
 * @file imgfscmd_functions.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <json-c/json.h>

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
           "                                  default value is %u\n"
           "                                  maximum value is %u\n"
           "          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "          -small_res <X_RES> <Y_RES>: resolution for small images.\n"
           "                                  default value is %dx%d\n"
           "                                  maximum value is %dx%d\n"
           "  read   <imgFS_filename> <imgID> [original|orig|thumbnail|thumb|small]:\n"
           "      read an image from the imgFS and save it to a file.\n"
           "      default resolution is \"original\".\n"
           "  insert <imgFS_filename> <imgID> <filename>: insert a new image in the imgFS.\n"
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

static void create_name(const char* img_id, int resolution, char** new_name)
{
    const char* suffix;
    switch (resolution) {
    case ORIG_RES:
        suffix = "_orig.jpg";
        break;
    case SMALL_RES:
        suffix = "_small.jpg";
        break;
    case THUMB_RES:
        suffix = "_thumb.jpg";
        break;
    default:
        suffix = "_unknown.jpg";
        break;
    }

    size_t name_len = strlen(img_id) + strlen(suffix) + 1;
    *new_name = (char*)calloc(name_len, sizeof(char));
    if (*new_name) {
        snprintf(*new_name, name_len, "%s%s", img_id, suffix);
    }
}


static int write_disk_image(const char *filename, const char *image_buffer, uint32_t image_size)
{
    FILE *file = fopen(filename, "wb");
    if (!file) {
        return ERR_IO;
    }

    size_t written = fwrite(image_buffer, 1, image_size, file);
    fclose(file);

    return (written == image_size) ? ERR_NONE : ERR_IO;
}

static int read_disk_image(const char *path, char **image_buffer, uint32_t *image_size)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        return ERR_IO;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return ERR_IO;
    }

    *image_buffer = (char*)calloc((unsigned long)file_size, sizeof(char));
    if (!*image_buffer) {
        fclose(file);
        return ERR_OUT_OF_MEMORY;
    }

    size_t read_size = fread(*image_buffer, 1,(unsigned long) file_size, file);
    fclose(file);

    if (read_size != (unsigned long) file_size) {
        free(*image_buffer);
        return ERR_IO;
    }

    *image_size = (uint32_t)file_size;
    return ERR_NONE;
}

int do_read_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 2 && argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    const char * const img_id = argv[1];

    const int resolution = (argc == 3) ? resolution_atoi(argv[2]) : ORIG_RES;
    if (resolution == -1) return ERR_RESOLUTIONS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size = 0;
    error = do_read(img_id, resolution, &image_buffer, &image_size, &myfile);
    do_close(&myfile);
    if (error != ERR_NONE) {
        return error;
    }

    // Extracting to a separate image file.
    char* tmp_name = NULL;
    create_name(img_id, resolution, &tmp_name);
    if (tmp_name == NULL) return ERR_OUT_OF_MEMORY;
    error = write_disk_image(tmp_name, image_buffer, image_size);
    free(tmp_name);
    free(image_buffer);

    return error;
}

int do_insert_cmd(int argc, char **argv)
{
    M_REQUIRE_NON_NULL(argv);
    if (argc != 3) return ERR_NOT_ENOUGH_ARGUMENTS;

    struct imgfs_file myfile;
    zero_init_var(myfile);
    int error = do_open(argv[0], "rb+", &myfile);
    if (error != ERR_NONE) return error;

    char *image_buffer = NULL;
    uint32_t image_size;

    // Reads image from the disk.
    error = read_disk_image (argv[2], &image_buffer, &image_size);
    if (error != ERR_NONE) {
        do_close(&myfile);
        return error;
    }

    error = do_insert(image_buffer, image_size, argv[1], &myfile);
    free(image_buffer);
    do_close(&myfile);
    return error;
}
