/**
 * httpget - simple HTTP client to download URL contents
 *
 * Homework for mail.ru application @ 25.04.2015 - 26.04.2015
 */

#define _POSIX_SOURCE

#include "url.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <pcre.h>

/*************************************************************************************************/

static int parse_url(const char* urlstr, url_t* url)
{
    int error = 0;

    url_parser_t* parser;
    error = url_parser_init_default(&parser);
    if (error) {
        fprintf(stderr, "Could not initilize url parser: %s\n", strerror(error));
        return error;
    }

    error = url_parse(parser, urlstr, url);
    url_parser_free(parser);

    if (error) {
        fprintf(stderr, "Could not parse URL \'%s\': %s\n", urlstr, strerror(error));
        return error;
    }

    return 0;
}

static int connect_socket(const char* host, const char* port, int* out_sockfd)
{
    int error = 0;

    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    struct addrinfo* res = NULL;
    error = getaddrinfo(host, port, &hints, &res);
    if (error) {
        fprintf(stderr, "getaddrinfo('%s') failed with %s\n", host, gai_strerror(error));
        return error;
    }

    int fd = -1;
    struct addrinfo* hostinfo = res;
    while (hostinfo != NULL) {

        // Look for AF_INET and AF_INET6 family
        if (hostinfo->ai_family == AF_INET || hostinfo->ai_family == AF_INET6) {
            fd = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
            if (fd < 0) {
                perror("Failed to create socket");
                freeaddrinfo(res);
                return -1;
            }


            error = connect(fd, hostinfo->ai_addr, hostinfo->ai_addrlen);
            if (error) {
                fprintf(stderr, "Failed to connect: %s\n", strerror(error));
                close(fd);
                freeaddrinfo(res);
                return error;
            }

            break;
        }

        hostinfo = hostinfo->ai_next;
    }

    assert(fd >= 0);

    freeaddrinfo(res);
    *out_sockfd = fd;

    return 0;
}

static int send_http_get(const url_t* url, int sockfd)
{
    int res = 0;

    // Build HTTP get query
    const char* format = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n";
    size_t query_length = snprintf(NULL, 0, format, url->path ? url->path : "/", url->host);
    size_t bufsize = query_length + 1;

    char* query = calloc(1, bufsize);
    if (!query) {
        fprintf(stderr, "No memory to allocate query buffer size %zd\n", bufsize);
        return ENOMEM;
    }

    snprintf(query, bufsize, format, (url->path ? url->path : "/"), url->host);

    // Send the query
    int sent = 0;
    while (sent < query_length) {
        res = send(sockfd, query + sent, query_length - sent, 0);
        if (res == -1) {
            perror("send failed\n");
            free(query);
            return res;
        }

        sent += res;
    }

    free(query);
    return 0;
}

/*
 * A slow way to recieve a line from HTTP reply
 * Called initially to parse reply header
  */
static int recv_line(int sockfd, char* buf, size_t maxchars)
{
    size_t nbytes = 0;
    char prev = '\0';
    char cur = '\0';
    while (maxchars-- > 0)
    {
        int res = recv(sockfd, &cur, 1, 0);
        if (res == -1) {
            perror("recv failed");
            return res;
        } 
        else if (res == 0) {
            break;
        }

        // Look for \r\n
        if (prev == '\r' && cur == '\n') {
            break;
        }

        *buf++ = cur;
        prev = cur;
        ++nbytes;
    }

    return nbytes;    
}

/*
 * Parses HTTP reply header, extracts status and skips until the start of data
 */
