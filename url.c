#include "url.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <pcre.h>

/*************************************************************************************/

#if !defined(countof)
#   define countof(_arr_)   (sizeof((_arr_)) / sizeof(*(_arr_))) 
#endif 

/*
 * Regex string below is a modified version of the one found in RFC 2396 Appendix B
 * Modified to support optional scheme, username and password and also makes host field mandatory
 */
//                                    012              34         5 6             7         8 9         AB        C   D        E F  
static const char* g_url_regex_str = "^(([^:/?#]+)://)?(([^:/?#]*)(:([^/?#]*))?@)?([^:/?#]+)(:([\\d]+))?(([^?#]*)?(\\?([^#]*))?(#([^#]*))?)?$";

/*
 * Match string indexes for above url regex
 */
enum 
{
    URL_MATCH_SCHEME    = 2,
    URL_MATCH_USERNAME  = 4,
    URL_MATCH_PASSWORD  = 6,
    URL_MATCH_HOST      = 7,
    URL_MATCH_PORT      = 9,
    URL_MATCH_FULLPATH  = 10,
    URL_MATCH_PATH      = 11,
    URL_MATCH_ARGS      = 13,
    URL_MATCH_ANCHOR    = 15, 

    URL_MATCH_MAX       = 16
};

/*
 * Default parser
 */
struct url_parser
{
    pcre* re;
    pcre_extra* re_study;
};

/*************************************************************************************/

int url_parser_init_default(url_parser_t** out_parser)
{
    if (!out_parser) {
        return EINVAL;
    }

    const char* error_str = NULL;
    int error_offset = 0;

    url_parser_t* parser = (url_parser_t*) calloc(1, sizeof(*parser));
    if (!parser) {
        return ENOMEM;
    }

    parser->re = pcre_compile(g_url_regex_str, PCRE_CASELESS, &error_str, &error_offset, NULL);
    if (!parser->re) {
        fprintf(stderr, "Could not compile URL regex: %s\n", error_str);
        goto error_out;
    }

    // It's ok if study is NULL
    parser->re_study = pcre_study(parser->re, 0, &error_str);
    if (error_str) {
        fprintf(stderr, "Could not optimize URL regex: %s\n", error_str);
        goto error_out;
    }

    *out_parser = parser;
    return 0;

error_out:
    url_parser_free(parser);
    return -1;
}

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
 * Will return a NULL string pointer if substring matched to "" empty string indicating no match
 */
static int get_nonempty_substring(const char* subject, int* ovector, int stringcount, int stringnumber, const char** stringptr)
{
    const char* str = NULL;
    int error = pcre_get_substring(subject, ovector, stringcount, stringnumber, &str);
    if (error < 0) {
        switch(error) {
        case PCRE_ERROR_NOSUBSTRING : break; // Substring was not matched at all which is a valid case for us
        case PCRE_ERROR_NOMEMORY    : return ENOMEM;
        default                     : return -1;
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
    int nmatches = pcre_exec(parser->re, parser->re_study, urlstr, strlen(urlstr), 0, 0, matchvec, countof(matchvec));
    if (nmatches < 0) {
        switch(nmatches) {
        case PCRE_ERROR_NOMATCH      : return EINVAL;
        case PCRE_ERROR_NOMEMORY     : return ENOMEM;
        default                      : return -1;
        }
    }

    /*
    for(size_t i = 0; i < URL_MATCH_MAX; ++i) {
        const char* match = NULL;
        pcre_get_substring(urlstr, matchvec, nmatches, i, &match);
        fprintf(stderr, "[%ld] \'%s\'\n", i, match);
        pcre_free_substring(match);
    }
    */
    
    #define get_nonempty_substring_or_die(_subject_, _ovector_, _stringcount_, _stringnumber_, _stringptr_) \
        error = get_nonempty_substring(_subject_, _ovector_, _stringcount_, _stringnumber_, _stringptr_);   \
        if (error) {                                                                                        \
            goto error_out;                                                                                 \
        }                                                                                                   \
    
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_HOST, &out_url->host);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_SCHEME, &out_url->scheme);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_USERNAME, &out_url->username);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PASSWORD, &out_url->password);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PATH, &out_url->path);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_ARGS, &out_url->args);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_ANCHOR, &out_url->anchor);
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_PORT, &out_url->port);    
    get_nonempty_substring_or_die(urlstr, matchvec, nmatches, URL_MATCH_FULLPATH, &out_url->fullpath);

    #undef get_nonempty_substring_or_die

    // Postcondition: host field is always set otherwise regex match should've failed
    assert(out_url->host != NULL);

    // All done
    return 0;

    // get_nonempty_substring_or_die will jump here so we can have a breakpoint on error handling outside the macro
error_out: 
    url_free(out_url);
    return error;
}

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
    url_free_kill_substring(url->port);
    url_free_kill_substring(url->fullpath);

    #undef url_free_kill_substring
}

/*************************************************************************************/
