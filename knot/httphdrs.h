#ifndef HTTPHDRS_H
#define HTTPHDRS_H

#define HEADER_200_BUF_SIZE 128
#define HEADER_200 ("HTTP/1.1 200 OK\r\n" \
                    "Content-Type: %s\r\n" \
                    "Content-Length: %d\r\n" \
                    "\r\n")

#define HEADER_404 ("HTTP/1.1 404 Not Found\r\n" \
                    "Connection: close\r\n")

#endif
