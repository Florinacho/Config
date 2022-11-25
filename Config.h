#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Config API
#define CONFIG_COMMENT_LENGTH 256

#define CFG_COMMENT  1
#define CFG_KV       2
#define CFG_BEGIN    3
#define CFG_END      4

#define CFG_READ   (1 << 0)
#define CFG_WRITE  (1 << 1)

#define CONFIG_TOKEN_COMMENT 1
#define CONFIG_TOKEN_EQUAL 2
#define CONFIG_TOKEN_NODE_BEGIN 3
#define CONFIG_TOKEN_NODE_END 4
#define CONFIG_TOKEN_IDENTIFIER 5
#define CONFIG_TOKEN_EOF 6

bool IsSpace(char c) {
	return ((c == ' ') || (c == '\t') || ('\n'));
}

bool IsAlpha(char c) {
	return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}
bool IsNum(char c) {
	return ((c >= '0') && (c <= '9'));
}

bool IsAlnum(char c) {
	return (IsAlpha(c) || IsNum(c));
}

typedef struct cfg_t {
	char buffer[1024];
	uint32_t offset;
	
	FILE* file;
	
	char tab;
	uint8_t tabCount;
	uint8_t indentation;
} cfg_t;

bool cfg_readLine(cfg_t* context);

int cfg_openEx(cfg_t* context, uint8_t mode, const char* filename, char tab, uint8_t tabCount) {
	if (context == NULL) {
		return 1;
	}
	
	const char* fileMode = "";
	switch (mode) {
	case CFG_READ :
		fileMode = "r";
		break;
	case CFG_WRITE :
		fileMode = "w";
		break;
	}
	
	if ((context->file = fopen(filename, fileMode)) == NULL) {
		return 2;
	}
	
	context->tab = tab;
	context->tabCount = tabCount;
	context->indentation = 0;
	context->offset = -1;
	memset(context->buffer, 0, sizeof(context->buffer));
	
	return 0;
}

int cfg_open(cfg_t* context, uint8_t mode, const char* filename) {
	return cfg_openEx(context, mode, filename, '\t', 1);
}

int cfg_close(cfg_t* context) {
	if (context == NULL) {
		return 1;
	}
	if (context->file == NULL) {
		return 2;
	}
	
	fclose(context->file);
	
	return 0;
}

int ConfigTabulature(cfg_t* context) {
	if ((context == NULL) || (context->file == NULL)) {
		return 1;
	}
	
	const uint32_t charCount = context->tabCount * context->indentation;
	for (uint32_t index = 0; index < charCount; ++index) {
		if (fwrite(&context->tab, sizeof(char), 1, context->file) != 1) {
			return 2;
		}
	}
	
	return 0;
}
#define MIN(a, b) ((a < b) ? a : b)
int cfg_commentl(cfg_t* context, const char* value, uint32_t length) {
	static const char PREFIX[] = "#";
	static const char SUFFIX[] = "\n";
	
	int ans = 0;
	if ((ans = ConfigTabulature(context)) != 0) {
		return ans;
	}

	uint32_t count = 0;
	while (count < length) {
		const uint32_t val = MIN(CONFIG_COMMENT_LENGTH, length - count);
		fwrite(PREFIX, sizeof(char), 1, context->file);
		count += fwrite(value + count, sizeof(char), val, context->file);
		fwrite(SUFFIX, sizeof(char), 1, context->file);
	}

	return ans;
}

int cfg_comment(cfg_t* context, const char* value) {
	return cfg_commentl(context, value, strlen(value));
}

int cfg_kvl(cfg_t* context, const char* key, uint32_t key_length, const char* value, uint32_t value_length) {
	static const char OPERATOR[] = " = ";

	ConfigTabulature(context);
	bool quotes = false;
	for (uint32_t index = 0; index < value_length; ++index) {
		if (IsAlnum(value[index]) == false) {
			quotes = true;
			break;
		}
	}
	
	fwrite(key, sizeof(char), key_length, context->file);
	fwrite(OPERATOR, sizeof(char), strlen(OPERATOR), context->file);
	
	if (quotes) {
		fwrite("\"", sizeof(char), 1, context->file);
	}
	fwrite(value, sizeof(char), value_length, context->file);
	if (quotes) {
		fwrite("\"", sizeof(char), 1, context->file);
	}
	
	fwrite("\n", sizeof(char), 1, context->file);
	
	return 0;
}

int cfg_kv(cfg_t* context, const char* key, const char* value) {
	cfg_kvl(context, key, strlen(key), value, strlen(value));
	return 0;
}

int cfg_beginl(cfg_t* context, const char* identifier, uint32_t length) {
	static const char SUFFIX[] = " {\n";

	ConfigTabulature(context);

	fwrite(identifier, sizeof(char), length, context->file);
	fwrite(SUFFIX, sizeof(char), strlen(SUFFIX), context->file);
	context->indentation += 1;

	return 0;
}

int cfg_begin(cfg_t* context, const char* identifier) {
	return cfg_beginl(context, identifier, strlen(identifier));
}

int cfg_end(cfg_t* context) {
	static const char SUFFIX[] = "}\n";

	context->indentation -= 1;
	ConfigTabulature(context);
	fwrite(SUFFIX, sizeof(char), strlen(SUFFIX), context->file);
	return 0;
}

