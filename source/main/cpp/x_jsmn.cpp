#include "xjsmn/x_jsmn.h"
#include <stddef.h>

#define JSMN_API static

// UTF-8; read a character and return the unicode codepoint (UTF-32)
static unsigned int jsmn_read(const char*& str, const char* end)
{
    unsigned int c = (unsigned char)*str;
    if (c != 0)
    {
        ++str;
        if (c < 0x80) {}
        else if ((c >> 5) == 0x6)
        {
            c = ((c << 6) & 0x7ff) + ((str[1]) & 0x3f);
            ++str;
        }
        else if ((c >> 4) == 0xe)
        {
            c = ((c << 12) & 0xffff) + (((str[1]) << 6) & 0xfff);
            c += (str[2]) & 0x3f;
            str += 2;
        }
        else if ((c >> 3) == 0x1e)
        {
            c = ((c << 18) & 0x1fffff) + (((str[1]) << 12) & 0x3ffff);
            c += ((str[2]) << 6) & 0xfff;
            c += (str[3]) & 0x3f;
            str += 3;
        }
        else
        {
            c = 0xFFFE; // illegal character
        }
    }
    return c;
}

static inline int jsmn_to_pos(jsmn_parser* parser, const char* cursor) { return (int)(cursor - parser->begin); }

/**
 * Constructs a new token and initializes it with type and boundaries.
 */
static inline void jsmn_push_token(jsmn_parser* parser, const jsmntype_t type, const int start, const int end, const int parent)
{
    jsmntok_t* token = &parser->tokens[parser->num_tokens++];
    token->type      = type;
    token->start     = start;
    token->end       = end;
    token->size      = 0;
    token->parent    = parent;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_parser* parser, const char* js, const size_t len)
{
    int start;

    start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        switch (js[parser->pos])
        {
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ',':
            case ']':
            case '}': goto found;
            default:
                if (!parser->strict)
                {
                    if (js[parser->pos] == ':')
                    {
                        goto found;
                    }
                }
                /* to quiet a warning from gcc*/
                break;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127)
        {
            parser->pos = start;
            return JSMN_ERROR_INVAL;
        }
    }
    if (parser->strict)
    {
        /* In strict mode primitive must be followed by a comma/object/array */
        parser->pos = start;
        return JSMN_ERROR_PART;
    }

found:
    if (parser->num_tokens >= parser->max_tokens)
    {
        parser->pos = start;
        return JSMN_ERROR_NOMEM;
    }
    jsmn_push_token(parser, JSMN_PRIMITIVE, start, parser->pos, parser->toksuper);
    parser->pos--;
    return 0;
}

static int jsmn_parse_primitive_utf8(jsmn_parser* parser)
{
    const char* next  = parser->cursor;
    const char* start = parser->cursor;
    for (; parser->cursor < parser->end;)
    {
        next           = parser->cursor;
        unsigned int c = jsmn_read(next, parser->end);
        if (c == 0)
        {
            break;
        }
        if (c == 0xFFFE)
        {
            parser->cursor = start;
            return JSMN_ERROR_INVAL;
        }

        switch (c)
        {
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ',':
            case ']':
            case '}': goto found;
            default:
                if (!parser->strict && c == ':')
                {
                    goto found;
                }
                // to quiet a warning from gcc
                break;
        }
        if (c < 32)
        {
            parser->cursor = start;
            return JSMN_ERROR_INVAL;
        }
        parser->cursor = next;
    }
    if (parser->strict)
    {
        // In strict mode primitive must be followed by a comma/object/array
        parser->cursor = start;
        return JSMN_ERROR_PART;
    }

found:
    if (parser->num_tokens >= parser->max_tokens)
    {
        parser->cursor = start;
        return JSMN_ERROR_NOMEM;
    }
    jsmn_push_token(parser, JSMN_PRIMITIVE, jsmn_to_pos(parser, start), jsmn_to_pos(parser, parser->cursor), parser->toksuper);
    return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_parser* parser, const char* js, const size_t len)
{
    int start = parser->pos;

    /* Skip starting quote */
    parser->pos++;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"')
        {
            if (parser->num_tokens >= parser->max_tokens)
            {
                parser->pos = start;
                return JSMN_ERROR_NOMEM;
            }
            jsmn_push_token(parser, JSMN_STRING, start + 1, parser->pos, parser->toksuper);
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->pos + 1 < len)
        {
            int i;
            parser->pos++;
            switch (js[parser->pos])
            {
                /* Allowed escaped symbols */
                case '\"':
                case '/':
                case '\\':
                case 'b':
                case 'f':
                case 'r':
                case 'n':
                case 't': break;
                /* Allows escaped symbol \uXXXX */
                case 'u':
                    parser->pos++;
                    for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++)
                    {
                        /* If it isn't a hex character we have an error */
                        if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                              (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                              (js[parser->pos] >= 97 && js[parser->pos] <= 102)))
                        { /* a-f */
                            parser->pos = start;
                            return JSMN_ERROR_INVAL;
                        }
                        parser->pos++;
                    }
                    parser->pos--;
                    break;
                /* Unexpected symbol */
                default: parser->pos = start; return JSMN_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSMN_ERROR_PART;
}

