#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <string.h>  /* memcpy() */
#include <stdio.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

#define PUTC(c, ch) do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
#define PUTS(c, s, len)     memcpy(lept_context_push(c, len), s, len)

typedef struct {
    const char* first;
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

#if 0
static void printCur(lept_context* c) {
    /* 13 */
    printf("[... %c%c%c%c%c%c%c%c%c%c%c%c%c ...]\n",
           *(c->json-6), *(c->json-5), *(c->json-4),
           *(c->json-3), *(c->json-2), *(c->json-1),
           *(c->json),
           *(c->json+1), *(c->json+2), *(c->json+3),
           *(c->json+4), *(c->json+5), *(c->json+6)
           );
}
#endif
static int lept_parse_value(lept_context* c, lept_value* v); /* forward declare */

void lept_free(lept_value* v) {
    size_t i;
    assert(v != NULL);
    switch(v->type) {
        case LEPT_STRING:
            free(v->u.s.s);
            break;
        case LEPT_ARRAY:
            for(i = 0; i < v->u.a.size; i++) {
                lept_free(&v->u.a.e[i]);
            }
            free(v->u.a.e);
            v->u.a.e = NULL;
            v->u.a.size = 0;
            break;
        case LEPT_OBJECT:
            for (i = 0; i < v->u.o.size; i++ ) {
                free(v->u.o.m[i].k);
                lept_free(&(v->u.o.m[i].v));
            }
            free(v->u.o.m);
            v->u.o.m = NULL;
            v->u.o.size = 0;
            break;
        default: 
            break;
    }
    v->type = LEPT_NULL;
}


/****** getter setter ******/

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE) );
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double b) {
    lept_free(v);
    v->u.n = b;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char*s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0)) ;
    lept_free(v);
    v->u.s.s = (char*)malloc(len+1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

size_t      lept_get_array_size(const lept_value* v) {
    assert( v!= NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v , size_t index) {
    assert( v!= NULL && v->type == LEPT_ARRAY);
    assert (index < v->u.a.size);
    return &(v->u.a.e[index]);
}

size_t      lept_get_object_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.size;
}

const char* lept_get_object_key(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert( index < v->u.o.size );
    return (v->u.o.m[index]).k;
}

size_t      lept_get_object_key_length(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert( index < v->u.o.size );
    return (v->u.o.m[index]).klen;
}

lept_value* lept_get_object_value(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert( index < v->u.o.size );
    return &((v->u.o.m[index]).v);
}

int         lept_find_object_index(const lept_value* v, const char* key, size_t klen) {
    int i;
    assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
    for(i = 0;i < v->u.o.size; i++) {
        if ( klen == v->u.o.m[i].klen && memcmp(v->u.o.m[i].k, key, klen) == 0) {
            return i;
        }
    }
    return -1;
}

lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen) {
    int i ;
    assert(v!=NULL && v->type == LEPT_OBJECT && key  != NULL);
    for (i = 0; i< v->u.o.size; i++) {
        if ((v->u.o.m[i]).klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0 ) {
            return &((v->u.o.m[i]).v);
        }
    }
    return NULL;
}

