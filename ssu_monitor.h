#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>

#define true 1
#define false 0

#define STD_ID 20192421
#define STRMAX 4096
#define PATHMAX 4096

char curPATH[PATHMAX];
char homePATH[PATHMAX];
char monitor_list_path[PATHMAX];

char *commanddata[10] = {
	"add",
	"delete",
	"tree",
	"help",
	"exit"
};

typedef struct command_parameter {
	char *command;
	char *argv[10];
} command_parameter;

void init();
void prompt();
void add();
void delete();
void add_usage();

void help();

void create_daemon();

char *QuoteCheck(char **str, char del);
char *Tokenize(char *str, char *del);
char **GetSubstring(char *str, int *cnt, char *del);
int ConvertPath(char *origin, char *resolved);
int isIncludePath(char *path1, char *path2);


void append_line(char *path, char *str);
int find_pattern(char *path, char *pattern);
void delete_line(char *path, char *pattern);
