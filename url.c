#include "url.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <pcre.h>

/*******************************************************************************/

#if !defined(countof)
#   define countof(_arr_)   (sizeof((_arr_)) / sizeof(*(_arr_))) 
#endif 


/*
 * Regex string below is a modified version of the one found in RFC 2396 Appendix B
 * Modified to support optional scheme, username and password
 */
//static const char* g_url_regex_str = "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?";
static const char* g_url_regex_str   = "^(([^:/?#]+)://)?(([^/?#]*):([^/?#]*)@)?([^:/?#]*)?(:([\\d]+))?([^?#]*)(\\?([^#]*))?(#(.*))?";
//                                       12              34         5           6          7 8         9       10  11       1213

/*
 * Match string indexes for above url regex
 */
enum 
{
    URL_MATCH_SCHEME = 2,
    URL_MATCH_USERNAME = 4,
    URL_MATCH_PASSWORD = 5,
    URL_MATCH_HOST = 6,
    URL_MATCH_PORT = 8,
    URL_MATCH_PATH = 9,
    URL_MATCH_ARGS = 11,
    URL_MATCH_ANCHOR = 13, 

    URL_MATCH_MAX = 14
};


/*
 * Default parser
 */
struct url_parser
{
    pcre* re;
    pcre_extra* re_study;
};


/**
 * @brief       Init default URL parser internal state.
 *              Separate context helps in multithreaded environments. 
 *              Also allows for other, more specialized and faster, implementations.
 *
 * @returns     0 on success, ENOMEM if there was not enough memory.
 */
int url_parser_init_default(url_parser_t** out_parser)
{
    if (!out_parser) {
        return EINVAL;
    }

    const char* error_str = NULL;
    int error_offset = 0;

    url_parser_t* parser = (url_parser_t*) calloc(1, sizeof(*parser)); // Calloc zeroes memory which is handy
    if (!parser) {
        return ENOMEM;
    }

    parser->re = pcre_compile(g_url_regex_str, PCRE_CASELESS, &error_str, &error_offset, NULL);
    if (!parser->re) {
        fprintf(stderr, "Could not compile URL regex: %s\n", error_str);
        url_parser_free(parser);
        return EINVAL;
    }

    // It's ok if re_study is NULL
    parser->re_study = pcre_study(parser->re, 0, &error_str);
    if (error_str) {
        fprintf(stderr, "Could not optimize URL regex: %s\n", error_str);
        url_parser_free(parser);
        return EINVAL;
    }

    *out_parser = parser;
    return 0;
}


/**
 * @brief       Free all resources associated with this parser.
 *              NOTE: This function may be called with not fully initialized parsers
 */
void url_parser_free(url_parser_t* parser)
{
    if (parser) 
    {
        if (parser->re_study) {
            pcre_free_study(parser->re_study);
        }

        if (parser->re) {
            pcre_free(parser->re);
        }

        memset(parser, 0, sizeof(*parser));
        free(parser);
    }
}


/*
 * A wrapper for pcre_get_substring
 * Will return a NULL string pointer if substring matched to "" empty string
 */
static int get_nonempty_substring(const char* subject, int* ovector, int stringcount, int stringnumber, const char** stringptr)
{
    const char* str = NULL;
    int error = pcre_get_substring(subject, ovector, stringcount, stringnumber, &str);
    if (error < 0) {
        switch(error) {
        case PCRE_ERROR_NOSUBSTRING : return EINVAL;
        case PCRE_ERROR_NOMEMORY    : return ENOMEM;
        default: assert(error >= 0);
        }
    }

    // Return NULL pointer instead of an empty string
    if (str && (0 == strlen(str))) {
        pcre_free_substring(str);
        str = NULL;
    }

    *stringptr = str;
    return 0;
}


/**
 * @brief       Prase URL string into decomposed structure
 *              This function accepts a URL string and decomposes it into components: 
 *              <scheme>://<username>:<password>@<host>:<port>/<path>?<args>#<anchor>
 *              String should be a well-formed URL except that <scheme> can be ommitted in which case http will be assumed.
 *
 * @string      Input string containing a URL 
 * @out_url     On success decomposed URL will be returned in this buffer.
 *              Caller is responsible to release it using @url_free@
 *
 * @returns     0 on success
 */
int url_parse(url_parser_t* parser, const char* urlstr, url_t* out_url)
{
    int error = 0;

    if (!parser || !parser->re) {
        return EINVAL;
    }

    if (!urlstr) {
        return EINVAL;
    }

    if (!out_url) {
        return EINVAL;
    }

    memset(out_url, 0, sizeof(*out_url));

    int matchvec[URL_MATCH_MAX * 3] = {0}; // PCRE wants 3 elements per match 
    int nmatches = pcre_exec(parser->re, parser->re_study, urlstr, strlen(urlstr), 0, PCRE_NOTEMPTY, matchvec, countof(matchvec));
    if (nmatches < 0) {
        switch(nmatches) {
        case PCRE_ERROR_NOMATCH      : return EINVAL;
        case PCRE_ERROR_NOMEMORY     : return ENOMEM;
        default                      : return EIO;
        }
    }
    
    // Something matched, decompose
    //for(size_t i = 0; i < MAX_MATCHES; ++i) {
    //    const char* match = NULL;
    //    pcre_get_substring(urlstr, matchvec, nmatches, i, &match);
    //    printf("[%ld] \'%s\'\n", i, match);
    //    pcre_free_substring(match);
    //}

    #define get_nonempty_substring_or_die(_subject_, _ovector_, _stringcount_, _stringnumber_, _stringptr_) \
        error = get_nonempty_substring(_subject_, _ovector_, _stringcount_, _stringnumber_, _stringptr_);   \
        if (error) {                                                                                        \
            goto error_out;                                                                                 \
        }                                                                                                   \
    
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_SCHEME, &out_url->scheme);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_USERNAME, &out_url->username);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PASSWORD, &out_url->password);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_HOST, &out_url->host);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PATH, &out_url->path);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_ARGS, &out_url->args);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_ANCHOR, &out_url->anchor);

    const char* portstr = NULL;
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PORT, &portstr);    
    if (portstr) {
        out_url->port = atol(portstr);
        pcre_free_substring(portstr);
    }

    // All done
    return 0;

error_out: // get_nonempty_substring_or_die will jump here so we can have a breakpoint on error handling outside the macro
    url_free(out_url);
    return error;
}

/**
 * @brief       Release resources associated with this URL structure.
 */
void url_free(url_t* url)
{
    assert(url != NULL);

    if (!url) {
        return;
    }

    #define url_free_kill_substring(_substringptr_) \
        if ((_substringptr_) != NULL) {             \
            pcre_free_substring((_substringptr_));  \
            (_substringptr_) = NULL;                \
        }                                           \

    url_free_kill_substring(url->scheme);
    url_free_kill_substring(url->username);
    url_free_kill_substring(url->password);
    url_free_kill_substring(url->host);
    url_free_kill_substring(url->path);
    url_free_kill_substring(url->args);
    url_free_kill_substring(url->anchor);
    url->port = 0;

    #undef url_free_kill_substring
}

/*******************************************************************************/

