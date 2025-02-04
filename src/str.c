#define STR_SHOW_INT_MAX_LEN (64 + (64 / 4) - 1) // 64-bit binary with a delimiter between each nibble.
static str
STR_show_int(char* dst, u16 dst_size, u64 value, enum StrShowIntStyle style, char delimiter)
{
	str result = { dst, 0 };

	switch (style)
	{
		case StrShowIntStyle_unsigned:
		{
			// Determine the characters of the string going right-to-left.
			char tmp_buf[26] = {0};
			u8   tmp_len     = 0;
			u64  remaining   = value;
			do
			{
				tmp_len += 1;

				char character = {0};

				// Insert delimiter?
				if (delimiter && tmp_len % 4 == 0)
				{
					character = delimiter;
				}
				// Insert the next digit.
				else
				{
					character  = '0' + (remaining % 10);
					remaining /= 10;
				}

				tmp_buf[countof(tmp_buf) - tmp_len] = character;
			}
			while (remaining);

			// String fits within destination buffer?
			if (tmp_len <= dst_size)
			{
				result.len = tmp_len;
			}
			// String needs to be truncated.
			else
			{
				result.len = dst_size;
			}

			// Copy string into destination buffer.
			memmove(result.data, tmp_buf + countof(tmp_buf) - tmp_len, result.len);
		} break;

		case StrShowIntStyle_signed:
		{
			// The number can just be interpreted as unsigned?
			if (!(value >> (bitsof(value) - 1)))
			{
				result = STR_show_int(dst, dst_size, value, StrShowIntStyle_unsigned, delimiter);
			}
			// The number is negative, so we take the two's complement and insert a minus sign.
			else if (dst_size)
			{
				result.data[0] = '-';
				result.len     =
					STR_show_int
					(
						dst      + 1,
						dst_size - 1,
						~value   + 1,
						StrShowIntStyle_unsigned,
						delimiter
					).len + 1;
			}
		} break;

		case StrShowIntStyle_binary:
		{
			// Determine amount of bits to write in order to skip any leading zeros.
			u8 bits_remaining = value ? (bitsof(value) - __builtin_clzll(value)) : 1;

			while (result.len < dst_size && bits_remaining)
			{
				char character = {0};

				// Insert delimiter?
				if (delimiter && (result.len + 1) % 4 == 0)
				{
					character = delimiter;
				}
				// Insert bit.
				else
				{
					bits_remaining -= 1;
					character       = '0' + ((value >> bits_remaining) & 1);
				}

				result.data[result.len]  = character;
				result.len              += 1;
			}
		} break;

		case StrShowIntStyle_hex_lower:
		case StrShowIntStyle_hex_upper:
		{
			char casing = (style == StrShowIntStyle_hex_upper) ? 'A' : 'a';

			// Determine amount of nibbles to write in order to skip any leading zeros.
			u8 nibbles_remaining = value ? ((bitsof(value) - __builtin_clzll(value) + 3) / 4) : 1;

			while (result.len < dst_size && nibbles_remaining)
			{
				char character = {0};

				// Insert delimiter?
				if (delimiter && (result.len + 1) % 4 == 0)
				{
					character = delimiter;
				}
				// Insert nibble.
				else
				{
					nibbles_remaining -= 1;
					u8 nibble = (value >> (nibbles_remaining * 4)) & 0b00001111;
					character = (nibble < 10) ? ('0' + nibble) : (casing + nibble - 10);
				}

				result.data[result.len]  = character;
				result.len              += 1;
			}
		} break;
	}

	return result;
}

