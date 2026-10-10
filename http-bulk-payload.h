#pragma once

#include <string>
#include <vector>

namespace http_bulk
{
enum class payload_format
{
	ndjson,
	jsonarray,
	custom
};

std::string render_payload(payload_format format,
						   const std::vector<std::string>& items,
						   bool wrap_json_array,
						   const std::string& preamble,
						   const std::string& postamble,
						   const std::string& json_template);

std::string render_json_array(const std::vector<std::string>& items);
std::string generate_uuid_v7();
std::string base64_encode(const std::string& input);
}
