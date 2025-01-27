/*
 * @file imgfs_server.c
 * @brief ImgFS server part, main
 *
 * @author Konstantinos Prasopoulos
 */

#include "util.h"
#include "imgfs.h"
#include "socket_layer.h"
#include "http_net.h"
#include "imgfs_server_service.h"

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "bits/sigaction.h"
#include <stdlib.h> // abort()
#include <vips/vips.h>

/********************************************************************/
static void signal_handler(int sig_num _unused)
{
    vips_shutdown();
    server_shutdown();
    exit(0);
}

/********************************************************************/
static void set_signal_handler(void)
{
    struct sigaction action;
    if (sigemptyset(&action.sa_mask) == -1) {
        perror("sigemptyset() in set_signal_handler()");
        abort();
    }
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    if ((sigaction(SIGINT,  &action, NULL) < 0) ||
        (sigaction(SIGTERM,  &action, NULL) < 0)) {
        perror("sigaction() in set_signal_handler()");
        abort();
    }
}

/********************************************************************/

int main (int argc, char *argv[])
{
    if (VIPS_INIT(argv[0])) {
        return ERR_IMGLIB;
    }

    // Start the server
    set_signal_handler();
    server_startup(argc, argv);


    // Loop on http receive as long as there are no error
    while(1) {
        int receive_ret = http_receive();
        if (receive_ret != ERR_NONE) {
            break;
        }
    }

    return 0;
}