/* return position of data which was push in */
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    void* tmp;
    int i = 3;
    tmp = NULL;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0) {
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        }
        while(c->top + size >= c->size) {
            c->size += (c->size >> 1);      /* c->size * 1.5 */
        }
        while (i--) {
            tmp = realloc(c->stack, c->size);
            if (tmp != NULL) {
                break;
            }
        }
        assert(tmp != NULL);
        c->stack = tmp;
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, int type) {
    size_t i = 0;
    EXPECT(c, literal[0]);
    for(i = 0; literal[i+1]; i++) {
        if (c->json[i]  != literal[i+1] ) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    /* /todo parse number */
    const char * p = c->json;
    char state = 'A';
    int parsing = 1;
    while(parsing) {
        switch(state) {
            case 'A':
                if (*p == '-'){
                    p++;
                    state = 'B';
                    break;
                }
                if ( ISDIGIT1TO9(*p) ) {
                    p++;
                    state = 'C';
                    break;
                }
                if ( *p == '0') {
                    p++;
                    state = 'D';
                    break;
                }
                return LEPT_PARSE_INVALID_VALUE;
            case 'B':
                if (*p == '0') {
                    p++;
                    state = 'D';
                    break;
                }
                if ( ISDIGIT1TO9(*p) ) {
                    p++;
                    state = 'C';
                    break;
                }
                return LEPT_PARSE_INVALID_VALUE;
            case 'C':
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'C';
                    break;
                }
                if (*p == '.') {
                    p++;
                    state = 'E';
                    break;
                }
                if (*p == 'e' || *p == 'E') {
                    p++;
                    state = 'G';
                    break;
                }
                state = 'F';
                break;
            case 'D':
                if (*p == '.') {
                    p++;
                    state = 'E';
                    break;
                }
                if (*p == 'e' || *p == 'E') {
                    p++;
                    state = 'G';
                    break;
                }
                state = 'F';
                break;
            case 'E':
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'H';
                    break;
                }
                return LEPT_PARSE_INVALID_VALUE;
            case 'F':
                parsing = 0;
                break;
            case 'G':
                if ( *p == '-' || *p == '+') {
                    p++;
                    state = 'I';
                    break;
                }
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'J';
                    break;
                }
                return LEPT_PARSE_INVALID_VALUE;
            case 'H':
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'H';
                    break;
                }
                if (*p == 'e' || *p == 'E') {
                    p++;
                    state = 'G';
                    break;
                }
                state = 'F';
                break;
            case 'I':
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'J';
                    break;
                }
                return LEPT_PARSE_INVALID_VALUE;
            case 'J':
                if ( ISDIGIT(*p) ) {
                    p++;
                    state = 'J';
                    break;
                }
                state = 'F';
                break;
            default :
                return LEPT_PARSE_INVALID_VALUE;
        }
    }

	/* end */
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL) ) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    if (c->json == p ) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static const char* lept_parse_hex4(const char* p, unsigned int *u) {
    int i;
    assert( p!= NULL && u != NULL);
    *u = 0;
    for(i = 0; i < 4; i++) {
        char ch = (*p ++);
        *u <<= 4;
        if ( ch >= '0' && ch <= '9'){ *u += (ch - '0'); }
        else if ( ch >= 'a' && ch <= 'f'){ *u += (ch - 'a' + 10); }
        else if ( ch >= 'A' && ch <= 'F'){ *u += (ch - 'A' + 10); }
        else {
            return NULL;
        }
    }
    return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
    assert(c!= NULL && u>= 0x00 && u <= 0x10FFFF);
    if ( u <= 0x7F) {
        PUTC(c, u & 0x7F);
    }else if (u <= 0x7FF) {
        PUTC(c, ((u >> 6) & 0x1F) | 0xC0);
        PUTC(c, ( u       & 0x3F) | 0x80);
    }else if (u <= 0xFFFF) {
        PUTC(c, ((u >> 12) & 0xF ) | 0xE0);
        PUTC(c, ((u >> 6 ) & 0x3F) | 0x80);
        PUTC(c, ( u        & 0x3F) | 0x80);
    }else {
        assert(u<= 0x10FFFF);
        PUTC(c, ((u >> 18) & 0x7 ) | 0xF0);
        PUTC(c, ((u >> 12) & 0x3F) | 0x80);
        PUTC(c, ((u >> 6 ) & 0x3F) | 0x80);
        PUTC(c, ( u        & 0x3F) | 0x80);
    }
}

