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
    CU_ASSERT_EQUAL(url.port, 0);
    CU_ASSERT_EQUAL(url.path, NULL);
    CU_ASSERT_EQUAL(url.args, NULL);
    CU_ASSERT_EQUAL(url.anchor, NULL);
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
    CU_ASSERT_TRUE((url.args != NULL)       && (0 == strcmp(url.args, "args")));
    CU_ASSERT_TRUE((url.anchor != NULL)     && (0 == strcmp(url.anchor, "anchor")));
    CU_ASSERT_EQUAL(url.port, 80);

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
    CU_ASSERT_TRUE((url.args != NULL)       && (0 == strcmp(url.args, "args")));
    CU_ASSERT_TRUE((url.anchor != NULL)     && (0 == strcmp(url.anchor, "anchor")));

    CU_ASSERT_TRUE(url.scheme == NULL);
    CU_ASSERT_TRUE(url.username == NULL);
    CU_ASSERT_TRUE(url.password == NULL);
    CU_ASSERT_EQUAL(url.port, 0);

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

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    error = CU_get_error();

error_out:
    CU_cleanup_registry();
    url_parser_free(g_parser);
    
    return error;
}
