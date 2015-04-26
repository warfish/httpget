/**
 *  @brief  URL parsing unit tests
 */

#include "url.h"

#include <stdlib.h>
#include <stdio.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/*************************************************************************************/

static url_parser_t* g_parser = NULL;

static void test_invalid_args(void)
{
    int error = 0;

    // Test basic invalid argument case
    error = url_parse(NULL, NULL, NULL);
    CU_ASSERT_EQUAL(EINVAL, error);

    // Test that url fields are initialized to null in case url string is empty
    url_t url;
    memset(&url, 0xAF, sizeof(url));
    error = url_parse(g_parser, "", &url);

    CU_ASSERT_EQUAL(error, EINVAL);
    CU_ASSERT_EQUAL(url.scheme, NULL);
    CU_ASSERT_EQUAL(url.username, NULL);
    CU_ASSERT_EQUAL(url.password, NULL);
    CU_ASSERT_EQUAL(url.host, NULL);
    CU_ASSERT_EQUAL(url.path, NULL);
    CU_ASSERT_EQUAL(url.args, NULL);
    CU_ASSERT_EQUAL(url.port, NULL);
    CU_ASSERT_EQUAL(url.path, NULL);
    CU_ASSERT_EQUAL(url.args, NULL);
    CU_ASSERT_EQUAL(url.anchor, NULL);
    CU_ASSERT_EQUAL(url.fullpath, NULL);
}

static void test_well_formed_url(void)
{
    int error = 0;

    const char* urlstr = "scheme://username:password@host:80/path/to/stuff?args#anchor";

    url_t url;
    memset(&url, 0, sizeof(url));

    error = url_parse(g_parser, urlstr, &url);
    CU_ASSERT_EQUAL(error, 0);

    CU_ASSERT_TRUE((url.scheme != NULL)     && (0 == strcmp(url.scheme, "scheme")));
    CU_ASSERT_TRUE((url.username != NULL)   && (0 == strcmp(url.username, "username")));
    CU_ASSERT_TRUE((url.password != NULL)   && (0 == strcmp(url.password, "password")));
    CU_ASSERT_TRUE((url.host != NULL)       && (0 == strcmp(url.host, "host")));
    CU_ASSERT_TRUE((url.path != NULL)       && (0 == strcmp(url.path, "/path/to/stuff")));
    CU_ASSERT_TRUE((url.fullpath != NULL)   && (0 == strcmp(url.fullpath, "/path/to/stuff?args#anchor")));
    CU_ASSERT_TRUE((url.args != NULL)       && (0 == strcmp(url.args, "args")));
    CU_ASSERT_TRUE((url.anchor != NULL)     && (0 == strcmp(url.anchor, "anchor")));
    CU_ASSERT_TRUE((url.port != NULL)       && (0 == strcmp(url.port, "80")));

    url_free(&url);
}

/*
 * Tests for a common user pattern - omitting scheme, username, password and port
 */
static void test_typical_http_url(void)
{
    int error = 0;

    const char* urlstr = "host/path/to/stuff?args#anchor";

    url_t url;
    memset(&url, 0, sizeof(url));

    error = url_parse(g_parser, urlstr, &url);
    CU_ASSERT_EQUAL(error, 0);

    CU_ASSERT_TRUE((url.host != NULL)       && (0 == strcmp(url.host, "host")));
    CU_ASSERT_TRUE((url.path != NULL)       && (0 == strcmp(url.path, "/path/to/stuff")));
    CU_ASSERT_TRUE((url.fullpath != NULL)   && (0 == strcmp(url.fullpath, "/path/to/stuff?args#anchor")));
    CU_ASSERT_TRUE((url.args != NULL)       && (0 == strcmp(url.args, "args")));
    CU_ASSERT_TRUE((url.anchor != NULL)     && (0 == strcmp(url.anchor, "anchor")));
    CU_ASSERT_TRUE(url.scheme == NULL);
    CU_ASSERT_TRUE(url.username == NULL);
    CU_ASSERT_TRUE(url.password == NULL);
    CU_ASSERT_TRUE(url.port == NULL);

    url_free(&url);
}