static int parse_http_reply(int sockfd)
{
    int res = 0;

    // Recv reply header line
    char linebuf[1024] = {0};
    res = recv_line(sockfd, linebuf, sizeof(linebuf) - 1);
    if (res == -1) {
        fprintf(stderr, "Failed to recieve HTTP reply\n");
        return res;
    }

    fprintf(stderr, "%s\n", linebuf);

    const char* reply_regex = "^HTTP/1.[01] ([\\d]+) ([\\w]+)";
    const char* error_str = NULL;
    int error_offset = 0;

    pcre* re = pcre_compile(reply_regex, 0, &error_str, &error_offset, NULL);
    if (!re) {
        fprintf(stderr, "Failed to compile regex: %s\n", error_str);
        return -1;
    }

    int matchvec[9] = {0};
    int nmatches = pcre_exec(re, NULL, linebuf, strlen(linebuf), 0, 0, matchvec, sizeof(matchvec) / sizeof(*matchvec));
    pcre_free(re);

    if (nmatches < 0) {
        fprintf(stderr, "HTTP reply header match failed: %d\n", nmatches);
        return -1;
    }

    const char* status_code_str = NULL;
    res = pcre_get_substring(linebuf, matchvec, nmatches, 1, &status_code_str);
    if (res < 0) {
        fprintf(stderr, "Status code string failed to match\n");
        return res;
    }

    // Check OK status
    long status_code = atol(status_code_str);
    pcre_free_substring(status_code_str);

    fprintf(stderr, "HTTP reply status code %ld\n", status_code);

    if (status_code != 200) {
        fprintf(stderr, "HTTP request failed\n");
        return -1;
    }

    // Skip until start of data
    while (1) {
        res = recv_line(sockfd, linebuf, sizeof(linebuf) - 1);
        if (res == -1) {
            fprintf(stderr, "Failed to recieve HTTP reply\n");
            return res;
        }

        if (0 == strncmp(linebuf, "\r\n", res)) {
            break;
        }
    }

    return 0;    
}

/*************************************************************************************************/

static void usage()
{
    fprintf(stdout, "httpget - simple HTTP client to download URL contents\n");
    fprintf(stdout, "httpget URL [output file]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "URL - HTTP urls are accepted as targets. Proxy is not supported.\n");
    fprintf(stdout, "output file - Optional file name to store URL contents in. Will use stdout if not specified.\n");
}

static void sighandler(int signo)
{
    fprintf(stderr, "Terminated by signal\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int error = 0;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Invalid number of arguments\n");
        usage();
        return EINVAL;
    }

    const char* urlstr = argv[1];
    const char* output = (argc == 3 ? argv[2] : NULL);

    url_t url;
    int sockfd = -1;
    FILE* outfile = stdout;

    error = parse_url(urlstr, &url);
    if (error) {
        return error;
    }

    const char* scheme = (url.scheme ? url.scheme : "http");
    if (0 != strcmp(scheme, "http")) {
        printf("Scheme '%s' is not supported\n", scheme);
        error = ENOTSUP;
        goto out;
    }        

    if (url.username || url.password) {
        printf("Authentication is not supported\n");
        error = ENOTSUP;
        goto out;
    }

    signal(SIGINT, sighandler);

    error = connect_socket(url.host, (url.port ? url.port : "80"), &sockfd);
    if (error) {
        goto out;
    }

    printf("Connected to %s\n", url.host);

    error = send_http_get(&url, sockfd);
    if (error) {
        goto out;
    }

    error = parse_http_reply(sockfd);
    if (error) {
        goto out;
    }

    // Open output stream
    if (output) {
        outfile = fopen(output, "w+");
    }

    if (!outfile) {
        perror("Could not open output file");
        goto out;
    }

    // parse_http_reply will position sockfd at the start of data on success
    // read remaining data
    char buf[1024] = {0};
    while(1) 
    {
        memset(buf, 0, sizeof(buf));
        int nbytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (nbytes == -1) {
            perror("recv failed");
            goto out;
        } 
        else if (nbytes == 0) {
            break;
        }

        fprintf(outfile, "%s", buf);    
    }

out:
    // Cleanup on both error and success passes
    if (sockfd >= 0) {
        close(sockfd);
    }

    if (outfile != stdout) {
        fclose(outfile);
    }

    url_free(&url);
    return error;
}


