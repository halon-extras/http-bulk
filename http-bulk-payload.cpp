#include "http-bulk-payload.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>

namespace http_bulk
{
namespace
{
constexpr char UUIDV7_TAG[] = "$UUIDV7";
constexpr char DATA_BASE64_TAG[] = "$DATA_BASE64";

std::string render_batch(payload_format format,
						 const std::vector<std::string>& items,
						 bool wrap_json_array)
{
	std::string payload;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (format == payload_format::jsonarray && wrap_json_array && i != 0)
			payload += ",";
		payload += items[i];
		if (format == payload_format::ndjson)
			payload += "\n";
	}

	if (format == payload_format::jsonarray && wrap_json_array)
		return "[" + payload + "]";
	return payload;
}

std::string json_quote(const std::string& value)
{
	std::string out;
	out.reserve(value.size() + 2);
	out += "\"";
	for (unsigned char ch : value)
	{
		switch (ch)
		{
			case '\\':
				out += "\\\\";
			break;
			case '\"':
				out += "\\\"";
			break;
			case '\b':
				out += "\\b";
			break;
			case '\f':
				out += "\\f";
			break;
			case '\n':
				out += "\\n";
			break;
			case '\r':
				out += "\\r";
			break;
			case '\t':
				out += "\\t";
			break;
			default:
				if (ch < 0x20)
				{
					static constexpr char hex[] = "0123456789abcdef";
					out += "\\u00";
					out += hex[(ch >> 4) & 0x0F];
					out += hex[ch & 0x0F];
				}
				else
					out += static_cast<char>(ch);
			break;
		}
	}
	out += "\"";
	return out;
}

std::string replace_template_tags(const std::string& json_template,
								  const std::string& json_array_payload)
{
	std::string rendered;
	rendered.reserve(json_template.size() + json_array_payload.size());

	size_t pos = 0;
	while (pos < json_template.size())
	{
		size_t uuid_pos = json_template.find(UUIDV7_TAG, pos);
		size_t data_pos = json_template.find(DATA_BASE64_TAG, pos);
		size_t tag_pos = std::min(uuid_pos, data_pos);

		if (tag_pos == std::string::npos)
		{
			rendered.append(json_template, pos, std::string::npos);
			break;
		}

		rendered.append(json_template, pos, tag_pos - pos);
		if (tag_pos == uuid_pos)
		{
			rendered += json_quote(generate_uuid_v7());
			pos = tag_pos + sizeof(UUIDV7_TAG) - 1;
		}
		else
		{
			rendered += json_quote(base64_encode(json_array_payload));
			pos = tag_pos + sizeof(DATA_BASE64_TAG) - 1;
		}
	}

	return rendered;
}
}

std::string render_json_array(const std::vector<std::string>& items)
{
	return render_batch(payload_format::jsonarray, items, true);
}

std::string generate_uuid_v7()
{
    std::array<unsigned char, 16> bytes{};

    const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now());
    const uint64_t timestamp =
        static_cast<uint64_t>(now.time_since_epoch().count());

    // 48-bit big-endian Unix timestamp in milliseconds
    for (size_t i = 0; i < 6; ++i)
        bytes[i] = static_cast<unsigned char>((timestamp >> ((5 - i) * 8)) & 0xFF);

    static thread_local std::mt19937_64 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<unsigned int> dist(0, 255);

    for (size_t i = 6; i < bytes.size(); ++i)
        bytes[i] = static_cast<unsigned char>(dist(rng));

    // Set version (7)
    bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x70);

    // Set variant (10xxxxxx)
    bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

    static constexpr char hex[] = "0123456789abcdef";
    std::string uuid;
    uuid.reserve(36);

    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            uuid.push_back('-');
        uuid.push_back(hex[(bytes[i] >> 4) & 0x0F]);
        uuid.push_back(hex[bytes[i] & 0x0F]);
    }

    return uuid;
}

std::string base64_encode(const std::string& input)
{
	static constexpr char alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string out;
	out.reserve(((input.size() + 2) / 3) * 4);

	uint32_t value = 0;
	int bits = -6;
	for (unsigned char ch : input)
	{
		value = (value << 8) | ch;
		bits += 8;
		while (bits >= 0)
		{
			out += alphabet[(value >> bits) & 0x3Fu];
			bits -= 6;
		}
	}

	if (bits > -6)
		out += alphabet[((value << 8) >> (bits + 8)) & 0x3Fu];
	while (out.size() % 4 != 0)
		out += '=';
	return out;
}

std::string render_payload(payload_format format,
						   const std::vector<std::string>& items,
						   bool wrap_json_array,
						   const std::string& preamble,
						   const std::string& postamble,
						   const std::string& json_template)
{
	std::string payload = preamble;
	if (!json_template.empty())
	{
		// Template substitutions are based on a canonical JSON array payload.
		payload += replace_template_tags(json_template, render_json_array(items));
	}
	else
		payload += render_batch(format, items, wrap_json_array);
	payload += postamble;
	return payload;
}
}