/*
 * Some real world URLs
 */
static void test_examples(void)
{
    int error = 0;

    url_t url;
    memset(&url, 0, sizeof(url));

    fprintf(stderr, "\nwww.google.com\n");

    error = url_parse(g_parser, "www.google.com", &url);
    CU_ASSERT_EQUAL(error, 0);

    CU_ASSERT_TRUE(url.scheme == NULL);
    CU_ASSERT_TRUE(url.username == NULL);
    CU_ASSERT_TRUE(url.password == NULL);
    CU_ASSERT_TRUE((url.host != NULL) && (0 == strcmp(url.host, "www.google.com")));
    CU_ASSERT_TRUE(url.path == NULL);
    CU_ASSERT_TRUE(url.fullpath == NULL);
    CU_ASSERT_TRUE(url.args == NULL);
    CU_ASSERT_TRUE(url.anchor == NULL);
    CU_ASSERT_TRUE(url.port == NULL);

    url_free(&url);

    fprintf(stderr, "\nroot@192.168.0.1\n");

    error = url_parse(g_parser, "root@192.168.0.1", &url);
    CU_ASSERT_EQUAL(error, 0);

    CU_ASSERT_TRUE(url.scheme == NULL);
    CU_ASSERT_TRUE((url.username != NULL) && (0 == strcmp(url.username, "root")));
    CU_ASSERT_TRUE(url.password == NULL);
    CU_ASSERT_TRUE((url.host != NULL) && (0 == strcmp(url.host, "192.168.0.1")));
    CU_ASSERT_TRUE(url.path == NULL);
    CU_ASSERT_TRUE(url.fullpath == NULL);
    CU_ASSERT_TRUE(url.args == NULL);
    CU_ASSERT_TRUE(url.anchor == NULL);
    CU_ASSERT_TRUE(url.port == NULL);

    url_free(&url);

    fprintf(stderr, "\nhttp://www.w3.org/Protocols/rfc2616/rfc2616.html\n");

    error = url_parse(g_parser, "http://www.w3.org/Protocols/rfc2616/rfc2616.html", &url);
    CU_ASSERT_EQUAL(error, 0);
    CU_ASSERT_TRUE((url.scheme != NULL) && (0 == strcmp(url.scheme, "http")));
    CU_ASSERT_TRUE(url.username == NULL);
    CU_ASSERT_TRUE(url.password == NULL);
    CU_ASSERT_TRUE((url.host != NULL) && (0 == strcmp(url.host, "www.w3.org")));
    CU_ASSERT_TRUE((url.path != NULL) && (0 == strcmp(url.path, "/Protocols/rfc2616/rfc2616.html")));
    CU_ASSERT_TRUE((url.fullpath != NULL) && (0 == strcmp(url.fullpath, "/Protocols/rfc2616/rfc2616.html")));
    CU_ASSERT_TRUE(url.args == NULL);
    CU_ASSERT_TRUE(url.anchor == NULL);
    CU_ASSERT_TRUE(url.port == NULL);

    url_free(&url);
}

int main(void)
{
    int error = 0;

    error = url_parser_init_default(&g_parser);
    if (error) {
        return error;
    }

    error = CU_initialize_registry();
    if (error) {
        goto error_out;
    }

    CU_pSuite suite = CU_add_suite("URL", NULL, NULL);
    if (!suite) {
        error = CU_get_error();
        goto error_out;
    }


    CU_add_test(suite, "invalid args", test_invalid_args);
    CU_add_test(suite, "well formed url", test_well_formed_url);
    CU_add_test(suite, "typical HTTP url", test_typical_http_url);
    CU_add_test(suite, "examples", test_examples);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    error = CU_get_error();

error_out:
    CU_cleanup_registry();
    url_parser_free(g_parser);
    
    return error;
}