bool cfg_readLine(cfg_t* context) {
	if ((context == NULL) || (context->file == NULL)) {
		return false;
	}
	
	char* ans = fgets(context->buffer, sizeof(context->buffer), context->file);
	context->offset = 0;
	return (ans != NULL);
}

uint8_t cfg_readToken(cfg_t* context, char** text, uint32_t* length) {
	if (context->offset >= sizeof(context->buffer) - 1) {
		if (cfg_readLine(context) == false) {
			return CONFIG_TOKEN_EOF;
		}
	}
	
	for (; context->offset < sizeof(context->buffer) - 1; ++context->offset) {
		if (text) {
			*text = &context->buffer[context->offset];
		}
		if (length) {
			*length = 1;
		}
		switch (context->buffer[context->offset]) {
		case '#' : 
			{
			*text = &context->buffer[++context->offset];
			uint32_t begin = context->offset; 
			while (context->offset < 1024 && context->buffer[context->offset] != '\0' && context->buffer[context->offset] != '\n') {
				++context->offset;
			}
			*length = context->offset - begin;
			}
			return CONFIG_TOKEN_COMMENT;
		case '=' : ++context->offset; return CONFIG_TOKEN_EQUAL;
		case '{' : ++context->offset; return CONFIG_TOKEN_NODE_BEGIN;
		case '}' : ++context->offset; return CONFIG_TOKEN_NODE_END;
		case '"' :
			*text = &context->buffer[++context->offset];
			const uint32_t begin = context->offset;
			while (context->buffer[context->offset] != '"') {
				++context->offset;
			}
			*length = context->offset - begin;
			//printf("%s::<%.*s>\n", __FUNCTION__, *length, *text);
			return CONFIG_TOKEN_IDENTIFIER;
		default :
			if (IsAlnum(context->buffer[context->offset])) {
				uint32_t begin = context->offset;
				while (IsAlnum(context->buffer[context->offset])) {
					++context->offset;
				}
				if (length) {
					*length = context->offset - begin;
				}
				return CONFIG_TOKEN_IDENTIFIER;
			}
		}
	}
	
	return 0;
}

bool cfg_read(cfg_t* context, uint8_t* type, char** key, uint32_t* key_length, char** value, uint32_t* value_length) {
	uint8_t k = 0;
	*type = 0;
	switch ((k = cfg_readToken(context, key, key_length))) {
	case CONFIG_TOKEN_COMMENT :
		*type = CFG_COMMENT;
		*value_length = 0;
		context->offset = -1;
		return true;
	case CONFIG_TOKEN_IDENTIFIER :
		switch ((k = cfg_readToken(context, NULL, NULL))) {
		case CONFIG_TOKEN_EQUAL :
			if (cfg_readToken(context, value, value_length) == 0) {
				context->offset = -1;
				return false;
			}
			*type = CFG_KV;
			context->offset = -1;
			return true;
		case CONFIG_TOKEN_NODE_BEGIN :
			*type = CFG_BEGIN;
			context->offset = -1;
			return true;
		default :
			printf("Error %d! Unknown token %d\n", __LINE__, k);
			break;
		}
		break;
	case CONFIG_TOKEN_NODE_END :
		context->offset = -1;
		*type = CFG_END;
		return true;
	case CONFIG_TOKEN_EOF :
		return false;
	default :
		printf("Error %d! Unknown token %u\n", __LINE__, k);
		return false;
		break;
	}

	return false;
}

uint32_t cfg_token_count(const char* string, uint32_t length, char separator) {
	uint32_t ans = 1;
	while (length--) {
		ans += (*string == separator);
		++string;
	}
	return ans;
}

const char* cfg_token(const char* string, uint32_t* length, char separator, const char** token, uint32_t* token_length) {
	if (*length == 0) {
		return NULL;
	}

	*token = string;

	for (uint32_t count = 0; count < *length; ++count) {
		if (string[count] == separator) {
			*token_length = count;
			*length -= count + 1;
			return &string[count + 1];
		}
	}
	*token_length = *length;
	*length = 0;

	return &string[*token_length];
}

bool cfg_token_index(const char* string, uint32_t length, char separator, const char** token, uint32_t* token_length, uint32_t index) {
	while ((string = cfg_token(string, &length, separator, token, token_length)) != NULL) {
		if (index-- == 0) {
			return true;
		}
	}
	
	*token = NULL;
	*token_length = 0;
	
	return false;
}

bool cfg_parse_int(int* value, const char* string, uint32_t length) {
	*value = 0;
	bool negative = false;
	
	while (*string == ' ') {
		++string; 
		--length; 
	}
	
	switch (*string) {
	case '-' :
		negative = true;
	case '+' :
		++string;
		--length;
		break;
	}
	
	while (length) {
		switch (*string) {
		case ' ' :
			goto EXIT_LABEL;
		case '0' :
		case '1' :
		case '2' :
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
		case '8' :
		case '9' :
			*value = *value * 10 + *string - '0';
			++string;
			--length;
			break;
		default :
			return false;
		}
	}
	
EXIT_LABEL:
	if (negative) {
		*value *= -1;
	}
	
	return true;
}

#endif // __CONFIG_H__