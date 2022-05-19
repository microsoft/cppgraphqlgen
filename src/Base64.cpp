// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/internal/Base64.h"

#include <array>
#include <stdexcept>

namespace graphql::internal {

std::uint8_t Base64::verifyFromBase64(char ch)
{
	const std::uint8_t result = fromBase64(ch);

	if ((result & 0xC0) != 0)
	{
		throw std::logic_error { "invalid character in base64 encoded string" };
	}

	return result;
}

std::vector<std::uint8_t> Base64::fromBase64(std::string_view encoded)
{
	std::vector<std::uint8_t> result;

	if (encoded.empty())
	{
		return result;
	}

	result.reserve((encoded.size() + (encoded.size() % 4)) * 3 / 4);

	// First decode all of the full unpadded segments 24 bits at a time
	while (encoded.size() >= 4 && encoded[3] != padding)
	{
		const uint32_t segment = (static_cast<uint32_t>(verifyFromBase64(encoded[0])) << 18)
			| (static_cast<uint32_t>(verifyFromBase64(encoded[1])) << 12)
			| (static_cast<uint32_t>(verifyFromBase64(encoded[2])) << 6)
			| static_cast<uint32_t>(verifyFromBase64(encoded[3]));

		result.emplace_back(static_cast<std::uint8_t>((segment & 0xFF0000) >> 16));
		result.emplace_back(static_cast<std::uint8_t>((segment & 0xFF00) >> 8));
		result.emplace_back(static_cast<std::uint8_t>(segment & 0xFF));

		encoded = encoded.substr(4);
	}

	// Get any leftover partial segment with 2 or 3 non-padding characters
	if (encoded.size() > 1)
	{
		const bool triplet = (encoded.size() > 2 && padding != encoded[2]);
		const std::uint8_t tail = (triplet ? verifyFromBase64(encoded[2]) : 0);
		const uint16_t segment = (static_cast<uint16_t>(verifyFromBase64(encoded[0])) << 10)
			| (static_cast<uint16_t>(verifyFromBase64(encoded[1])) << 4)
			| (static_cast<uint16_t>(tail) >> 2);

		if (triplet)
		{
			if (tail & 0x3)
			{
				throw std::logic_error { "invalid padding at the end of a base64 encoded string" };
			}

			result.emplace_back(static_cast<std::uint8_t>((segment & 0xFF00) >> 8));
			result.emplace_back(static_cast<std::uint8_t>(segment & 0xFF));

			encoded = encoded.substr(3);
		}
		else
		{
			if (segment & 0xFF)
			{
				throw std::logic_error { "invalid padding at the end of a base64 encoded string" };
			}

			result.emplace_back(static_cast<std::uint8_t>((segment & 0xFF00) >> 8));

			encoded = encoded.substr(2);
		}
	}

	// Make sure anything that's left is 0 - 2 characters of padding
	if ((encoded.size() > 0 && padding != encoded[0])
		|| (encoded.size() > 1 && padding != encoded[1]) || encoded.size() > 2)
	{
		throw std::logic_error { "invalid padding at the end of a base64 encoded string" };
	}

	return result;
}

char Base64::verifyToBase64(std::uint8_t i)
{
	unsigned char result = toBase64(i);

	if (result == padding)
	{
		throw std::logic_error { "invalid 6-bit value" };
	}

	return result;
}

std::string Base64::toBase64(const std::vector<std::uint8_t>& bytes)
{
	std::string result;

	if (bytes.empty())
	{
		return result;
	}

	size_t count = bytes.size();
	const std::uint8_t* data = bytes.data();

	result.reserve((count + (count % 3)) * 4 / 3);

	// First encode all of the full unpadded segments 24 bits at a time
	while (count >= 3)
	{
		const uint32_t segment = (static_cast<uint32_t>(data[0]) << 16)
			| (static_cast<uint32_t>(data[1]) << 8) | static_cast<uint32_t>(data[2]);

		result.append({ verifyToBase64(static_cast<std::uint8_t>((segment & 0xFC0000) >> 18)),
			verifyToBase64(static_cast<std::uint8_t>((segment & 0x3F000) >> 12)),
			verifyToBase64(static_cast<std::uint8_t>((segment & 0xFC0) >> 6)),
			verifyToBase64(static_cast<std::uint8_t>(segment & 0x3F)) });

		data += 3;
		count -= 3;
	}

	// Get any leftover partial segment with 1 or 2 bytes
	if (count > 0)
	{
		const bool pair = (count > 1);
		const uint16_t segment =
			(static_cast<uint16_t>(data[0]) << 8) | (pair ? static_cast<uint16_t>(data[1]) : 0);
		const std::array<char, 4> remainder { verifyToBase64(static_cast<std::uint8_t>(
												  (segment & 0xFC00) >> 10)),
			verifyToBase64(static_cast<std::uint8_t>((segment & 0x3F0) >> 4)),
			(pair ? verifyToBase64(static_cast<std::uint8_t>((segment & 0xF) << 2)) : padding),
			padding };

		result.append(remainder.data(), remainder.size());
	}

	return result;
}

Base64::Comparison Base64::compareBase64(
	const std::vector<std::uint8_t>& bytes, std::string_view maybeEncoded) noexcept
{
	if (bytes.empty())
	{
		if (maybeEncoded.empty())
		{
			return Comparison::EqualTo;
		}
	}
	else if (maybeEncoded.empty())
	{
		return Comparison::LessThan;
	}

	auto result = Comparison::EqualTo;
	auto itr = bytes.cbegin();
	const auto itrEnd = bytes.cend();

	// First decode and compare all of the full unpadded segments 24 bits at a time
	while (maybeEncoded.size() >= 4 && maybeEncoded[3] != padding)
	{
		const auto a = fromBase64(maybeEncoded[0]);
		const auto b = fromBase64(maybeEncoded[1]);
		const auto c = fromBase64(maybeEncoded[2]);
		const auto d = fromBase64(maybeEncoded[3]);

		if (((a | b | c | d) & 0xC0) != 0)
		{
			// Invalid Base64 characters
			return Comparison::InvalidBase64;
		}

		if (Comparison::EqualTo == result)
		{
			const uint32_t segment = (static_cast<uint32_t>(a) << 18)
				| (static_cast<uint32_t>(b) << 12) | (static_cast<uint32_t>(c) << 6)
				| static_cast<uint32_t>(d);
			const std::array decoded { static_cast<std::uint8_t>((segment & 0xFF0000) >> 16),
				static_cast<std::uint8_t>((segment & 0xFF00) >> 8),
				static_cast<std::uint8_t>(segment & 0xFF) };

			for (auto value : decoded)
			{
				if (itr == itrEnd)
				{
					result = Comparison::LessThan;
					break;
				}

				if (*itr != value)
				{
					result = *itr < value ? Comparison::LessThan : Comparison::GreaterThan;
					break;
				}

				++itr;
			}
		}

		maybeEncoded = maybeEncoded.substr(4);
	}

	// Compare any leftover partial segment with 2 or 3 non-padding characters
	if (maybeEncoded.size() > 1)
	{
		const bool triplet = (maybeEncoded.size() > 2 && padding != maybeEncoded[2]);
		const auto a = fromBase64(maybeEncoded[0]);
		const auto b = fromBase64(maybeEncoded[1]);
		const auto c = triplet ? fromBase64(maybeEncoded[2]) : std::uint8_t {};

		if (((a | b | c) & 0xC0) != 0 || (c & 0x3) != 0)
		{
			// Invalid Base64 characters or padding
			return Comparison::InvalidBase64;
		}

		const uint16_t segment = (static_cast<uint16_t>(a) << 10) | (static_cast<uint16_t>(b) << 4)
			| (static_cast<uint16_t>(c) >> 2);
		const std::array decoded { static_cast<std::uint8_t>((segment & 0xFF00) >> 8),
			static_cast<std::uint8_t>(segment & 0xFF) };

		if (triplet)
		{
			if (Comparison::EqualTo == result)
			{
				for (auto value : decoded)
				{
					if (itr == itrEnd)
					{
						result = Comparison::LessThan;
						break;
					}

					if (*itr != value)
					{
						result = *itr < value ? Comparison::LessThan : Comparison::GreaterThan;
						break;
					}

					++itr;
				}
			}

			maybeEncoded = maybeEncoded.substr(3);
		}
		else
		{
			if (decoded[1] != 0)
			{
				// Invalid padding
				return Comparison::InvalidBase64;
			}

			if (Comparison::EqualTo == result)
			{
				if (itr == itrEnd)
				{
					result = Comparison::LessThan;
				}
				else if (*itr != decoded[0])
				{
					result = *itr < decoded[0] ? Comparison::LessThan : Comparison::GreaterThan;
				}

				++itr;
			}

			maybeEncoded = maybeEncoded.substr(2);
		}
	}

	// Make sure anything that's left is 0 - 2 characters of padding
	if ((maybeEncoded.size() > 0 && padding != maybeEncoded[0])
		|| (maybeEncoded.size() > 1 && padding != maybeEncoded[1]) || maybeEncoded.size() > 2)
	{
		return Comparison::InvalidBase64;
	}

	if (Comparison::EqualTo == result)
	{
		// We should reach the end of the byte vector
		if (itr != itrEnd)
		{
			result = Comparison::GreaterThan;
		}
	}

	return result;
}

bool Base64::validateBase64(std::string_view maybeEncoded) noexcept
{
	if (maybeEncoded.empty())
	{
		return true;
	}

	// First decode and compare all of the full unpadded segments 24 bits at a time
	while (maybeEncoded.size() >= 4 && maybeEncoded[3] != padding)
	{
		const auto a = fromBase64(maybeEncoded[0]);
		const auto b = fromBase64(maybeEncoded[1]);
		const auto c = fromBase64(maybeEncoded[2]);
		const auto d = fromBase64(maybeEncoded[3]);

		if (((a | b | c | d) & 0xC0) != 0)
		{
			// Invalid Base64 characters
			return false;
		}

		maybeEncoded = maybeEncoded.substr(4);
	}

	// Compare any leftover partial segment with 2 or 3 non-padding characters
	if (maybeEncoded.size() > 1)
	{
		const bool triplet = (maybeEncoded.size() > 2 && padding != maybeEncoded[2]);
		const auto a = fromBase64(maybeEncoded[0]);
		const auto b = fromBase64(maybeEncoded[1]);
		const auto c = triplet ? fromBase64(maybeEncoded[2]) : std::uint8_t {};

		if (((a | b | c) & 0xC0) != 0 || (c & 0x3) != 0)
		{
			// Invalid Base64 characters or padding
			return false;
		}

		if (triplet)
		{
			maybeEncoded = maybeEncoded.substr(3);
		}
		else
		{
			const uint16_t segment = (static_cast<uint16_t>(a) << 10)
				| (static_cast<uint16_t>(b) << 4) | (static_cast<uint16_t>(c) >> 2);

			if ((segment & 0xFF) != 0)
			{
				// Invalid padding
				return false;
			}

			maybeEncoded = maybeEncoded.substr(2);
		}
	}

	// Make sure anything that's left is 0 - 2 characters of padding
	if ((maybeEncoded.size() > 0 && padding != maybeEncoded[0])
		|| (maybeEncoded.size() > 1 && padding != maybeEncoded[1]) || maybeEncoded.size() > 2)
	{
		return false;
	}

	return true;
}

} // namespace graphql::internal
