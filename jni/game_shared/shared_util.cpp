
#include "common.h"
#include "extdll.h"

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/*
* Globals initialization
*/
#define COM_TOKEN_LEN				1500
char s_shared_token[ COM_TOKEN_LEN ];
char s_shared_quote = '\"';


/* <2d4b0a> ../game_shared/shared_util.cpp:68 */
char *SharedVarArgs(const char *format, ...)
{
	va_list argptr;
	const int BufLen = 1024;
	const int NumBuffers = 4;

	static char string[ NumBuffers ][ BufLen ];
	static int curstring = 0;

	curstring = (curstring + 1) % NumBuffers;

	va_start(argptr, format);
	Q_vsnprintf(string[ curstring ], BufLen, format, argptr);
	va_end(argptr);

	return string[ curstring ];
}

char *BufPrintf(char *buf, int &len, const char *fmt, ...)
{
	if (!buf || len <= 1)
		return NULL;

	va_list argptr;
	va_start(argptr, fmt);

	int written = Q_vsnprintf(buf, len, fmt, argptr);

	va_end(argptr);

	buf[len - 1] = '\0';

	if (written < 0)
	{
		len = 0;
		return NULL;
	}

	int used = Q_strlen(buf);

	if (used >= len)
	{
		len = 0;
		return buf + used;
	}

	len -= used;
	return buf + used;
}


/* <2d4d11> ../game_shared/shared_util.cpp:137 */
const char *NumAsString(int val)
{
	const int BufLen = 16;
	const int NumBuffers = 4;

	static char string[ NumBuffers ][ BufLen ];
	static int curstring = 0;

	int len = 16;

	curstring = (curstring + 1) % 4;
	BufPrintf(string[curstring], len, "%d", val);

	return string[curstring];
}

// Returns the token parsed by SharedParse()

/* <2d4da4> ../game_shared/shared_util.cpp:155 */
char *SharedGetToken()
{
	return s_shared_token;
}

// Returns the token parsed by SharedParse()

/* <2d4dbf> ../game_shared/shared_util.cpp:164 */
NOXREF void SharedSetQuoteChar(char c)
{
	s_shared_quote = c;
}

// Parse a token out of a string

/* <2d4de7> ../game_shared/shared_util.cpp:173 */
const char *SharedParse(const char *data)
{
	int c;
	int len;

	len = 0;
	s_shared_token[0] = '\0';

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			// end of file;
			return NULL;
		}

		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;

		goto skipwhite;
	}

	// handle quoted strings specially
	if (c == s_shared_quote)
	{
		data++;

		while (true)
		{
			c = *data++;
			if (c == s_shared_quote || !c)
			{
				s_shared_token[len] = '\0';
				return data;
			}

			if (len < COM_TOKEN_LEN - 1)
			{
				s_shared_token[len] = c;
				len++;
			}
		}
	}

	// parse single characters
	if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
	{
		s_shared_token[len] = c;
		len++;
		s_shared_token[len] = '\0';
		return data + 1;
	}

	// parse a regular word
	do
	{
		if (len < COM_TOKEN_LEN - 1)
		{
			s_shared_token[len] = c;
			len++;
		}
		data++;
		c = *data;

		if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
			break;

	} while (c > 32);

	s_shared_token[len] = '\0';
	return data;
}

// Returns true if additional data is waiting to be processed on this line

/* <2d4e40> ../game_shared/shared_util.cpp:247 */
NOXREF bool SharedTokenWaiting(const char *buffer)
{
	const char *p;

	p = buffer;
	while (*p && *p!='\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return true;

		p++;
	}

	return false;
}
