#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int);
			if (!maxrem) {
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == 'd' || *format == 'i') {
			format++;
			int num = va_arg(parameters, int);
			char buf[12];
			int i = 0;
			int is_negative = 0;
			
			if (num < 0) {
				is_negative = 1;
				num = -num;
			}
			
			if (num == 0) {
				buf[i++] = '0';
			} else {
				while (num > 0) {
					buf[i++] = '0' + (num % 10);
					num /= 10;
				}
			}
			
			if (is_negative) {
				buf[i++] = '-';
			}
			
			for (int j = i - 1; j >= 0; j--) {
				if (!print(&buf[j], 1))
					return -1;
				written++;
			}
		} else if (*format == 'x') {
			format++;
			unsigned int num = va_arg(parameters, unsigned int);
			char buf[9];
			int i = 0;
			const char *hex = "0123456789abcdef";
			
			if (num == 0) {
				buf[i++] = '0';
			} else {
				while (num > 0) {
					buf[i++] = hex[num % 16];
					num /= 16;
				}
			}
			
			for (int j = i - 1; j >= 0; j--) {
				if (!print(&buf[j], 1))
					return -1;
				written++;
			}
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}
