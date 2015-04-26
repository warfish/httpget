/**
 * @file url.h
 *  
 * URL parsing 
 */

#ifndef _HTTPGET_URL_H_
#define _HTTPGET_URL_H_

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @brief   URL parser opaque context
 */
typedef struct url_parser url_parser_t;


/**
 * @brief Decomposed URL 
 */
typedef struct url
{
    const char* scheme;
    const char* username;
    const char* password;
    const char* host;
    const char* path;
    const char* args;
    const char* anchor;
    const char* port;     
    const char* fullpath;   // path + args + anchor in one string
} url_t;

/**
 * @brief       Init default URL parser internal state.
 *              Separate context is convinient for unit tests and multithreaded environments. 
 *
 * @out_parser  On success will contain pointer to initialized parser.
 *              Caller is responsible to free it using @url_parser_free@
 *
 * @returns     0 on success, ENOMEM if there was not enough memory.
 */
int url_parser_init_default(url_parser_t** out_parser);

/**
 * @brief       Free all resources associated with this parser.
 */
void url_parser_free(url_parser_t* parser);

/**
 * @brief       Parse URL string into decomposed structure
 *
 *              This function accepts a URL string and decomposes it into components: 
 *              <scheme>://<username>:<password>@<host>:<port>/<path>?<args>#<anchor>
 *              Everything except <host> is optional in which case corresponding fields will contain NULL pointers
 *
 * @string      Input string containing a URL 
 * @out_url     On success decomposed URL will be returned in this buffer.
 *              Caller is responsible to release it using @url_free@
 *
 * @returns     0 on success
 *              EINVAL if @string@ is not a valid URL
 *              ENOMEM if there was no memory
 */
int url_parse(url_parser_t* parser, const char* string, url_t* out_url);

/**
 * @brief       Release resources allocated for this URL structure.
 */
void url_free(url_t* url);

#ifdef __cplusplus
}
#endif
#endif
