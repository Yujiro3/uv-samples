/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2011-2014 sheeps.me
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @package         hitSuji
 * @copyright       Copyright (c) 2011-2015 sheeps.me
 * @author          Yujiro Takahashi <yujiro3@gmail.com>
 * @link            http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html
 * @filesource
 */

#ifndef HAVE_FCGI
#define HAVE_FCGI

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <uv.h>

#ifndef HAVE_FCGI_H
#   include "fcgi.h"
#endif

/**
 * Decode a FastCGI Packet
 *
 * @access public
 * @param  fcgi_record_header_t *header
 * @param  char                 *data
 * @return void
 */
void decode_packet_header(fcgi_record_header_t *header, char *data)
{
    header->version       = (uint8_t) data[0];
    header->type          = (uint8_t) data[1];
    header->requestId     = (uint16_t)((data[2] << 8) +  data[3]);
    header->contentLength = (uint16_t)((data[4] << 8) +  data[5]);
    header->paddingLength = (uint8_t) data[6];
    header->reserved      = (uint8_t) data[7];
}

/**
 * パケットの作成
 *
 * @access public
 * @param  uint8_t    *packet
 * @param  uint8_t    type
 * @param  char       *content
 * @param  size_t     content_len
 * @param  uint16_t   requestId
 * @return size_t
 */
size_t build_packet(uint8_t *packet, uint8_t type, char *content, size_t content_len, uint16_t requestId)
{
    uint8_t header[] = {
        VERSION_1,              /* version */
        type,                   /* type */
        FCGI_MSB(requestId),    /* requestIdB1 */
        FCGI_LSB(requestId),    /* requestIdB0 */
        FCGI_MSB(content_len),  /* contentLengthB1 */
        FCGI_LSB(content_len),  /* contentLengthB0 */
        0x00,                   /* paddingLength */
        0x00                    /* reserved */
    }, *ptr;
    ptr = packet;

    /* ヘッダーのコピー */
    memcpy(ptr, header, HEADER_LEN);
    ptr += HEADER_LEN;

    /* コンテンツのコピー */
    if (content_len > 0) {
        memcpy(ptr, content, content_len);
        ptr += content_len;
    }
    
    return (HEADER_LEN + content_len);
}

/**
 * FastCGI Name value pairの作成
 *
 * @access public
 * @param  uint8_t *packet
 * @param  char    *name
 * @param  char    *value
 * @return size_t
 */
size_t build_name_value(uint8_t *packet, char *name, char *value)
{
    uint8_t *ptr = packet;
    size_t  name_len  = strlen(name);
    size_t  value_len = strlen(value);
    size_t  length = 0;

    length = (name_len < 128) ? 1 : 4;
    length = (value_len < 128) ? length + 1 : length + 4;

    if (name_len < 128) {
        /* nameLengthB0 */
        *ptr = (uint8_t) name_len;                  ptr++;
    } else {
        /* nameLengthB3 & nameLengthB2 & nameLengthB1 & nameLengthB0 */
        *ptr = (uint8_t)(name_len >> 24) | 0x80;    ptr++;
        *ptr = (uint8_t)(name_len >> 16);           ptr++;
        *ptr = (uint8_t)(name_len >> 8);            ptr++;
        *ptr = (uint8_t) name_len;                  ptr++;
    }

    if (value_len < 128) {
        /* nameLengthB0 */
        *ptr = (uint8_t) value_len;                 ptr++;
    } else {
        /* nameLengthB3 & nameLengthB2 & nameLengthB1 & nameLengthB0 */
        *ptr = (uint8_t)(value_len >> 24) | 0x80;   ptr++;
        *ptr = (uint8_t)(value_len >> 16);          ptr++;
        *ptr = (uint8_t)(value_len >> 8);           ptr++;
        *ptr = (uint8_t) value_len;                 ptr++;
    }

    /* 名前（キー）のコピー */
    memcpy(ptr, name, name_len);
    ptr += name_len;

    /* 名前（キー）のコピー */
    memcpy(ptr, value, value_len);
    ptr += value_len;

    return (name_len + value_len + length);
}

/**
 * 値の設定
 *
 * @access public
 * @param  uint8_t *packet
 * @param  fcgi_params_t *params
 * @param  char *stdin
 * @param  uint16_t requestId
 * @return size_t
 */
size_t build_params_packet(uint8_t *packet, fcgi_params_t *params, char *stdin, uint16_t requestId)
{
    char *ptr, contentlen[8], build_param[608];
    uint8_t method[32], clength[32], script[256], addr[256], port[32];
    size_t  method_len=0, clength_len=0, script_len=0, addr_len=0, port_len=0;

    method_len = build_name_value(method, "REQUEST_METHOD", "PUT");

    sprintf(contentlen, "%d", (int)strlen(stdin));
    clength_len = build_name_value(clength, "CONTENT_LENGTH", contentlen);

    if (strlen(params->script)) {
        script_len = build_name_value(script, "SCRIPT_FILENAME", params->script);
    }

    if (strlen(params->addr)) {
        addr_len = build_name_value(addr, "REMOTE_ADDR", params->addr);
    }

    ptr = build_param;

    /* REQUEST_METHODのコピー */
    memcpy(ptr, method, method_len);
    ptr += method_len;

    /* CONTENT_LENGTHのコピー */
    memcpy(ptr, clength, clength_len);
    ptr += clength_len;

    /* SCRIPT_FILENAMEのコピー */
    if (script_len > 0) {
        memcpy(ptr, script, script_len);
        ptr += script_len;
    }

    /* REMOTE_ADDRのコピー */
    if (addr_len > 0) {
        memcpy(ptr, addr, addr_len);
        ptr += addr_len;
    }

    /* REMOTE_PORTのコピー */
    if (port_len > 0) {
        memcpy(ptr, port, port_len);
        ptr += port_len;
    }

    return build_packet(packet, PARAMS, build_param, (method_len + clength_len + script_len + addr_len + port_len), requestId);
}

