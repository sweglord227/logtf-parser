#include <stdbool.h>
#include <bsd/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>


struct memory
{
	char *content;
	size_t size;
};
 
static size_t cb(void *data, size_t size, size_t nmemb, void *clientp)
{
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)clientp;
	
	char *ptr = realloc(mem->content, mem->size + realsize + 1);
	if(ptr == NULL)
		return 0;  /* out of memory! */
	
	mem->content = ptr;
	memcpy(&(mem->content[mem->size]), data, realsize);
	mem->size += realsize;
	mem->content[mem->size] = 0;
	
	return realsize;
}

//query logs.tf/json/[logID] and give back json as output
int parse_log(char *logID)
{
	printf("Processing log %s...\n", logID);
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		struct memory rawJson = {0};
		//cat url
		int size = sizeof("https://logs.tf/json/") + sizeof(logID);
		char logURL[size];
		strlcpy(logURL, "https://logs.tf/json/", size);
		strlcat(logURL, logID, size);
		
		//curl options
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, logURL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rawJson);
		
		//call url for json
		res = curl_easy_perform(curl);
		if(res)
		{
			curl_easy_strerror(res);
			return -1;
		}
		
		//parse json
		cJSON *log = cJSON_ParseWithLength(rawJson.content, rawJson.size);
		free(rawJson.content);
		if (log == NULL)
		{
			const char *err = cJSON_GetErrorPtr();
			if(err != NULL)
				fprintf(stderr, "Error before: %s\n", err);
			return -1;
		}
		
		int r = cJSON_GetObjectItem(log, "length")->valueint;
		if(!r)
		{
			fprintf(stderr, "Old log type unsupported");
			return -1;
		}
		
		//cleanup
		cJSON_Delete(log);
		curl_easy_cleanup(curl);
		
		printf("Match Time: %i sec\n", r);
		return r;
	}
	else
	{
		fprintf(stderr, "Failed to init CUrl\n");
		return -1;
	}
}

//break loglist into usable urls and query for data
int scan_file(FILE *logList)
{
	int c;
	int *line = malloc(sizeof(int));
	int lineSize = 0;
	int avgLength = 0;
	int totalLines = 0;
	
	while( (c = getc(logList)) != EOF)
	{
		bool isNewline = false;
		
		if(c == '\n' && lineSize > 0)
		{
			isNewline = true;
			char logID[lineSize];
			for(int i = 0; i < lineSize; i++)
			{
				logID[i] = line[i];
			}
			logID[lineSize] = '\0';
			avgLength += parse_log(logID);
		}
		else
			line[lineSize] = c;
		
		if(isNewline)
		{ 
			free(line);
			line = malloc(sizeof(int));
			lineSize = 0;
			totalLines++;
		}
		else
		{
			lineSize++;
			int *backup;
			backup = line;
			line = realloc(line, sizeof(int) * (lineSize + 1));
			if(line == NULL)
			{
				fprintf(stderr, "Failed to reallocate memory\n");
				free(backup);
				exit(1);
			}
		}
	}
	
	fclose(logList);
	if(line)
		free(line);
	
	if(totalLines > 0)
		printf("%i seconds average per match\n", avgLength/totalLines);
	else
		printf("%i seconds in match\n", avgLength);
	return 0;
}

int main(int argc, char *argv[])
{
	u_int8_t r = 0;
	
	//exit if too many or too few args given
	if(argc > 2)
	{
		fprintf(stderr, "Too many arguments\n");
		r = 1;
	}
	else if(argc < 2)
	{
		fprintf(stderr, "Too few arguments\n");
		r = 1;
	}
	else
	{
		r = scan_file(fopen(argv[1], "r"));
		if(r)
		{
			fprintf(stderr, "Failed to allocate memory\n");
		}
	}
	
	return r;
}
