#include "ccore/c_target.h"
#include "cbase/c_runes.h"
#include "cjsmn/c_jsmn.h"

#include "cunittest/cunittest.h"

using namespace ncore;

UNITTEST_SUITE_BEGIN(xjsmn)
{
    UNITTEST_FIXTURE(parse)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test)
        {
            jsmntok_t   tokens[256];
            jsmn_parser parser;

            // some example JSON to sparse for this unittest
            const char* json = "{\"key1\":\"value1\",\"key2\":\"value2\",\"key3\":\"value3\"}";

            jsmn_init(&parser, tokens, 256);
            int count = jsmn_parse(&parser, json, ascii::strlen(json));
            CHECK_EQUAL(7, count);
        }

        UNITTEST_TEST(test2)
        {
            jsmntok_t   tokens[256];
            jsmn_parser parser;

            // JSON with UTF-8 text to parse
            const char* json = "{\"key1\":\"中国語\",\"key2\":\"value2\",\"key3\":\"value3\"}";

            jsmn_init(&parser, tokens, 256);
            int count = jsmn_parse(&parser, json, ascii::strlen(json));
            CHECK_EQUAL(7, count);
        }

        UNITTEST_TEST(test3)
        {
            jsmntok_t   tokens[512];
            jsmn_parser parser;

            // JSON with UTF-8 text to parse
            const char* json = "["\
"[{\"y\":0.25,\"x\":3,\"a\":7},\"\",{\"x\":9},\"\"],"\
"[{\"y\":-0.75,\"x\":2},\"\",{\"x\":1},\"\",{\"x\":7},\"\",{\"x\":1},\"\"],"\
"[{\"y\":-0.875,\"x\":5},\"\",{\"x\":5},\"\"],"\
"[{\"y\":-0.625},\"\",\"\",{\"x\":13},\"\",\"\"],"\
"[{\"y\":-0.75,\"x\":3},\"\",{\"x\":9},\"\"],"\
"[{\"y\":-0.75,\"x\":2},\"\",{\"x\":1},\"\",{\"x\":7},\"\",{\"x\":1},\"\"],"\
"[{\"y\":-0.875,\"x\":5},\"\",{\"x\":5},\"\"],"\
"[{\"y\":-0.625},\"\",\"\",{\"x\":13},\"\",\"\"],"\
"[{\"y\":-0.75,\"x\":3},\"\",{\"x\":9},\"\"],"\
"[{\"y\":-0.75,\"x\":2},\"\",{\"x\":1},\"\",{\"x\":7},\"\",{\"x\":1},\"\"],"\
"[{\"y\":-0.875,\"x\":5},\"\",{\"x\":5},\"\"],"\
"[{\"y\":-0.625},\"\",\"\",{\"x\":13},\"\",\"\"],"\
"[{\"y\":-0.5,\"x\":2.5},\"\",{\"x\":10},\"\"],"\
"[{\"rx\":4,\"ry\":8.175,\"y\":-4.675,\"x\":-0.5},\"\"],"\
"[{\"rx\":13,\"y\":-4.675,\"x\":-0.5},\"\"],"\
"[{\"r\":15,\"rx\":4,\"y\":-4.675,\"x\":-0.5},\"\"],"\
"[{\"r\":30,\"y\":-2,\"x\":-0.5},\"\"],"\
"[{\"x\":-0.5},\"\"],"\
"[{\"r\":45,\"y\":-2,\"x\":-0.5},\"\"],"\
"[{\"x\":-0.5},\"\"],"\
"[{\"r\":-45,\"rx\":13,\"y\":-5.675,\"x\":-0.5},\"\"],"\
"[{\"x\":-0.5},\"\"],"\
"[{\"r\":-30,\"y\":-2,\"x\":-0.5},\"\"],"\
"[{\"x\":-0.5},\"\"],"\
"[{\"r\":-15,\"y\":-1,\"x\":-0.5},\"\"]]";

            jsmn_init(&parser, tokens, 512);
			//jsmn_strict(&parser, false);
            int count = jsmn_parse(&parser, json, ascii::strlen(json));
            CHECK_EQUAL(7, count);
        }
    }
}
UNITTEST_SUITE_END