static int jsmn_parse_string_utf8(jsmn_parser* parser)
{
    const char* start       = parser->cursor;
    const char* token_start = parser->cursor;

    // quote is already consumed

    for (; parser->cursor < parser->end;)
    {
        const char*  next = parser->cursor;
        unsigned int c    = jsmn_read(next, parser->end);
        if (c == 0)
            break;
        if (c == 0xFFFE)
        {
            parser->cursor = start;
            return JSMN_ERROR_INVAL;
        }

        /* Quote: end of string */
        if (c == '\"')
        {
            if (parser->num_tokens >= parser->max_tokens)
            {
                parser->cursor = start;
                return JSMN_ERROR_NOMEM;
            }
            jsmn_push_token(parser, JSMN_STRING, jsmn_to_pos(parser, token_start), jsmn_to_pos(parser, parser->cursor), parser->toksuper);
            parser->cursor = next;
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && next < parser->end)
        {
            int i;
            parser->cursor = next;
            c              = jsmn_read(next, parser->end);
            switch (c)
            {
                /* Allowed escaped symbols */
                case '\"':
                case '/':
                case '\\':
                case 'b':
                case 'f':
                case 'r':
                case 'n':
                case 't': parser->cursor = next; break;
                /* Allows escaped symbol \uXXXX */
                case 'u':
                    for (i = 0; i < 4 && next < parser->end; i++)
                    {
                        const char* read = next;
                        c                = jsmn_read(read, parser->end);
                        if (c == 0)
                            break;

                        /* If it isn't a hex character we have an error */
                        if (!((c >= 48 && c <= 57) || /* 0-9 */
                              (c >= 65 && c <= 70) || /* A-F */
                              (c >= 97 && c <= 102)))
                        { /* a-f */
                            parser->cursor = start;
                            return JSMN_ERROR_INVAL;
                        }
                        parser->cursor = next;
                        next           = read;
                    }
                    break;
                /* Unexpected symbol */
                default: parser->cursor = start; return JSMN_ERROR_INVAL;
            }
        }

        parser->cursor = next;
    }
    parser->cursor = start;
    return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
JSMN_API int jsmn_parse_ascii(jsmn_parser* parser, const char* js, const size_t len)
{
    int        r;
    int        i;
    jsmntok_t* token;
    int        count = parser->num_tokens;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
        char       c;
        int        parent = -1;
        jsmntype_t type;

        c = js[parser->pos];
        switch (c)
        {
            case '{':
            case '[':
                count++;
                if (parser->num_tokens >= parser->max_tokens)
                {
                    return JSMN_ERROR_NOMEM;
                }
                if (parser->toksuper != -1)
                {
                    jsmntok_t* t = &parser->tokens[parser->toksuper];
                    if (parser->strict)
                    {
                        /* In strict mode an object or array can't become a key */
                        if (t->type == JSMN_OBJECT)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                    }
                    t->size++;
                    parent = parser->toksuper;
                }
                jsmn_push_token(parser, (c == '{' ? JSMN_OBJECT : JSMN_ARRAY), parser->pos, -1, parent);
                parser->toksuper = parser->num_tokens - 1;
                break;
            case '}':
            case ']':
                type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
                if (parser->num_tokens < 1)
                {
                    return JSMN_ERROR_INVAL;
                }
                token = &parser->tokens[parser->num_tokens - 1];
                for (;;)
                {
                    if (token->start != -1 && token->end == -1)
                    {
                        if (token->type != type)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        token->end       = parser->pos + 1;
                        parser->toksuper = token->parent;
                        break;
                    }
                    if (token->parent == -1)
                    {
                        if (token->type != type || parser->toksuper == -1)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        break;
                    }
                    token = &parser->tokens[token->parent];
                }
                break;
            case '\"':
                r = jsmn_parse_string(parser, js, len);
                if (r < 0)
                {
                    return r;
                }
                count++;
                if (parser->toksuper != -1)
                {
                    parser->tokens[parser->toksuper].size++;
                }
                break;
            case '\t':
            case '\r':
            case '\n':
            case ' ': break;
            case ':': parser->toksuper = parser->num_tokens - 1; break;
            case ',':
                if (parser->toksuper != -1 && parser->tokens[parser->toksuper].type != JSMN_ARRAY && parser->tokens[parser->toksuper].type != JSMN_OBJECT)
                {
                    parser->toksuper = parser->tokens[parser->toksuper].parent;
                }
                break;

            /* In non-strict mode every unquoted value is a primitive */
            default:
                if (parser->strict)
                {
                    /* In strict mode primitives are: numbers and booleans */
                    switch (c)
                    {
                        case '-':
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case 't':
                        case 'f':
                        case 'n':
                            /* And they must not be keys of the object */
                            if (parser->tokens != NULL && parser->toksuper != -1)
                            {
                                const jsmntok_t* t = &parser->tokens[parser->toksuper];
                                if (t->type == JSMN_OBJECT || (t->type == JSMN_STRING && t->size != 0))
                                {
                                    return JSMN_ERROR_INVAL;
                                }
                            }
                            break;
                        /* Unexpected char in strict mode */
                        default: return JSMN_ERROR_INVAL;
                    }
                }

                r = jsmn_parse_primitive(parser, js, len);
                if (r < 0)
                {
                    return r;
                }
                count++;
                if (parser->toksuper != -1)
                {
                    parser->tokens[parser->toksuper].size++;
                }
                break;
        }
    }

    for (i = parser->num_tokens - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (parser->tokens[i].start != -1 && parser->tokens[i].end == -1)
        {
            return JSMN_ERROR_PART;
        }
    }

    return count;
}

