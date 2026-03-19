#include "../http-bulk-payload.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
void expect(bool condition, const std::string& message)
{
	if (!condition)
	{
		std::cerr << message << "\n";
		std::abort();
	}
}

void test_jsonarray_legacy_single_item()
{
	const std::vector<std::string> items { R"({"id":1})" };
	const std::string payload = http_bulk::render_payload(
		http_bulk::payload_format::jsonarray,
		items,
		false,
		"",
		"",
		"");
	expect(payload == R"({"id":1})",
		"jsonarray without wrapping should preserve legacy single-item output");
}

void test_json_template_base64_uses_jsonarray()
{
	const std::vector<std::string> items { R"({"id":1})", R"({"id":2})" };
	const std::string payload = http_bulk::render_payload(
		http_bulk::payload_format::ndjson,
		items,
		false,
		"",
		"",
		R"({"input":{"payloads":[{"data":$DATA_BASE64}]}})");
	const std::string expected = R"({"input":{"payloads":[{"data":")"
		+ http_bulk::base64_encode(R"([{"id":1},{"id":2}])")
		+ R"("}]}})";
	expect(payload == expected,
		"json template should inject base64 of the canonical json array payload");
}

void test_json_template_adds_uuidv7_json_string()
{
	const std::vector<std::string> items { R"({"id":1})" };
	const std::string payload = http_bulk::render_payload(
		http_bulk::payload_format::jsonarray,
		items,
		false,
		"",
		"",
		R"({"requestId":$UUIDV7})");

	expect(payload.size() == 52, "uuidv7 payload length should match the JSON string shape");
	expect(payload.rfind(R"({"requestId":")", 0) == 0,
		"uuidv7 template should inject a JSON string");
	expect(payload.back() == '}', "uuidv7 payload should be valid JSON text");

	const std::string uuid = payload.substr(14, 36);
	expect(uuid[8] == '-' && uuid[13] == '-' && uuid[18] == '-' && uuid[23] == '-',
		"uuidv7 should preserve canonical dashes");
	expect(uuid[14] == '7', "uuidv7 version nibble should be 7");

	const char variant = static_cast<char>(std::tolower(static_cast<unsigned char>(uuid[19])));
	expect(variant >= '8' && variant <= 'b',
		"uuidv7 variant nibble should be RFC 4122 compatible");
}
}

int main()
{
	test_jsonarray_legacy_single_item();
	test_json_template_base64_uses_jsonarray();
	test_json_template_adds_uuidv7_json_string();
	return 0;
}
