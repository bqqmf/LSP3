#include "ssu_monitor.h"

int main (int argc, char *argv[]) {

	init();

	prompt();

	exit(0);
}

// 데몬 관련 함수
char *QuoteCheck(char **str, char del) {
	char *tmp = *str+1;
	int i = 0;

	while(*tmp != '\0' && *tmp != del) {
		tmp++;
		i++;
	}
	if(*tmp == '\0') {
		*str = tmp;
		return NULL;
	}
	if(*tmp == del) {
		for(char *c = *str; *c != '\0'; c++) {
			*c = *(c+1);
		}
		*str += i;
		for(char *c = *str; *c != '\0'; c++) {
			*c = *(c+1);
		}
	}
}

void create_daemon() {
	pid_t pid;
	int fd, maxfd;

	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();
	for (fd = 0; fd < maxfd; fd++)
		close(fd);
	umask(0);
	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	exit(0);
	// while (1);
}

// Util
char *Tokenize(char *str, char *del) {
	int i = 0;
	int del_len = strlen(del);
	static char *tmp = NULL;
	char *tmp2 = NULL;

	if(str != NULL && tmp == NULL) {
		tmp = str;
	}

	if(str == NULL && tmp == NULL) {
		return NULL;
	}

	char *idx = tmp;

	while(i < del_len) {
		if(*idx == del[i]) {
			idx++;
			i = 0;
		} else {
			i++;
		}
	}
	if(*idx == '\0') {
		tmp = NULL;
		return tmp;
	}
	tmp = idx;

	while(*tmp != '\0') {
		if(*tmp == '\'' || *tmp == '\"') {
			QuoteCheck(&tmp, *tmp);
			continue;
		}
		for(i = 0; i < del_len; i++) {
			if(*tmp == del[i]) {
				*tmp = '\0';
				break;
			}
		}
		tmp++;
		if(i < del_len) {
			break;
		}
	}

	return idx;
}


char **GetSubstring(char *str, int *cnt, char *del) {
	*cnt = 0;
	int i = 0;
	char *token = NULL;
	char *templist[100] = {NULL, };
	token = Tokenize(str, del);
	if(token == NULL) {
		return NULL;
	}

	while(token != NULL) {
		templist[*cnt] = token;
		*cnt += 1;
		token = Tokenize(NULL, del);
	}

	char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
	for (i = 0; i < *cnt; i++) {
		temp[i] = templist[i];
	}
	return temp;
}

int ConvertPath(char* origin, char* resolved) {
	char *path = (char *)malloc(sizeof(char *) * PATH_MAX);
	char *tmppath = (char *)malloc(sizeof(char *) * PATH_MAX);

	if(origin == NULL) {
		return -1;
	}

	if(origin[0] == '~') {
		sprintf(path, "%s%s", homePATH, origin+1);
	} else if (origin[0] == '.') {
		sprintf(path, "%s%s", curPATH, origin+1);
	} else if(origin[0] != '/') {
		sprintf(path, "%s/%s", curPATH, origin);
	} else {
		sprintf(path, "%s", origin);
	}

	if(!strcmp(path, "/")) {
		resolved = "/";
		return 0;
	}

	strcpy(resolved, path);

	return 0;
}

// ssu_monitor 관련 함수
void prompt() {
	char input[STRMAX];
	int argcnt = 0;
	char **arglist = NULL;
	int command;
	command_parameter parameter = {(char *) 0, (char *) 0};

	while (true) {
		printf("%d> ", STD_ID);
		fgets(input, STRMAX, stdin);
		input[strlen(input)-1] = '\0';

		if ((arglist = GetSubstring(input, &argcnt, " \t")) == NULL)
			continue;

		if (argcnt == 0) continue;

		if (!strcmp(arglist[0], commanddata[0])) {
			if (argcnt < 2) {
				help();
				continue;
			}
			add(arglist+1);
		} else if (!strcmp(arglist[0], commanddata[1])) {
			// delete
		} else if (!strcmp(arglist[0], commanddata[2])) {
			// tree 
		} else if (!strcmp(arglist[0], commanddata[3])) {
			// help 
		} else if (!strcmp(arglist[0], commanddata[4])) {
			// exit
		} else {
			// help
		}


	}
}

void init() {
	getcwd(curPATH, PATHMAX);
	sprintf(homePATH, "%s", getenv("HOME"));

	char monitor_list_path[PATHMAX];
	if (snprintf(monitor_list_path, sizeof(monitor_list_path), "%s/%s", curPATH, "monitor_list.txt") > sizeof(monitor_list_path)) {
		fprintf(stderr, "snprintf error\n");
		exit(1);
	}
	if (access(monitor_list_path, F_OK) < 0) {
		int fd;
		if ((fd = creat(monitor_list_path, 0644)) < 0) {
			fprintf(stderr, "creat error for %s\n", monitor_list_path);
			exit(1);
		}
	}

}

// args[0] : path
// args[1] : -t
void add(char **args) {
	int tOption = false;
	int sleep_time = 1;
	struct stat sb;

	char path[PATHMAX];
	if (ConvertPath(args[0], path) != 0) {
		fprintf(stderr, "invalid path %s\n", path);
		return;
	}

	if (stat(path, &sb) < 0) {
		fprintf(stderr, "%s not exists\n", path);
		return;
	}

	if (!S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "%s not DIR\n", path);
		return;
	}

	if (access(path, F_OK) < 0) {
		fprintf(stderr, "%s not exists\n", path);
		return;
	}

	// exception 3 later

	if (args[1] != NULL && !strcmp(args[1], "-t"))
		tOption = true;

	if (tOption) {
		if (args[2] == NULL || (sleep_time = atoi(args[2])) < 0) {
			add_usage();
			return;
		}
		if (sleep_time == 0) {
			if (args[2][0] >= 'a' && args[2][0] <= 'z' ||
					args[2][0] >= 'A' && args[2][0] <= 'Z') {
				add_usage();
				return;
			}
		}
	}

	// create path/log.txt
	char log_path[PATHMAX];
	if (snprintf(log_path, sizeof(log_path), "%s/%s", path, "log.txt") > sizeof(log_path)) {
		fprintf(stderr, "snprintf error\n");
		return;
	}
	if (access(log_path, F_OK) < 0) {
		int fd;
		if ((fd = creat(log_path, 0644)) < 0) {
			fprintf(stderr, "creat error for %s\n", log_path);
			return;
		}
	}

	printf("monitoring started (%s), %d\n", path, sleep_time);

	// 데몬 생성
	pid_t daemon_pid;
	if ((daemon_pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
	} else if (daemon_pid == 0) {
		create_daemon();
	}

}

void add_usage() {
	printf("add <DIRPATH> [OPTION] <TIME>\n");
}
void help() {
	printf("help\n");
}