/**
 * Parse JSON string and fill tokens.
 */
JSMN_API int jsmn_parse_utf8(jsmn_parser* parser, const char* js, const size_t len)
{
    int        r;
    int        i;
    jsmntok_t* token;
    int        count = parser->num_tokens;

    parser->begin  = js;
    parser->end    = js + len;
    parser->cursor = js;

    while (parser->cursor < parser->end)
    {
        int          parent;
        jsmntype_t   type;
        const char*  next = parser->cursor;
        unsigned int c    = jsmn_read(next, parser->end);
        switch (c)
        {
            case '\0': goto read_eof;
            case '{':
            case '[':
                count++;
                if (parser->num_tokens >= parser->max_tokens)
                {
                    return JSMN_ERROR_NOMEM;
                }
                parent = -1;
                if (parser->toksuper != -1)
                {
                    jsmntok_t* t = &parser->tokens[parser->toksuper];
                    if (parser->strict)
                    {
                        // In strict mode an object or array can't become a key
                        if (t->type == JSMN_OBJECT)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                    }
                    t->size++;
                    parent = parser->toksuper;
                }
                jsmn_push_token(parser, (c == '{' ? JSMN_OBJECT : JSMN_ARRAY), jsmn_to_pos(parser, parser->cursor), -1, parent);
                parser->toksuper = parser->num_tokens - 1;
                parser->cursor   = next;
                break;
            case '}':
            case ']':
                type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
                if (parser->num_tokens < 1)
                {
                    return JSMN_ERROR_INVAL;
                }
                token = &parser->tokens[parser->num_tokens - 1];
                for (;;)
                {
                    if (token->start != -1 && token->end == -1)
                    {
                        if (token->type != type)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        token->end       = (unsigned int)(next - parser->begin);
                        parser->toksuper = token->parent;
                        break;
                    }
                    if (token->parent == -1)
                    {
                        if (token->type != type || parser->toksuper == -1)
                        {
                            return JSMN_ERROR_INVAL;
                        }
                        break;
                    }
                    token = &parser->tokens[token->parent];
                }
                parser->cursor = next;
                break;
            case '\"':
                parser->cursor = next; // skip the quote
                r              = jsmn_parse_string_utf8(parser);
                if (r < 0)
                {
                    return r;
                }
                count++;
                if (parser->toksuper != -1)
                {
                    parser->tokens[parser->toksuper].size++;
                }
                break;
            case '\t':
            case '\r':
            case '\n':
            case ' ': parser->cursor = next; break;
            case ':':
                parser->toksuper = parser->num_tokens - 1;
                parser->cursor   = next;
                break;
            case ',':
                if (parser->toksuper != -1 && parser->tokens[parser->toksuper].type != JSMN_ARRAY && parser->tokens[parser->toksuper].type != JSMN_OBJECT)
                {
                    parser->toksuper = parser->tokens[parser->toksuper].parent;
                }
                parser->cursor = next;
                break;

            default:
                // In non-strict mode every unquoted value is a primitive
                if (parser->strict)
                {
                    switch (c)
                    {
                        case '-':
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case 't':
                        case 'f':
                        case 'n':
                            /* And they must not be keys of the object */
                            if (parser->tokens != NULL && parser->toksuper != -1)
                            {
                                const jsmntok_t* t = &parser->tokens[parser->toksuper];
                                if (t->type == JSMN_OBJECT || (t->type == JSMN_STRING && t->size != 0))
                                {
                                    return JSMN_ERROR_INVAL;
                                }
                            }
                            break;
                        default:
                            // Unexpected char in strict mode
                            return JSMN_ERROR_INVAL;
                    }
                }

                r = jsmn_parse_primitive_utf8(parser);
                if (r < 0)
                {
                    return r;
                }
                count++;
                if (parser->toksuper != -1)
                {
                    parser->tokens[parser->toksuper].size++;
                }

                break;
        }
    }

read_eof:
    for (i = parser->num_tokens - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (parser->tokens[i].start != -1 && parser->tokens[i].end == -1)
        {
            return JSMN_ERROR_PART;
        }
    }

    return count;
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
void jsmn_init(jsmn_parser* parser, jsmntok_t* tokens, const unsigned int max_tokens)
{
    parser->strict     = true;
    parser->num_tokens = 0;
    parser->toksuper   = -1;
    parser->tokens     = tokens;
    parser->max_tokens = max_tokens;
    parser->pos        = 0;
    parser->begin      = 0;
    parser->cursor     = 0;
    parser->end        = 0;
}

void jsmn_strict(jsmn_parser* parser, bool strict) { parser->strict = strict; }

/**
 * Parse json string and fill tokens (UTF-8 encoding)
 */
int jsmn_parse(jsmn_parser* parser, const char* js, const size_t len) { return jsmn_parse_utf8(parser, js, len); }
