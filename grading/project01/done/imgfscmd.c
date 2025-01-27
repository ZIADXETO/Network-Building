/**
 * @file imgfscmd.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * Image Filesystem Command Line Tool
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused
#include <vips/vips.h>

#include <stdlib.h>
#include <string.h>


/**
 * @typedef command
 * @brief Defines a function pointer type for handling commands.
 *
 * A function pointer type used to associate command-line operations with their handlers.
 * Each function takes command line arguments (argc, argv) and returns an integer status code.
 *
 * @param argc Integer of count of command-line arguments.
 * @param argv Array of character pointers representing command-line arguments.
 * @return Integer status code (0 for success, non-zero for error).
 */
typedef int (*command) (int argc, char* argv[]) ;

/**
 * @struct command_mapping
 * @brief Maps command names to their corresponding function pointers.
 *
 * This structure links a human-readable command name to a specific function designed to execute the command.
 *
 * @param func_name The name of the command.
 * @param func The function that will be executed when this command is invoked.
 */
typedef struct {
    const char* func_name ;
    command func ;
} command_mapping ;

/**
 * @var commands
 * @brief Static array of command_mapping structures to define the command-line interface.
 *
 * Defines an array of commands supported by this application, linking command names to their
 * corresponding execution functions. Array is terminated by a NULL entry.
 */
static command_mapping commands[] = {
    {"list", do_list_cmd},
    {"create", do_create_cmd},
    {"help", help},
    {"delete", do_delete_cmd},
    {NULL, NULL},
} ;


/*******************************************************************************
 * MAIN
 */

/**
 * @fn main
 * @brief The entry point for the command-line utility.
 *
 * Initializes the application, processes command-line arguments, executes the corresponding
 * command, and handles errors.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return Returns an integer indicating the success or error of command execution.
 */
int main(int argc, char* argv[])
{
    VIPS_INIT(argv[0]);

    int ret = 0;

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        /* **********************************************************************
         * TODO WEEK 07: THIS PART SHALL BE EXTENDED.
         * **********************************************************************
         */
        argc--; argv++; // skips command call name

        int found = 0 ;
        int i = 0 ;
        while (found!=1 && commands[i].func_name!=NULL) {
            if (strncmp(argv[0], commands[i].func_name, MIN(strlen(commands[i].func_name), strlen(argv[0])) + 1) == 0) {
                ret = commands[i].func(argc-1, argv+1);
                found = 1;
            }
            ++i;
        }

        if (!found) {
            ret = ERR_INVALID_COMMAND;
        }
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help(argc, argv);
    }

    vips_shutdown();

    return ret;
}
