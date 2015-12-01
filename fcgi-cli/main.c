#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#ifndef HAVE_FCGI_H
#   include "fcgi.h"
#endif

/**
 * パケットのダンプ
 *
 * @access public
 * @param uint8_t **buffer
 * @param size_t len
 * @return void
 */
void pack_dump(char **packet, size_t len) {
    int ct;
    char *ptr;
    
    ptr = *packet;

    for (ct=0; ct < len; ct++) {
        printf("%02x", *ptr);

        if ((ct % 2) == 1)  {
            printf(" ");
        }
        if ((ct % 8) == 7) {
            printf("\n");
        }
        ptr++;
    }
    printf("\n");

    ptr = *packet;
    for (ct=0; ct < len; ct++) {
        if (0x7e >= *ptr && 0x21 <= *ptr) {
            printf("%c", *ptr);
        } else {
            printf(".");
        }
        if ((ct % 2) == 1)  {
            printf(" ");
        }
        if ((ct % 8) == 7) {
            printf("\n");
        }
        ptr++;
    }
    printf("\n");
}

/**
 * レスポンスイベント
 *
 * @access public
 * @param char *response
 * @param size_t length
 * @param void* argv
 * @return void
 */
void on_response(char *response, size_t length, void* argv)
{
    char *stdin = (char*) argv;
    printf("on_response\n");
    printf("%s\n\n", stdin);
    printf("%s\n", response);
}

/**
 * メイン関数
 *
 * @access public
 * @param int  argc
 * @param char **argv
 * @return int
 */
int main(int argc, char **argv) 
{
    uv_loop_t *loop = uv_default_loop();

    char stdin[] = "test";
    fcgi_params_t params = {
        .socket = "/var/run/php5-fpm.sock",
        .script = "/usr/share/nginx/html/index.php",
        .addr = "127.0.0.1"
    };

    fcgi_request(loop, &params, stdin, on_response, stdin);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
