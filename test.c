#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

/* base */
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {                    \
        test_count++;       \
        if (equality)       \
            test_pass++;    \
        else {              \
            fprintf(stderr, \
                    "%s:%d: expect: " format " actual: " format "\n",   \
                    __FILE__, __LINE__, \
                    expect, actual );   \
            main_ret = 1;               \
        }       \
    } while(0)

/* define */
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE( sizeof(expect) - 1 == alength && memcmp((expect), actual, alength) == 0,    \
                    expect, actual, "%s" )
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%ld")

#define TEST_ERROR(error, json)  \
    do {   \
        lept_value v;   \
        lept_init(&v);  \
        v.type = LEPT_FALSE;   \
        EXPECT_EQ_INT(error, lept_parse(&v, json));   \
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));  \
        lept_free(&v);  \
    }while(0)

#define TEST_NUMBER(expect, json) \
    do {    \
        lept_value v;   \
        lept_init(&v);  \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));   \
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));        \
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));        \
        lept_free(&v);  \
    }while(0)

#define TEST_STRING(expect, json) \
    do {    \
        lept_value v;   \
        lept_init(&v);  \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));      \
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));  \
        lept_free(&v);  \
    }while(0)

#define TEST_ROUNDTRIP(json) \
    do { \
        lept_value v;   \
        char* json2;    \
        size_t length;  \
        lept_init(&v);  \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));  \
        EXPECT_EQ_INT(LEPT_STRINGIFY_OK, lept_stringify(&v, &json2, &length));  \
        EXPECT_EQ_STRING(json, json2, length);  \
        lept_free(&v);  \
        free(json2);    \
    }while(0)

#define TEST_ROUNDTRIP_REAL(json) \
    do { \
        lept_value v1, v2;  \
        char* json2;    \
        size_t length;  \
        lept_init(&v1); \
        lept_init(&v2); \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v1, json));    \
        EXPECT_EQ_INT(LEPT_STRINGIFY_OK, lept_stringify(&v1, &json2, &length));  \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v2, json2));   \
        EXPECT_TRUE(lept_is_equal(&v1, &v2));   \
        lept_free(&v1); \
        lept_free(&v2); \
        free(json2);    \
    } while(0)


/*** define end ***/


static void test_parse_null() {
    lept_value v;
    lept_init(&v);
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_invalid_value() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    lept_value v;
    lept_init(&v);
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_false() {
    lept_value v;
    lept_init(&v);
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));

    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "fallse"));
    lept_free(&v);
}

static void test_parse_true() {
    lept_value v;
    lept_init(&v);
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
    
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "tru"));
    lept_free(&v);
}

static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

/***** test number *****/

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000");        /* must underflow */
    TEST_NUMBER(12.2e002, "12.2e002");
    TEST_NUMBER(12.2e00, "12.2e00000");
    /* the smallest number > 1 */
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002");
    /* minimum denormal */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324");
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    /* Max subnormal double */
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    /* Min normal positive double */
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    /* Max double */
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_number_too_big() {
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e10000");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e10000");
}

/************ test string **************/

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000\x1fG\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

/********** test array **********/

static void test_parse_array() {
    lept_value v;
    size_t i, j;
    
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(1, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ \"abc\" , 123, true,  false ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ \"abc\" , 123, true,  false, [1,2,3,4,5] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", 
                    lept_get_string(lept_get_array_element(&v, 4)), 
                    lept_get_string_length(lept_get_array_element(&v, 4)) );
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    for (i = 0; i < 4; i++) {
        lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
        for (j = 0;j < i; j++) {
            lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));
        }
    }
    lept_free(&v);
}

/*********** test object *************/

