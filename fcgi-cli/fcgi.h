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
 * @link            http://public.dhe.ibm.com/software/dw/jp/websphere/wmq/mqtt31_spec/mqtt-v3r1_ja.pdf
 * @filesource
 */

#ifndef HAVE_FCGI_H
#define HAVE_FCGI_H

#include <stdint.h>

#define VERSION_1           0x01

#define BEGIN_REQUEST       1
#define ABORT_REQUEST       2
#define END_REQUEST         3
#define PARAMS              4
#define STDIN               5
#define STDOUT              6
#define STDERR              7
#define DATA                8
#define GET_VALUES          9
#define GET_VALUES_RESULT   10
#define UNKNOWN_TYPE        11

#define RESPONDER           0x01

#define SEPARATOR           "\r\n\r\n"
#define SEPARATOR_LEN       (4)

#define HEADER_LEN          (8)

#define REQUEST_ID_MAX      (65534)

#define FCGI_MSB(x) (uint8_t) ((x & 0xFF00) >> 8)
#define FCGI_LSB(x) (uint8_t) (x & 0x00FF)

typedef void (*fcgi_response_cb)(char *response, size_t length, void* argv);

typedef struct {
    char *socket;
    char *script;
    char *addr;
} fcgi_params_t;

typedef struct {
    uint8_t  version;
    uint8_t  type;
    uint16_t requestId;
    uint16_t contentLength;
    uint8_t  paddingLength;
    uint8_t  reserved;
} fcgi_record_header_t;

typedef struct {
    void            *argv;
    uv_pipe_t        handle;
    uv_connect_t     conn;
    uint16_t         id;
    uv_buf_t         wrbuf;
    fcgi_response_cb cb;
} fcgi_request_t;

int fcgi_request(uv_loop_t *loop, fcgi_params_t *params, char *stdin, fcgi_response_cb cb, void *argv);

#endif  // #ifndef HAVE_FCGI_H
