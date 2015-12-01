#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define ECHO_SOCK "~/uv-sample/pipe-echo-server/echo.sock"

/**
 * メモリスペースの確保
 *
 * @access public
 * @param uv_handle_t* handle
 * @param size_t suggested_size
 * @param uv_buf_t *buf
 * @return void
 */
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

/**
 * クローズイベント
 *
 * @access public
 * @param uv_handle_t* handle
 * @return void
 */
static void on_close(uv_handle_t* handle)
{
    printf("client closed\n");
}

/**
 * 読み込みイベント
 *
 * @access public
 * @param uv_stream_t *handle
 * @param ssize_t nread
 * @param const uv_buf_t *buf
 * @return void
 */
void echo_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
    printf("read:nread:%d\n", (int)nread);

    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*) handle, on_close);
        return;
    }
    printf("read:%s\n", buf->base);
    free(buf->base);
    uv_close((uv_handle_t*) handle, on_close);
}

/**
 * 書込みイベント
 *
 * @access public
 * @param uv_write_t *req
 * @param int        status
 * @return void
 */
void echo_write(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }
    
    free(req);
}

/**
 * 接続イベント
 *
 * @access public
 * @param uv_connect_t* req
 * @param int status
 * @return void
 */
void on_connect(uv_connect_t* req, int status)
{
    if (status == 0) {
        uv_write_t *write = (uv_write_t *) malloc(sizeof(uv_write_t));
        uv_buf_t wrbuf = uv_buf_init("test", strlen("test"));

        uv_read_start((uv_stream_t*) req->handle, alloc_buffer, echo_read);

        
        write->handle = req->handle;
        uv_write(write, req->handle, &wrbuf, 1, echo_write);
    }
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
    uv_pipe_t handle;
    uv_connect_t req;
    int result;

    result = uv_pipe_init(loop, &handle, 0);

    printf("result:%d\n", result);
    uv_pipe_connect(&req, &handle, ECHO_SOCK, on_connect);

    result = uv_run(loop, UV_RUN_DEFAULT);
    printf("loop:%d\n", result);

    return 0;
}