/**
 * CGIリクエストの構築
 *
 * @access public
 * @param fcgi_params_t *params
 * @param char *stdin
 * @return fcgi_request_t
 */
fcgi_request_t build_request(fcgi_params_t *params, char *stdin)
{
    fcgi_request_t request;
    char header[HEADER_LEN] = {
        0x00, RESPONDER, 0x00, 0x00,
        0x00, 0x00,      0x00, 0x00
    };
    uint8_t *ptr, *packet = NULL;
    uint8_t pack_header[16], pack_params[616], pack_params_end[16];
    uint8_t *pack_stdin = NULL, pack_stdin_end[16];
    size_t pack_header_len, pack_params_len, pack_params_end_len;
    size_t pack_stdin_len, pack_stdin_end_len;

    // Pick random number between 1 and max 16 bit unsigned int 65535
    srand((unsigned)time(NULL));
    request.id = (uint16_t)(rand() % REQUEST_ID_MAX) + 1;
    request.id = 1;

    /* ヘッダーパケットの取得 */
    pack_header_len = build_packet(pack_header, BEGIN_REQUEST, header, HEADER_LEN, request.id);

    /* パラメータパケットの取得 */
    pack_params_len     = build_params_packet(pack_params, params, stdin, request.id);
    pack_params_end_len = build_packet(pack_params_end, PARAMS, NULL, 0, request.id);

    /* 入力値パケットの取得 */
    pack_stdin         = malloc(HEADER_LEN + strlen(stdin));
    pack_stdin_len     = build_packet(pack_stdin, STDIN, stdin, strlen(stdin), request.id);
    pack_stdin_end_len = build_packet(pack_stdin_end, STDIN, NULL, 0, request.id);

    /* メモリスペースの確保 */
    ptr = packet = malloc(pack_header_len + pack_params_len + pack_params_end_len + pack_stdin_len + pack_stdin_end_len);

    /* ヘッダーパケットのコピー */
    memcpy(ptr, pack_header, pack_header_len);
    ptr += pack_header_len;

    /* パラメータパケットのコピー */
    memcpy(ptr, pack_params, pack_params_len);
    ptr += pack_params_len;
    memcpy(ptr, pack_params_end, pack_stdin_end_len);
    ptr += pack_stdin_end_len;

    /* 入力値パケットのコピー */
    memcpy(ptr, pack_stdin, pack_stdin_len);
    free(pack_stdin);
    ptr += pack_stdin_len;
    memcpy(ptr, pack_stdin_end, pack_stdin_end_len);
    ptr += pack_stdin_end_len;

    request.wrbuf = uv_buf_init(
        (char*) packet, 
        (
            pack_header_len + pack_params_len + pack_params_end_len + 
            pack_stdin_len + pack_stdin_end_len
        )
    );

    return request;
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
    fcgi_request_t *request = (fcgi_request_t *) handle->data;

    /* fcgi_request()で確保したメモリを開放 */
    free(request);
}

/**
 * メモリスペースの確保
 *
 * @access public
 * @param uv_handle_t *handle
 * @param size_t suggested_size
 * @param uv_buf_t *buf
 * @return void
 */
static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);     // on_read()で解放
    buf->len = suggested_size;
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
static void on_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
    fcgi_record_header_t header;
    fcgi_request_t *request = (fcgi_request_t *) handle->data;
    char *ptr, *contents;
    size_t length;

    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*) handle, on_close);
        return;
    }
    ptr = buf->base;
    decode_packet_header(&header, ptr);
    contents = ptr += HEADER_LEN;

    ptr    = strstr(ptr, SEPARATOR);
    ptr   += SEPARATOR_LEN;
    length = header.contentLength - (ptr - contents);

    if (request->cb != NULL) {
        request->cb(ptr, length, request->argv);
    }

    /* alloc_buffer()で確保したメモリを開放 */
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
static void on_write(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }
    fcgi_request_t *request = (fcgi_request_t *) req->data;

    free(request->wrbuf.base);
    request->wrbuf.len = 0;

    /* on_connect()で確保したメモリを開放 */
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
static void on_connect(uv_connect_t* req, int status)
{
    if (status == 0) {
        fcgi_request_t *request = (fcgi_request_t *) req->data;
        uv_write_t *write = (uv_write_t *) malloc(sizeof(uv_write_t));      // on_write()で解放
        
        req->handle->data = (void *)request;
        write->data       = (void *)request;
        write->handle     = req->handle;

        uv_read_start((uv_stream_t*) req->handle, alloc_buffer, on_read);
        uv_write(write, req->handle, (uv_buf_t *)&request->wrbuf, 1, on_write);
    }
}

/**
 * CGIリクエスト
 *
 * @access public
 * @param uv_loop_t *loop
 * @param fcgi_params_t *params
 * @param char *stdin
 * @param fcgi_response_cb cb
 * @param void *argv
 * @return int
 */
int fcgi_request(uv_loop_t *loop, fcgi_params_t *params, char *stdin, fcgi_response_cb cb, void *argv)
{
    fcgi_request_t *request = NULL;
    
    request = (fcgi_request_t *) malloc(sizeof(fcgi_request_t));        // on_close()で解放
    *request = build_request(params, stdin);

    if (0 > request->id) {
        return -1;
    }
    request->argv = argv;
    request->cb   = cb;
    request->conn.data = (void *) request;

    uv_pipe_init(loop, &request->handle, 0);
    uv_pipe_connect(&request->conn, &request->handle, params->socket, on_connect);

    return 0;
}

#endif  // #ifndef HAVE_FCGI
