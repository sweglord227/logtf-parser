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

struct matchInfo
{
	int length;
	int numRounds;
};
 
static size_t cb(void *data, size_t size, size_t nmemb, void *clientp)
{
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)clientp;
	
	char *ptr = realloc(mem->content, mem->size + realsize + 1);
	if(ptr == NULL)
		return 0;
	
	mem->content = ptr;
	memcpy(&(mem->content[mem->size]), data, realsize);
	mem->size += realsize;
	mem->content[mem->size] = 0;
	
	return realsize;
}

//query https://logs.tf/json/[logID] and give back json as output
struct matchInfo parse_log(char *logID)
{
	printf("Processing log %s...\n", logID);
	
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		struct memory rawJson = {0};
		int size = sizeof("https://logs.tf/json/") + sizeof(logID);
		char logURL[size];
		strlcpy(logURL, "https://logs.tf/json/", size);
		strlcat(logURL, logID, size);
		
		curl_easy_setopt(curl, CURLOPT_URL, logURL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rawJson);
		
		CURLcode res;
		if( (res = curl_easy_perform(curl)) )
		{
			curl_easy_strerror(res);
			exit(1);
		}
		
		cJSON *log = cJSON_ParseWithLength(rawJson.content, rawJson.size);
		
		if (log == NULL)
		{
			const char *err = cJSON_GetErrorPtr();
			if(err != NULL)
				fprintf(stderr, "Error before: %s\n", err);
			exit(1);
		}
		
		struct matchInfo match = {0, 0};
		match.length = cJSON_GetObjectItem(log, "length")->valueint;
		if(!match.length)
		{
			fprintf(stderr, "Old log type unsupported");
			exit(1);
		}
		
		cJSON *teamsJson = cJSON_GetObjectItem(log, "teams");
		
		for(int i = 0; i < 2; i++)
		{
			char *teamColor;
			if(i)
				teamColor = "Red";
			else
				teamColor = "Blue";
			cJSON *team = cJSON_GetObjectItem(teamsJson, teamColor);
			match.numRounds += cJSON_GetObjectItem(team, "score")->valueint;
		}
		
		free(rawJson.content);
		cJSON_Delete(log);
		curl_easy_cleanup(curl);
		
		return match;
	}
	else
	{
		fprintf(stderr, "Failed to init cURL\n");
		exit(1);
	}
}

//break loglist into usable urls and query for data
int scan_file(FILE *logList)
{
	int c;
	int *line = malloc(sizeof(int));
	int lineSize = 0;
	int totalLogs = 0;
	struct matchInfo match = {};
	
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
			
			struct matchInfo temp = parse_log(logID);
			match.length += temp.length;
			match.numRounds += temp.numRounds;
		}
		else
			line[lineSize] = c;
		
		if(isNewline)
		{ 
			free(line);
			line = malloc(sizeof(int));
			lineSize = 0;
			totalLogs++;
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
	
	if(totalLogs > 0)
		printf("%i seconds average per match\n", match.length / totalLogs);
	else
		printf("%i seconds in match\n", match.length);
	
	//cannot divide by zero. there must be a round to have a log.
	printf("%i seconds average per round\n", match.length / match.numRounds);
	printf("%i total rounds\n", match.numRounds);
	
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