/* if error when parsing string, stack must roll back */
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
    size_t head;
    unsigned u, u2;
    const char* p;
    assert(c!=NULL && str!=NULL && len!=NULL);
    *len = 0;
    head = c->top;
    EXPECT(c, '\"');
    p = c->json;
    while(1) {
        char ch = *p++;
        switch(ch) {
            case '\"':
                *len = c->top - head;
                *str = (char*)lept_context_pop(c, *len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                STRING_ERROR( LEPT_PARSE_MISS_QUOTATION_MARK);
            case '\\':
                ch = *p++;
                switch(ch) {
                    case '\"': PUTC(c, '\"');break;
                    case '\\': PUTC(c, '\\');break;
                    case '/' : PUTC(c, '/');break;
                    case 'b' : PUTC(c, '\b');break;
                    case 'f' : PUTC(c, '\f');break;
                    case 'n' : PUTC(c, '\n');break;
                    case 'r' : PUTC(c, '\r');break;
                    case 't' : PUTC(c, '\t');break;
                    case 'u' :
                        if ( (p = lept_parse_hex4(p, &u)) == NULL ) {
                            STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_HEX );
                        }
                        if (u >= 0xD800 && u <= 0xDBFF ) {
                            if ( *p++ != '\\' || *p++ != 'u' ) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_SURROGATE );
                            }
                            if ( (p = lept_parse_hex4(p, &u2)) == NULL ) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_HEX );
                            }
                            if ( u2 < 0xDC00 || u2 > 0xDFFF) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_SURROGATE );
                            }
                            u = (0x10000 + ((u - 0xD800) << 10) + u2 - 0xDC00);
                        }
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR( LEPT_PARSE_INVALID_STRING_ESCAPE );
                }
                break;
            default:
                if ((unsigned char)ch < 0x20) {  /* must change to unsigned char */
                    STRING_ERROR( LEPT_PARSE_INVALID_STRING_CHAR );
                }
                PUTC(c, ch);
                break;
        }/*end switch*/
    }/*end while*/
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK) {
        lept_set_string(v, s, len);
    }
    return ret;
}

#if 0
static int lept_parse_string_old(lept_context* c, lept_value* v) {
    size_t head = c->top;
    size_t len;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    while(1) {
        char ch = *p++;
        switch(ch) {
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                STRING_ERROR( LEPT_PARSE_MISS_QUOTATION_MARK);
            case '\\':
                ch = *p++;
                switch(ch) {
                    case '\"': PUTC(c, '\"');break;
                    case '\\': PUTC(c, '\\');break;
                    case '/' : PUTC(c, '/');break;
                    case 'b' : PUTC(c, '\b');break;
                    case 'f' : PUTC(c, '\f');break;
                    case 'n' : PUTC(c, '\n');break;
                    case 'r' : PUTC(c, '\r');break;
                    case 't' : PUTC(c, '\t');break;
                    case 'u' :
                        if ( (p = lept_parse_hex4(p, &u)) == NULL ) {
                            STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_HEX );
                        }
                        if (u >= 0xD800 && u <= 0xDBFF ) {
                            if ( *p++ != '\\' || *p++ != 'u' ) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_SURROGATE );
                            }
                            if ( (p = lept_parse_hex4(p, &u2)) == NULL ) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_HEX );
                            }
                            if ( u2 < 0xDC00 || u2 > 0xDFFF) {
                                STRING_ERROR( LEPT_PARSE_INVALID_UNICODE_SURROGATE );
                            }
                            u = (0x10000 + ((u - 0xD800) << 10) + u2 - 0xDC00);
                        }
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR( LEPT_PARSE_INVALID_STRING_ESCAPE );
                }
                break;
            default:
                if ((unsigned char)ch < 0x20) {  /* must change to unsigned char */
                    STRING_ERROR( LEPT_PARSE_INVALID_STRING_CHAR );
                }
                PUTC(c, ch);
                break;
        }
    }
}
#endif

static int lept_parse_array(lept_context* c, lept_value* v) {
    size_t size = 0; /* size of this array */
    int ret;
    lept_value e;
    lept_value* tmp;
    assert(c != NULL && v != NULL);
    EXPECT(c, '[');
    lept_parse_whitespace(c);
    if ( *c->json == ']') {
        c->json ++;
        v->type = LEPT_ARRAY;
        v->u.a.e = NULL;
        v->u.a.size = 0;
        return LEPT_PARSE_OK;
    }
    if (*c->json == ',') {
        return LEPT_PARSE_EXPECT_VALUE;
    }
    for(;;) {
        lept_init(&e);
        lept_parse_whitespace(c);
        tmp = NULL;
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {
            /* roll back */
            while(size--) {
                tmp = lept_context_pop(c, sizeof(lept_value));
                lept_free(tmp);
            }
            return ret;
        }
        size ++;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json ++;
            continue;
        }else if ( *c->json == ']'){
            c->json ++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            v->u.a.e = (lept_value*) malloc(sizeof(lept_value)*size);
            assert(v->u.a.e != NULL);
            memcpy(v->u.a.e,
                   lept_context_pop(c, sizeof(lept_value)*size ),
                   sizeof(lept_value)*size );
            return LEPT_PARSE_OK;
        }else {
            /* roll back */
            while(size--) {
                tmp = lept_context_pop(c, sizeof(lept_value));
                lept_free(tmp);
            }
            return LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
        }
    }
}

