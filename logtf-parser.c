#include <stdbool.h>
#include <bsd/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>


//query logs.tf/json/[logNum] and give back json as output
void getLogJson(char *logNum)
{
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		//cat url
		int size = sizeof("https://logs.tf/json/") + sizeof(logNum);
		char logURL[size];
		strlcpy(logURL, "https://logs.tf/json/", size);
		strlcat(logURL, logNum, size);
		
		//query url
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, logURL);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		printf("\n%s\n", logURL);
		
		if(res)
		{
			fprintf(stderr, "URL error %i", res);
		}
	}
	else
	{
		fprintf(stderr, "Failed to init CUrl");
	}
}

//break loglist into usable urls and query for data
int parseLog(FILE *logList)
{
	int c;
	int *line = malloc(sizeof(int));
	int lineSize = 0;
	
	while( (c = getc(logList)) != EOF)
	{
		bool isNewline = false;
		
		if(c == '\n')
		{
			isNewline = true;
			getLogJson((char*)line);
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
				free(backup);
				return 1;
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
		r = parseLog(fopen(argv[1], "r"));
		if(r)
		{
			fprintf(stderr, "Failed to allocate memory\n");
		}
	}
	
	return r;
}
