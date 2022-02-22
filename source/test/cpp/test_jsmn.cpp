#include "xbase/x_target.h"
#include "xbase/x_runes.h"
#include "xjsmn/x_jsmn.h"

#include "xunittest/xunittest.h"

using namespace xcore;

UNITTEST_SUITE_BEGIN(xjsmn)
{
	UNITTEST_FIXTURE(parse)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		UNITTEST_TEST(test)
		{
			jsmntok_t tokens[256];
			jsmn_parser parser;

			// some example JSON to sparse for this unittest
			const char* json = "{\"key1\":\"value1\",\"key2\":\"value2\",\"key3\":\"value3\"}";

			jsmn_init(&parser, tokens, 256);
			int count = jsmn_parse(&parser, json, ascii::strlen(json));
			CHECK_EQUAL(7, count);
		}

		UNITTEST_TEST(test2)
		{
			jsmntok_t tokens[256];
			jsmn_parser parser;

			// JSON with UTF-8 text to parse
			const char* json = "{\"key1\":\"中国語\",\"key2\":\"value2\",\"key3\":\"value3\"}";

			jsmn_init(&parser, tokens, 256);
			int count = jsmn_parse(&parser, json, ascii::strlen(json));
			CHECK_EQUAL(7, count);
		}
	}
}
UNITTEST_SUITE_END