static int lept_parse_object (lept_context* c, lept_value* v) {
    size_t size;
    int ret;
    lept_member* tmp;
    assert(c != NULL && v != NULL);
    size = 0;
    lept_init(v);
    /* printf("parse lept_object\n"); */
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = LEPT_OBJECT;
        v->u.o.size = 0;
        v->u.o.m = NULL;
        return LEPT_PARSE_OK;
    }
    for (;;) {
        lept_member m;
        char * str;
        lept_parse_whitespace(c);
        /* parse lept_member */
        /* printf("  parse lept_member -- ");printCur(c); */
        if ((ret = lept_parse_string_raw(c, &str, &(m.klen))) != LEPT_PARSE_OK ) {
            /* roll back */
            while(size--) {
               tmp = lept_context_pop(c, sizeof(lept_member));
               free(tmp->k);
               lept_free(&(tmp->v));
            }
            return ret;
        }
        /* set key */
        m.k = (char*) malloc(m.klen+1);
        memcpy(m.k, str, m.klen);
        m.k[m.klen] = '\0';
        lept_parse_whitespace(c);
        if ((*c->json ++) != ':') {
            /* roll back */
            while(size--) {
               tmp = lept_context_pop(c, sizeof(lept_member));
               free(tmp->k);
               lept_free(&(tmp->v));
            }
            return LEPT_PARSE_MISS_COLON;
        }
        lept_parse_whitespace(c);
        if((ret = lept_parse_value(c, &(m.v))) != LEPT_PARSE_OK) {
            /* roll back */
            while(size--) {
               tmp = lept_context_pop(c, sizeof(lept_member));
               free(tmp->k);
               lept_free(&(tmp->v));
            }
            return ret;
        }
        /* parse lept_member ok */
        /* push lept_member */
        /* printf("  OK - push lept_member\n"); */
        size ++ ;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));

        lept_parse_whitespace(c);
        if ( *c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
            continue;
        }else if ( *c->json == '}') {
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            v->u.o.m = (lept_member*) malloc(sizeof(lept_member) * size);
            memcpy( v->u.o.m,
                    lept_context_pop(c, sizeof(lept_member)*size),
                    sizeof(lept_member)*size );
            return LEPT_PARSE_OK;
        }else {
            /* roll back */
            while(size--) {
                tmp = lept_context_pop(c, sizeof(lept_member));
                free(tmp->k);
                lept_free(&(tmp->v));
            }
            return LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        case '\"': return lept_parse_string(c, v);
        case '[' : return lept_parse_array(c, v);
        case '{' : return lept_parse_object(c, v);
        /* default:   return LEPT_PARSE_INVALID_VALUE; */
        default: return lept_parse_number(c, v);
    }
}

