#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

typedef struct node {
	char path[PATHMAX];
	struct stat sb;
	struct node *next;
	struct node *child;
	int status;
	int is_dir;
} node;

typedef struct change_info {
	time_t time;
	char path[PATHMAX];
	char change[10];
} change_info;

void init();
void prompt();
void add(int argc, char **args);
void delete();
void tree(char **args);
void add_usage();

void help();

void create_daemon();
void daemon_setting();
void handler(int signo);

char *QuoteCheck(char **str, char del);
char *Tokenize(char *str, char *del);
char **GetSubstring(char *str, int *cnt, char *del);
int ConvertPath(char *origin, char *resolved);
int is_include_path(char *path1, char *path2);


void append_line(char *path, char *str);
int find_pid(char *path, char *pattern);
int find_path(char *path, char *pattern);
void delete_line_by_pid(char *path, char *pattern);
void print_tree(char *dir, int depth);

node *create_node();
node *create_tree(char *path);
int check_node(node *old, node *new);
void free_tree(node *root);
void compare_tree(node *, node*);
void check_changes(node *);
void print_changes();
char *get_time(time_t);
void sort_list();