static void __attribute__((format(printf, 3, 0)))
STR_fmt_builder_va_list(StrFmtBuilderCallback* callback, void* context, char* fmt, va_list args)
{
	if (callback && fmt)
	{
		#define CALLBACK(DATA, LEN) \
			do \
			{ \
				if (callback(context, (DATA), (LEN))) \
				{ \
					goto BREAK; \
				} \
			} \
			while (false)

		char* stream = fmt;
		while (true)
		{
			// Determine how far we can go before the string ends or there's a format specifier.
			i32 raw_len = 0;
			while (stream[raw_len] != '\0' && stream[raw_len] != '%')
			{
				raw_len += 1;
			}

			// There's characters we can submit without any additional processing?
			if (raw_len)
			{
				CALLBACK(stream, raw_len);
				stream += raw_len;
			}

			// Format specifier found?
			if (stream[0] == '%')
			{
				//
				// Eat the introductory (%) character.
				//

				stream += 1;

				//
				// Determine the length-modifier if one exists.
				//

				#include "str.length_modifier.meta"
				/*
					# Make sure to order it in such a way that no faulty short-circuiting in the if-else statements will occur.
					modifiers = ['hh', 'h', 'll']

					# Enumerations of the supported length modifiers.
					Meta.enums('LengthModifier', None, ['none'] + modifiers)
					Meta.line('''
						enum LengthModifier length_modifier = {0};
					''')

					# Determine the length modifier used (if there even is one).
					for condition, modifier in Meta.elifs({
						' && '.join(f"stream[{index}] == '{character}'" for index, character in enumerate(modifier)) : modifier
						for modifier in modifiers
					}):
						Meta.line(f'''
							length_modifier  = LengthModifier_{modifier};
							stream          += {len(modifier)};
						''')
				*/

				//
				// Process the specified conversion.
				//

				char substr_buf[STR_SHOW_INT_MAX_LEN] = {0};
				str  substr                           = {0};

				switch (stream[0])
				{
					// Integer.
					{
						enum StrShowIntStyle style;
						for (;;)
						{
							case 'u' : style = StrShowIntStyle_unsigned;  break;
							case 'i' :
							case 'd' : style = StrShowIntStyle_signed;    break;
							case 'b' :
							case 'B' : style = StrShowIntStyle_binary;    break;
							case 'x' : style = StrShowIntStyle_hex_lower; break;
							case 'X' : style = StrShowIntStyle_hex_upper; break;
						}

						u64 value = {0};
						switch (length_modifier)
						{
							case LengthModifier_none : value = va_arg(args, unsigned          );          break;
							case LengthModifier_hh   : value = va_arg(args, unsigned          ) & 0x00FF; break;
							case LengthModifier_h    : value = va_arg(args, unsigned          ) & 0xFFFF; break;
							case LengthModifier_ll   : value = va_arg(args, unsigned long long);          break;
						}

						substr = STR_show_int(substr_buf + 1, countof(substr_buf) - 1, value, style, 0);
					} break;

					// String.
					case 's':
					{
						substr.data = va_arg(args, char*);

						// Null string given?
						if (!substr.data)
						{
							substr = str("(null)");
						}
						// Plain c-string.
						else
						{
							substr.len = strlen(substr.data);
						}
					} break;

					// Single character.
					case 'c':
					{
						substr_buf[0] = va_arg(args, int);
						substr.data   = substr_buf;
						substr.len    = 1;
					} break;

					// Escaped (%) character.
					case '%':
					{
						substr = str("%");
					} break;

					// Unknown format specifier.
					default:
					{
						// Insert a grawlix and continue on processing the rest of the format string.
						substr = str("#%!&");
					} break;
				}

				//
				// Eat conversion-specifier character.
				//

				stream += 1;

				//
				// Submit the formatted data.
				//

				CALLBACK(substr.data, substr.len);
			}

			// Format string entirely processed?
			if (stream[0] == '\0')
			{
				break;
			}
		}
		BREAK:;
	}
}

static void __attribute__((format(printf, 3, 4)))
STR_fmt_builder(StrFmtBuilderCallback* callback, void* context, char* fmt, ...)
{
	va_list args = {0};
	va_start(args, fmt);
	STR_fmt_builder_va_list(callback, context, fmt, args);
	va_end(args);
}