/* parse API */
int lept_parse(lept_value* v, const char* json) {
    int ret = -1;
    lept_context c;
    assert(v != NULL);
    c.json = json;
    c.first = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ( (ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK ) {
        /* to do */
        lept_parse_whitespace(&c);
        if ( *(c.json) != '\0') {
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

static int lept_stringify_value(lept_context* c, const lept_value* v);
/* stringify API */
int lept_stringify(const lept_value* v, char** json, size_t* length) {
    int ret;
    lept_context c;
    assert(v!= NULL && json != NULL);
    c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
    c.top = 0;
    if ((ret = lept_stringify_value(&c, v)) != LEPT_STRINGIFY_OK) {
        if (length) {
            *length = 0;
        }
        *json = NULL;
        free(c.stack);
        return ret;
    }
    if (length) {
        *length = c.top;
    }
    PUTC(&c, '\0');
    *json = c.stack;
    return LEPT_STRINGIFY_OK;
}

static int lept_stringify_string(lept_context* c , const char* str, size_t len) {
    size_t i;
    char buffer[7];
    assert(str != NULL );
    PUTC(c, '\"');
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char) str[i];
        switch (ch) {
            case '\"' :     PUTS(c, "\\\"", 2); break;
            case '\\' :     PUTS(c, "\\\\", 2); break;
            /* case '/' :      PUTS(c, "\\/", 2);  break; */
            case '\b' :     PUTS(c, "\\b", 2);  break;
            case '\f' :     PUTS(c, "\\f", 2);  break;
            case '\n' :     PUTS(c, "\\n", 2);  break;
            case '\r' :     PUTS(c, "\\r", 2);  break;
            case '\t' :     PUTS(c, "\\t", 2);  break;
            default :
                if (ch < 0x20) {
                    sprintf(buffer, "\\u%04X", ch);
                    PUTS(c, buffer, 6);
                }else {
                    PUTC(c, str[i]);
                }
                break;
        }
    }
    PUTC(c, '\"');
    return LEPT_STRINGIFY_OK;
}

static int lept_stringify_value(lept_context* c, const lept_value* v) {
    size_t i, length;
    char* buffer;
    int ret;
    switch(v->type) {
        case LEPT_NULL:     PUTS(c, "null", 4);     break;
        case LEPT_FALSE:    PUTS(c, "false", 5);    break;
        case LEPT_TRUE:     PUTS(c, "true", 4);     break;
        case LEPT_NUMBER:
            buffer = lept_context_push(c, 32);
            length = sprintf(buffer, "%.17g", v->u.n);
            c->top -= (32 - length) ;
            break;
        case LEPT_ARRAY:
            PUTC(c,'[');
            for (i = 0; i< v->u.a.size; i++) {
                if (i > 0) {
                    PUTC(c, ',');
                }
                if( (ret = lept_stringify_value(c, &(v->u.a.e[i]))) != LEPT_STRINGIFY_OK ) {
                    return ret;
                }
            }     
            PUTC(c, ']');
            break;
        case LEPT_OBJECT:
            PUTC(c,'{');
            for (i = 0; i< v->u.o.size; i++) {
                if (i > 0) {
                    PUTC(c, ',');
                }
                if ((ret = lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen)) != LEPT_STRINGIFY_OK ) {
                    return ret;
                }
                PUTC(c, ':');
                if ((ret = lept_stringify_value(c, &(v->u.o.m[i].v))) != LEPT_STRINGIFY_OK) {
                    return ret;
                }
            }     
            PUTC(c, '}');
            break;
        case LEPT_STRING:
            if ((ret = lept_stringify_string(c, v->u.s.s, v->u.s.len)) != LEPT_STRINGIFY_OK) {
                return ret;
            }
            break;
        default: 
            return LEPT_STRINGIFY_INVALID_TYPE;
    }
    return LEPT_STRINGIFY_OK;
}
/* you must free json by yourself */


/* compare API */
int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
    size_t i;
    assert(lhs != NULL && rhs != NULL);
    if (lhs->type != rhs->type) {
        return 0;
    } 
    switch(lhs->type) {
        case LEPT_ARRAY:
            if (lhs->u.a.size != rhs->u.a.size) {
                return 0;
            }
            for (i = 0; i < lhs->u.a.size; i++) {
                if ( !lept_is_equal(&(lhs->u.a.e[i]), &(rhs->u.a.e[i])) ){
                    return 0;
                }
            }
            return 1;
        case LEPT_OBJECT:
            if (lhs->u.o.size != rhs->u.o.size) {
                return 0;
            }
            for (i = 0; i < lhs->u.o.size; i++) {
                if (lhs->u.o.m[i].klen != rhs->u.o.m[i].klen ||
                    memcmp(lhs->u.o.m[i].k, rhs->u.o.m[i].k, lhs->u.o.m[i].klen) != 0 ||
                    !lept_is_equal(&(lhs->u.o.m[i].v), &(rhs->u.o.m[i].v)) ) {
                    return 0;
                }
            }
            return 1;
        case LEPT_STRING:
            return lhs->u.s.len == rhs->u.s.len && memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
        case LEPT_NUMBER:
            return lhs->u.n == rhs->u.n;
        default: 
            return 1;
    }
}
