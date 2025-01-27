#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <json-c/json.h>
#include "error.h"

#define OUTPUT_STDOUT 0
#define OUTPUT_JSON   1



int do_list(const struct imgfs_file* imgfs_file, enum do_list_mode output_mode, char** json_output)
{
    // Ensure the imgfs_file pointer is not null.
    M_REQUIRE_NON_NULL(imgfs_file);

    if (output_mode == STDOUT) {
        // Print the header information.
        print_header(&imgfs_file->header);

        // Check if there are no files and output a placeholder message.
        if (imgfs_file->header.nb_files == 0) {
            puts("<< empty imgFS >>");
        } else {
            // Iterate through all possible file slots and print metadata for non-empty slots.
            for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
                if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                    print_metadata(&imgfs_file->metadata[i]);
                }
            }
        }
        return ERR_NONE;
    } else if (output_mode == JSON) {
        // Create a new JSON object.
        struct json_object *json_obj = json_object_new_object();
        if (!json_obj) {
            return ERR_RUNTIME;
        }

        // Create a JSON array to hold image IDs.
        struct json_object *json_array = json_object_new_array();
        if (!json_array) {
            json_object_put(json_obj);
            return ERR_RUNTIME;
        }

        // Populate the JSON array with image IDs from valid metadata entries.
        for (uint32_t i = 0; i < imgfs_file->header.max_files; i++) {
            if (imgfs_file->metadata[i].is_valid == NON_EMPTY) {
                struct json_object *img_id = json_object_new_string(imgfs_file->metadata[i].img_id);
                if (!img_id) {
                    json_object_put(json_array);
                    json_object_put(json_obj);
                    return ERR_RUNTIME;
                }
                json_object_array_add(json_array, img_id);
            }
        }

        // Add the array to the main JSON object under the key "Images".
        json_object_object_add(json_obj, "Images", json_array);

        // Convert the JSON object to a string.
        const char *json_str = json_object_to_json_string(json_obj);
        if (!json_str) {
            json_object_put(json_obj);
            return ERR_RUNTIME;
        }

        // Duplicate the JSON string to ensure it persists outside of this function.
        *json_output = strdup(json_str);
        if (!*json_output) {
            json_object_put(json_obj);
            return ERR_OUT_OF_MEMORY;
        }

        // Clean up the JSON object.
        json_object_put(json_obj);

        return ERR_NONE;
    } else {
        // Handle invalid output modes.
        return ERR_INVALID_ARGUMENT;
    }
}

