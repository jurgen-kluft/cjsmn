#ifndef __JSMN_H__
#define __JSMN_H__

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum
{
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT    = 1 << 0,
    JSMN_ARRAY     = 1 << 1,
    JSMN_STRING    = 1 << 2,
    JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

enum jsmnerr
{
    /* Not enough tokens were provided */
    JSMN_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct jsmntok
{
    jsmntype_t type;
    int        start;
    int        end;
    int        size;
    int        parent;
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct jsmn_parser
{
    const char*  begin;
    const char*  cursor;
    const char*  end;

    unsigned int pos;      /* offset in the JSON string */
    int          toksuper; /* superior token node, e.g. parent object or array */

    unsigned int num_tokens;  /* next token to allocate */
    unsigned int max_tokens;
    jsmntok_t*   tokens;
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser* parser, jsmntok_t* tokens, unsigned int max_tokens);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing a single JSON object.
 * UTF-8 encoding is supported.
 */
int jsmn_parse(jsmn_parser* parser, const char* js, const size_t len);


#endif // __JSMN_H__