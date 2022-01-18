#include "tools.hpp"

// ========================

// reallocates memory of size newSize for pointer
// if newSize is 0, pointer is freed
void *reallocate(void *pointer, size_t newSize)
{
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, newSize);

    if (result == NULL)
    {
        // exit if we have no more memory available
        fprintf(stderr, "MEMORY ALLOCATION FAILED! (Asked for %zu)\n", newSize);
        exit(1);
    }
    return result;
}

// copies a c string and returns that
char *cpystr(const char *chars, int length)
{
	char *copy = ALLOCATE(char, length + 1);
	memcpy(copy, chars, length);
	copy[length] = '\0';
	return copy;
}

char *_strpstr(const char *str, const char *delim, bool front, bool back)
{
    char *ret = cpystr(str, strlen(str));
    
    if (strlen(ret) == 0 || strlen(delim) == 0 || strlen(delim) > strlen(ret)) return ret;

    // strip front
    while (strstart(ret, delim) && front)
        ret += strlen(delim);
    
    if (strlen(ret) == 0 || strlen(delim) == 0 || strlen(delim) > strlen(ret)) return ret;

    // strip back
    while (strend(ret, delim) && back)
        ret = cpystr(ret, strlen(ret) - strlen(delim));

    return ret;
}
// strip string using delim as delimeter
char *strpstr(const char *str, const char *delim) { return _strpstr(str, delim, true, true); }
// strip front of string using delim as delimeter
char *strpstrf(const char *str, const char *delim) { return _strpstr(str, delim, true, false); }
// strip back of string using delim as delimeter
char *strpstrb(const char *str, const char *delim) { return _strpstr(str, delim, false, true); }

// escapes a string with shit like "\n" (actually "\\n") in it
char *escstr(const char *str)
{
    char *ret = "";

    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] == '\\')
        {
            if (i + 1 < strlen(str)) // char after '\'
            {
                i++; // go to next char
                switch (str[i])
                {
                case 'a': ret = fstr("%s\a", ret); break;
                case 'b': ret = fstr("%s\b", ret); break;
                case 'e': ret = fstr("%s\e", ret); break;
                case 'f': ret = fstr("%s\f", ret); break;
                case 'n': ret = fstr("%s\n", ret); break;
                case 'r': ret = fstr("%s\r", ret); break;
                case 't': ret = fstr("%s\t", ret); break;
                case 'v': ret = fstr("%s\v", ret); break;
                case '\\': ret = fstr("%s\\", ret); break;
                case '\'': ret = fstr("%s\'", ret); break;
                case '\"': ret = fstr("%s\"", ret); break;
                }
            }
        }
        else
            ret = fstr("%s%c", ret, str[i]);
    }

    return ret;
}

// formats a string
char *fstr(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int smallSize = sizeof(char) * 1024;
    char *smallBuffer = (char*)malloc(smallSize);

    int size = vsnprintf(smallBuffer, smallSize, format, args);

    va_end(args);

    if (size < sizeof smallBuffer)
        return smallBuffer;

    int bigSize = sizeof(char) * (size + 1);
    char *buffer = (char*)malloc(bigSize);

    va_start(args, format);
    vsnprintf(buffer, bigSize, format, args);
    va_end(args);

    return buffer;
}

// checks if a string is numeric
bool isnum(const char *str, bool float_allowed)
{
    bool float_found = false;
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] >= '0' && str[i] <= '9') continue;
        
        if (str[i] == '.' && !float_found && float_allowed)
        {
            float_found = true;
            continue;
        }

        return false;
    }
    return true;
}

// check if a string starts with the given substring
bool strstart(const char *str, const char *start)
{
    // str cannot be shorter than start
    if (str == NULL || strlen(str) == 0 || strlen(str) < strlen(start)) return false;

    return strncmp(str, start, strlen(start)) == 0;
}

// check if a string ends with the given substring
bool strend(const char *str, const char *end)
{
    // str cannot be shorter than start
    if (str == NULL || strlen(str) == 0 || strlen(str) < strlen(end)) return false;

    // printf("checking '%s' == '%s' : %s\n",
    //     str + strlen(str) - strlen(end), end, 
    //     strcmp(str + strlen(str) - strlen(end), end) == 0 ? "true" : "false");
    return strcmp(str + strlen(str) - strlen(end), end) == 0;
}

// returns the number of utf8 code points in the buffer at s
size_t utf8len(char *s)
{
    size_t len = 0;
    for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++len;
    return len;
}


// returns the binary representation of the given int as a string
char *bitsf(int value, int len)
{
    char *ret = "";
    while (len--)
        ret = fstr("%s%c", ret, '0' + ((value >> len) & 1));
    return ret;
}


int bitlen(uint32_t value)
{
    int bits = 0;
    for (int bit_test = 16; bit_test > 0; bit_test >>= 1)
    {
        if (value >> bit_test != 0)
        {
            bits += bit_test;
            value >>= bit_test;
        }
    }
    return bits + value;
}

// read contents of file to string
char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char *buffer = (char *)malloc(fileSize + 1);
	if (buffer == NULL)
	{
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize)
	{
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}

// converts the given string to uppercase
char *toUpper(const char *str)
{
    size_t l = strlen(str);
    char *ret = (char*)malloc(l * sizeof (char));
    const char OFFSET = 'a' - 'A';

    for (int ch = 0; ch < l; ch++)
        ret[ch] = (str[ch] >= 'a' && str[ch] <= 'z') ? str[ch] - OFFSET : str[ch];

    return ret;
}