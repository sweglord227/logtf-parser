//  Copyright (C) 2023 sweglord227
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with this program.
//  If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.

#include <stdbool.h>
#include <bsd/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>


const char *argp_program_version = "logtf-parser 1.0";
const char *argp_program_bug_address = "<https://github.com/sweglord227/logtf-parser/issues>";
static char doc[] = "\nCopyright (C) 2023  sweglord227\n\n\
This program comes with ABSOLUTELY NO WARRANTY;\n\
for details type `logtf-parser --warranty'.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; type `logtf-parser --copyright' for details.";

static struct argp_option options[] =
{
	{0,0,0,0, "Search Options:" },
	{"player-search",   'p', 0, 0, "Player search"},
	{"uploader-search", 'u', 0, 0, "Uploader search"},
	{"map-search",      'm', 0, 0, "Map search"},
	{"title-search",    't', 0, 0, "Log title search"},
	
	{0,0,0,0, "Modify Search Results:" },
	{"limit-results",    'l', 0, 0, "Limit search results Deault: 1000"},
	{"offset-results",   'o', 0, 0, "Offset search results Default: 0"},
	
	{0,0,0,0, "Copyright and Warranty Information:" },
	{"copyright",  0, 0, 0, "Display copyright information"},
	{"warranty",   0, 0, 0, "Display warranty information"},
	{ 0 }
};

struct arguments
{
	char *arg1;
	char *map, *uploader, *player, *title, **strings;
	int limit, offset;
};

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

struct matchInfo parse_json(struct memory rawJson)
{
	bool r = false;
	struct matchInfo match = {};
	cJSON *log = cJSON_ParseWithLength(rawJson.content, rawJson.size);
	
	if (log == NULL)
	{
		const char *err = cJSON_GetErrorPtr();
		if(err != NULL)
			fprintf(stderr, "Error before: %s\n", err);
		else
			fprintf(stderr, "Error in Json Parse");
		r = true;
	}
	
	if(!r)
	{
		match.numRounds = 0;
		match.length = cJSON_GetObjectItem(log, "length")->valueint;
		if(!match.length)
		{
			fprintf(stderr, "Old log type unsupported");
			r = true;
		}
		if(!r)
		{
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
		}
	}
	cJSON_Delete(log);
	return match;
}

//query https://logs.tf/json/[logID] and give back json as output
struct memory get_log(char *logID)
{
	struct memory rawJson = {0};
	printf("Processing log %s...\n", logID);
	
	CURL *curl = curl_easy_init();
	if(curl) 
	{
		int size = sizeof("https://logs.tf/json/") + sizeof(logID);
		char logURL[size];
		strlcpy(logURL, "https://logs.tf/json/", size);
		strlcat(logURL, logID, size);
		
		curl_easy_setopt(curl, CURLOPT_URL, logURL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rawJson);
		
		CURLcode res;
		if( (res = curl_easy_perform(curl)) )
			curl_easy_strerror(res);
		else
			parse_json(rawJson);
		
		curl_easy_cleanup(curl);
	}
	else
	{
		fprintf(stderr, "Failed to init cURL\n");
		exit(1);
	}
	return rawJson;
}

void save_line(int *line, int lineSize, char *id)
{
	for(int i = 0; i < lineSize; i++)
		id[i] = line[i];
}

int read_file(FILE *file, char **idList)
{
	bool err = false;
	int c;
	int *line = malloc(sizeof(int));
	int totalLines = 0;
	int lineSize = 0;
	
	if(line == NULL || idList == NULL)
		err = true;
	
	while( (c = getc(file)) != EOF && !err)
	{
		bool isNewline = false;
		
		if(c == '\n' && lineSize > 0)
		{
			isNewline = true;
			if( (idList[totalLines] = malloc(sizeof(char) * lineSize)) )
				save_line(line, lineSize, idList[totalLines]);
			else
				err = true;
		}
		else
			line[lineSize] = c;
		
		if(isNewline)
		{ 
			free(line);
			if( !(line = malloc(sizeof(int))) )
				err = true;
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
				free(backup);
				err = true;
			}
		}
	}
	
	fclose(file);
	if(line)
		free(line);
	if(err)
		totalLines = -1;
	return totalLines;
}

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;
	
	switch (key)
		{
		case 'p':
			arguments->player = arg;
			break;
		case 'u':
			arguments->uploader = arg;
			break;
		case 'm':
			arguments->map = arg;
			break;
		case 't':
			arguments->title = arg;
			break;
		case 'l':
			arguments->limit = arg ? atoi (arg) : 1000;
			break;
		case 'o':
			arguments->offset = arg ? atoi (arg) : 0;
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
		case ARGP_KEY_ARG:
			arguments->arg1 = arg;
			arguments->strings = &state->argv[state->next];
			state->next = state->argc;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
		}
	return 0;
}

static struct argp argp = {options, parse_opt, NULL /*args_doc*/, doc};

int main(int argc, char *argv[])
{
	struct arguments arguments;
	arguments.limit = 1000;
	arguments.offset = 0;
	
	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	
	char **idList = malloc(sizeof(char*));
	u_int8_t r = 0;
	int numIds = read_file(fopen(argv[1], "r"), idList);
	if(numIds != -1)
	{
		for(int i = 0; i < numIds; i++)
		{
			struct memory rawJson = get_log(idList[i]);
			parse_json(rawJson);
		}
		free(idList);
	}
	else
	{
		fprintf(stderr, "Mem failure\n");
		free(idList);
		r = 1;
	}
	return r;
}
