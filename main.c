#include "Config.h"

#include <stdio.h>

void TestConfigWrite(const char* filename) {
	cfg_t config;
	
	cfg_open(&config, CFG_WRITE, filename);
	
	// Single line comment
	cfg_comment(&config, "This is a test config file");
	
	// Multi-line comment
	cfg_comment(&config, 
	            "123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
	            "123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
				"123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
				"123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
				"123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 "
				"123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 ");
				  
	// Indented comment
	char comment[] = "Test line X";
	for (config.indentation = 0; config.indentation < 9; ++config.indentation) {
		comment[10] = '0' + config.indentation;
		cfg_comment(&config, comment);
	}
	config.indentation = 0;
	
	// Simple KV
	cfg_kv(&config, "identifier", "value");
	
	// Node
	cfg_comment(&config, "Test node");
	cfg_begin(&config, "node0");
		cfg_comment(&config, "Test node child");
		cfg_kv(&config, "child", "value");
		cfg_comment(&config, "Test encapsulated");
		cfg_begin(&config, "node1");
			cfg_kv(&config, "child", "value");
			cfg_kv(&config, "position", "0, 5");
			cfg_kv(&config, "array", "0,1,,2, 3, 5,12");
		cfg_end(&config);
	cfg_end(&config);
	
	cfg_close(&config);
}

void TestConfigRead(const char* filename) {
	cfg_t inConfig;
	cfg_t outConfig;
	
	uint8_t type = 0;
	char* key = NULL;
	uint32_t key_length = 0;
	char* value = NULL;
	uint32_t value_length = 0;
	
	cfg_open(&inConfig, CFG_READ, filename);
	cfg_open(&outConfig, CFG_WRITE, "out.cfg");

	while (cfg_read(&inConfig, &type, &key, &key_length, &value, &value_length)) {
		switch (type) {
		case CFG_COMMENT :
			cfg_commentl(&outConfig, key, key_length);
			break;
		case CFG_KV :
			cfg_kvl(&outConfig, key, key_length, value, value_length);
			
			static const char* KEY = "array";
			uint32_t KEY_LEN = strlen(KEY);
			if (KEY_LEN == key_length && memcmp(KEY, key, key_length) == 0) {
				const char* token = NULL;
				uint32_t token_length = 0;
				uint32_t token_count = cfg_token_count(value, value_length, ',');
				
				printf("Found array with %d values: %.*s = ", token_count, KEY_LEN, KEY);
				for (uint32_t index = 0; index < token_count; ++index) {
					if (cfg_token_index(value, value_length, ',', &token, &token_length, index)) {
						printf("[%.*s] ", token_length, token);
					} else {
						printf("error, ");
					}
				}
				printf("\n");
			}
			break;
		case CFG_BEGIN :
			cfg_beginl(&outConfig, key, key_length);
			break;
		case CFG_END :
			cfg_end(&outConfig);
			break;
		}
	}

	cfg_close(&inConfig);
	cfg_close(&outConfig);
}

int main() {
	static const char* CONFIG_FILE_PATH = "./test.cfg";
	
	TestConfigWrite(CONFIG_FILE_PATH);
	TestConfigRead(CONFIG_FILE_PATH);
	
	{
		// Token tests
		const char* TABLE = ",a,b,c,d,,,,este,";
		const uint32_t TABLE_LEN = strlen(TABLE);
		const char* token = NULL;
		uint32_t token_length = 0;
		
		{
			// Traverse
			const char* ptr = TABLE;
			uint32_t len = TABLE_LEN;
			printf("Tokenize: \"%.*s\"\n", len, ptr);
			while ((ptr = cfg_token(ptr, &len, ',', &token, &token_length)) != NULL) {
				printf("{%.*s}\n", token_length, token);
			} 
		}
		
		{
			// Index
			const uint32_t index = 8;
			cfg_token_index(TABLE, TABLE_LEN, ',', &token, &token_length, index);
			printf("TABLE[%d] = \"%.*s\"\n", index, token_length, token);
		}
	}
	
	{
		// Parse int
		int i;
		if (cfg_parse_int(&i, "   -123 ", 8)) {
			printf("Parse pass: %d\n", i);
		} else {
			printf("Parse failed.\n");
		}
	}
		
	return 0;
}