static void test_parse_object() {
    lept_value v;
    size_t i;
    lept_value* tmp;

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
    lept_free(&v);
    
    /* tests 1 */
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " {\"1\":1} "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(1, lept_get_object_size(&v));
    lept_free(&v);

    /* tests 1 */
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "      { \"a\":[1,2,3,4]}     "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(1, lept_get_object_size(&v));
    
    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    tmp = lept_get_object_value(&v, 0);
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(tmp));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(tmp));
    lept_free(&v);
    
    /* test 2 */
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, 
                            "    { "
                            " \"n\" : null , "  
                            " \"f\" : false , "
                            " \"t\" : true , "
                            " \"i\" : 1234 , "
                            " \"s\" : \"abcd1234\" , "
                            " \"a\" : [ 0, 1, 2, 3, 4 , 5] , "
                            " \"ooo\" : {\"0\":0,   \"1\" : 1 , \"2\" : 2, \"3\" : 3  }     , "
                            " \"test\\u0000test\": {\"0\":0}   " 
                            " } "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(8, lept_get_object_size(&v));

    EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    tmp = lept_get_object_value(&v, 0);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(tmp));

    EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
    tmp = lept_get_object_value(&v, 1);
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(tmp));

    EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
    tmp = lept_get_object_value(&v, 2);
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(tmp));

    EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
    tmp = lept_get_object_value(&v, 3);
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(tmp));
    EXPECT_EQ_DOUBLE(1234.0, lept_get_number(tmp));

    EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
    tmp = lept_get_object_value(&v, 4);
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(tmp));
    EXPECT_EQ_STRING("abcd1234", lept_get_string(tmp), lept_get_string_length(tmp));

    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
    tmp = lept_get_object_value(&v, 5);
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(tmp));
    EXPECT_EQ_SIZE_T(6, lept_get_array_size(tmp));
    for (i = 0; i<= 5; i++) {
        lept_value* tmp2 = lept_get_array_element(tmp, i);
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(tmp2));
        EXPECT_EQ_DOUBLE((double)i, lept_get_number(tmp2)); 
    }

    EXPECT_EQ_STRING("ooo", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6)); 
    tmp = lept_get_object_value(&v, 6);
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(tmp));
    EXPECT_EQ_SIZE_T(4, lept_get_object_size(tmp));
    for (i = 0; i <= 3; i++) {
        lept_value* ov = lept_get_object_value(tmp, i);
        EXPECT_TRUE(('0'+i) == lept_get_object_key(tmp, i)[0] );
        EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(tmp, i));
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
        EXPECT_EQ_DOUBLE((double)i, lept_get_number(ov));
    }
    /* test lept_find_object_value() */
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_find_object_value(&v, "n", 1)) );
    EXPECT_TRUE(NULL == lept_find_object_value(&v, "ff", 2));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(lept_find_object_value(&v, "test\0test", 9)) );
     
    lept_free(&v);

}


/*********** access test *************/

static void test_access_string() {
    lept_value v;
    lept_init(&v);
    /* test  */
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v) );
    lept_set_string(&v, "hello", 5);
    EXPECT_EQ_STRING("hello", lept_get_string(&v), lept_get_string_length(&v) );
    lept_set_string(&v, "hello world", 11);
    EXPECT_EQ_STRING("hello world", lept_get_string(&v), lept_get_string_length(&v) );
    /* test end */
    lept_free(&v);
}

static void test_access_number() {  /* test getter and setter */
    lept_value v;
    lept_init(&v);

    lept_set_string(&v, "a", 1);
    EXPECT_EQ_STRING("a", lept_get_string(&v), lept_get_string_length(&v) );
    lept_set_number(&v, 1234.1);
    EXPECT_EQ_DOUBLE(1234.1, lept_get_number(&v));

    lept_free(&v);
}

static void test_access_boolean() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "haha", 4);
    lept_set_boolean(&v, 1 == 1);
    EXPECT_TRUE(lept_get_boolean(&v));
    lept_set_boolean(&v, 1 != 1);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_null() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_null(&v);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

/***** main test function ****/
static void test_parse() {
    
    test_parse_null();
    test_parse_false();
    test_parse_true();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_expect_value();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();

}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}
static void test_stringify() {
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("false");
    /* other */
    TEST_ROUNDTRIP("{\"0\":0,\"1\":1}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":1234,\"s\":\"abcd1234\",\"a\":[0,1,2,3,{\"0\":0,\"1\":1},4],\"o\":{\"0\":0,\"1\":1,\"2\":2,\"3\":3}}");
    /* other */
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

static void test_roundtrip_real() {
    TEST_ROUNDTRIP_REAL("123.1");
    TEST_ROUNDTRIP_REAL("\"HAHAHAHAHAHA\"");
    TEST_ROUNDTRIP_REAL("{\"0\":0,\"1\":1}");
    TEST_ROUNDTRIP_REAL("true");
    TEST_ROUNDTRIP_REAL("false");
    TEST_ROUNDTRIP_REAL("{\"n\":null,\"f\":false,\"t\":true,\"i\":1234,\"s\":\"abcd1234\",\"a\":[0,1,2,3,{\"0\":0,\"1\":1},4],\"o\":{\"0\":0,\"1\":1,\"2\":2,\"3\":3}}");
}


int main() {
    test_parse();
    test_access();
    test_stringify();
    test_roundtrip_real();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
