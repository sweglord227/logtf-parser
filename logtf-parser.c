#include <stdbool.h>
#include <bsd/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "json.h"


struct string
{
	char *response;
	size_t size;
};

void init_string(struct string *s)
{
	s->size = 0;
	s->response = malloc(s->size+1);
	if (s->response == NULL)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		exit(1);
	}
	s->response[0] = '\0';
}

size_t writefunc(void *response, size_t size, size_t nmemb, struct string *s)
{
	size_t new_size = s->size + size*nmemb;
	s->response = realloc(s->response, new_size+1);
	if (s->response == NULL)
	{
		fprintf(stderr, "Failed to reallocate memory\n");
		exit(1);
	}
	memcpy(s->response+s->size, response, size*nmemb);
	s->response[new_size] = '\0';
	s->size = new_size;
	
	return size*nmemb;
}

//query logs.tf/json/[logID] and give back json as output
void parse_log(char *logID)
{
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		//cat url
		int size = sizeof("https://logs.tf/json/") + sizeof(logID);
		char logURL[size];
		strlcpy(logURL, "https://logs.tf/json/", size);
		strlcat(logURL, logID, size);
		
		//query url
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, logURL);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		
		if(res)
		{
			curl_easy_strerror(res);
			exit(1);
		}
	}
	else
	{
		fprintf(stderr, "Failed to init CUrl\n");
		exit(1);
	}
}

//break loglist into usable urls and query for data
int parse_file(FILE *logList)
{
	int c;
	int *line = malloc(sizeof(int));
	int lineSize = 0;
	
	while( (c = getc(logList)) != EOF)
	{
		bool isNewline = false;
		
		if(c == '\n' && lineSize > 0)
		{
			isNewline = true;
			char logID[lineSize + 1];
			for(int i = 0; i < lineSize; i++)
			{
				logID[i] = line[i];
			}
			logID[lineSize] = '\0';
			parse_log(logID);
		}
		else
			line[lineSize] = c;
		
		if(isNewline)
		{ 
			free(line);
			line = malloc(sizeof(int));
			lineSize = 0;
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
		r = parse_file(fopen(argv[1], "r"));
		if(r)
		{
			fprintf(stderr, "Failed to allocate memory\n");
		}
	}
	
	return r;
}
