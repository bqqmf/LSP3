#include "ssu_monitor.h"

// status 0 : 생성, 2 : 수정, 3 : 삭제
change_info change_list[1000];
int change_idx;

int main (int argc, char *argv[]) {

	init();

	prompt();
//	debug(new_root);

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

void create_daemon(char *path, char *log_path, int sleep_time) {

	daemon_setting();

	node *old_root = create_tree(path);

	while (1) {
		node *new_root= create_tree(path);

		change_idx = 0;
		memset(change_list, 0, sizeof(change_info) * 1000);
		compare_tree(old_root, new_root);  // 수정되었거나 그대로인 파일 체크
		check_changes(new_root);
		print_changes(log_path);

		free_tree(old_root);
		old_root = new_root;

		sleep(sleep_time);
	}

}

void daemon_setting() {
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
	signal(SIGUSR1, handler);
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

/* find_path()에서 호출됨 */
/* path1과 path2가 서로 포함하는지 검사 */
/* return 1 : 포함, 0 : 미포함 */
int is_include_path(char *path1, char *path2) {
	int i;
	int cnt1, cnt2;
	char tmp1[PATHMAX], tmp2[PATHMAX];
	strcpy(tmp1, path1);
	strcpy(tmp2, path2);
	char **pathlist1 = GetSubstring(tmp1, &cnt1, "/");
	char **pathlist2 = GetSubstring(tmp2, &cnt2, "/");


	if(cnt1==cnt2) {
		for(i = 0; i < cnt1; i++) {
			if(!strcmp(pathlist1[i], pathlist2[i]))
				continue;
			return 0;
		}
		return 1;
	} else if (cnt1 > cnt2) {
		for(i = 0; i < cnt2; i++) {
			if(!strcmp(pathlist1[i], pathlist2[i]))
				continue;
			return 0;
		}
		return 1;
	} else {
		for(i = 0; i < cnt1; i++) {
			if(!strcmp(pathlist1[i], pathlist2[i]))
				continue;
			return 0;
		}
		return 1;
	}
}

// ssu_monitor 관련 함수
void prompt() {
	char input[PATHMAX];
	int argcnt = 0;
	char **arglist = NULL;
	int command;
	command_parameter parameter = {(char *) 0, (char *) 0};

	while (true) {
		printf("%d> ", STD_ID);
		fgets(input, PATHMAX, stdin);
		input[strlen(input)-1] = '\0';

		if ((arglist = GetSubstring(input, &argcnt, " \t")) == NULL)
			continue;

		if (argcnt == 0) continue;

		if (!strcmp(arglist[0], commanddata[0])) {
			if (argcnt < 2) {
				help();
				continue;
			}
			add(argcnt, arglist+1);
		} else if (!strcmp(arglist[0], commanddata[1])) {
			if (argcnt < 2) {
				help();
				continue;
			}
			delete(arglist[1]);
		} else if (!strcmp(arglist[0], commanddata[2])) {
			if (argcnt != 2) {
				help();
				continue;
			}
			tree(arglist + 1);
		} else if (!strcmp(arglist[0], commanddata[3])) {
			help();	
		} else if (!strcmp(arglist[0], commanddata[4])) {
			exit(0);
		} else {
			help();	
		}


	}
}

void init() {
	getcwd(curPATH, PATHMAX);
	sprintf(homePATH, "%s", getenv("HOME"));

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

	memset(change_list, 0, sizeof(change_info) * 1000);
	change_idx = 0;

}

// args[0] : path
// args[1] : -t
// args[2] : time
void add(int argc, char **args) {
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

	if (argc > 2 && args[1] != NULL && !strcmp(args[1], "-t"))
		tOption = true;

	if (tOption) {
		char tmp[PATHMAX];
		strcpy(tmp, args[2]);
		char *p = tmp;
		while (*p != '\0') {
			if (!isdigit(*p)) {
				// 숫자가 아닌 문자 발견	
				add_usage();
				return ;  
			}
			p++;
		}
		sleep_time = atoi(args[2]);
	}

	int find = find_path(monitor_list_path, path);
	if (find == 1) {
		fprintf(stderr, "%s cannot be monitored\n", path);
		return;
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
		create_daemon(path, log_path, sleep_time);
	} else {
		// monitor_list.txt에 한 줄 추가
		char tmp[PATHMAX];
		if (snprintf(tmp, sizeof(tmp), "%s %d\n", path, daemon_pid) > sizeof(tmp)) {
			fprintf(stderr, "tmp over PATHMAX\n");
			return;
		}
		append_line(monitor_list_path, tmp);
	}

}

void delete(char *pid) {
	int find = find_pid(monitor_list_path, pid);

	if (find == 1) {
		delete_line_by_pid(monitor_list_path, pid);
	}
	else {
		fprintf(stderr, "%s not exists in %s\n", pid, monitor_list_path);
		return;
	}
}

void tree(char **args) {
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

	printf("%s\n", args[0]);
	print_tree(path, 0);

}

void add_usage() {
	printf("add <DIRPATH> [OPTION] <TIME>\n");
}
void help() {
	printf("add <DIRPATH> [OPTION]\n");
	printf("delete <DAEMON_PID>\n");
	printf("tree <DIRPATH>\n");
	printf("help\n");
	printf("exit\n");
}

void append_line(char *path, char *str) {
	FILE *fp;

	if ((fp = fopen(path, "a+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	fprintf(fp, "%s", str);

	fclose(fp);
}

int find_pid(char *path, char *pattern) {
	FILE *fp;

	if ((fp = fopen(path, "a+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}

	char line[PATHMAX];
	char pid[PATHMAX];

	while (fscanf(fp, "%s %s\n", line, pid) != EOF) {
		if (!strcmp(pattern, pid))
			return 1;
	}

	return 0;
}

/* add() 에서 호출됨 */
/* monitor_list.txt에서 인자로 들어온 경로가 있는지 검사 */
/* return 1 : 발견함, 0 : 발견 못함 */
int find_path(char *path, char *pattern) {
	FILE *fp;

	if ((fp = fopen(path, "a+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}

	char line[PATHMAX];
	char pid[PATHMAX];

	while (fscanf(fp, "%s %s\n", line, pid) != EOF) {
		if (is_include_path(line, pattern))
			return 1;
	}

	return 0;
}

void delete_line_by_pid(char *path, char *pattern) {
	FILE *fp, *fp_tmp;

	if ((fp = fopen(path, "a+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}
	if ((fp_tmp = fopen("tmpfile", "a+")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", "tmpfile");
		exit(1);
	}

	char line[PATHMAX];
	char pid[PATHMAX];

	while (fscanf(fp, "%s %s\n", line, pid) != EOF) {
		if (!strcmp(pid, pattern)) {
			printf("monitoring ended (%s)\n", line);
			kill(atoi(pid), SIGUSR1);  // send SIGUSR1 to pid
			continue;
		}
		fprintf(fp_tmp, "%s %s\n", line, pid);
	}

	fclose(fp_tmp);
	fclose(fp);

	remove(path);
	rename("tmpfile", path);
}

void print_tree(char *dir, int depth) {
	struct stat sb;
	struct dirent** namelist;
	int count;

	// 디렉토리 경로 설정
	const char* dir_path = dir;

	// 디렉토리 스캔
	count = scandir(dir_path, &namelist, NULL, alphasort);

	// 파일 목록 출력
	for (int i = 0; i < count; i++) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		char fullpath[PATHMAX];
		if (snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, namelist[i]->d_name) > 
				sizeof(fullpath)) {
			fprintf(stderr, "snprintf error\n");
			continue;
		}

		if (lstat(fullpath, &sb) < 0) {
			fprintf(stderr, "lstat error for %s\n", namelist[i]->d_name);
			continue;
		}

		if (S_ISDIR(sb.st_mode)) {
			for (int i = 0; i < depth; i ++) 
				printf("    ");
			printf("----%s\n", namelist[i]->d_name);
			print_tree(namelist[i]->d_name, depth + 1);
		}
		else if (S_ISREG(sb.st_mode)) {
			for (int i = 0; i < depth; i ++) 
				printf("    ");
			printf("----%s\n", namelist[i]->d_name);
		}
		free(namelist[i]);
	}

	free(namelist);
}

node *create_node() {
	node *new_node = (node *)calloc(sizeof(node), 1);

	memset(new_node->path, 0, PATHMAX);
	new_node->next = NULL;
	new_node->child = NULL;
	new_node->status = 0;
	new_node->is_dir = 0;

	return new_node;
}

node *create_tree(char *path) {
	node *parent = (node *)calloc(sizeof(node), 1);

	strcpy(parent->path, path);
	if (lstat(path, &(parent->sb)) < 0) {
		fprintf(stderr, "lstat error for %s\n", path);
		return NULL;
	}
	parent->is_dir = 1;

	node *cur = parent;  // 마지막에 추가된 노드

	struct dirent** namelist;
	int count;

	count = scandir(path, &namelist, NULL, alphasort);

	for (int i = 0; i < count; i++) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")
				|| !strcmp(namelist[i]->d_name, "log.txt"))
			continue;

		node *new= (node *)calloc(sizeof(node), 1);
		sprintf(new->path, "%s/%s", path, namelist[i]->d_name);

		if (lstat(new->path, &(new->sb)) < 0) {
			free(new);
			continue;
		}

		if (S_ISDIR(new->sb.st_mode)) {
			new = create_tree(new->path);
		}

		if (parent->child == NULL) {
			parent->child = new;
			cur = new;
		} else {
			cur->next = new;
			cur = new;
		}
	}
	return parent;
}

int check_node(node *old, node *new) {
	if (new == NULL)
		return 0;

	// old와 new가 같은 파일이라면
	if (!strcmp(old->path, new->path)) {
		if (old->sb.st_mtime != new->sb.st_mtime)  // 수정 시간이 다르면
			new->status = 2;  // 수정됨(2)
		else
			new->status = 1;  // 확인완료(1)
		return 1;
	}

	if (check_node(old, new->child))
		return 1;
	if (check_node(old, new->next))
		return 1;

	return 0;
}

void free_tree(node *root) {
	if (root->child != NULL)
		free_tree(root->child);
	if (root->next != NULL)
		free_tree(root->next);

	free(root);
}

void compare_tree(node *old, node *new) {
	if (old == NULL)
		return;

	int removed = check_node(old, new);
	if (!removed) {
		printf("%s removed\n", old->path);
		change_list[change_idx].time = time(NULL);
		strcpy(change_list[change_idx].path, old->path);
		strcpy(change_list[change_idx++].change, "remove");
	}

	if (old->child != NULL)
		compare_tree(old->child, new);
	if (old->next != NULL)
		compare_tree(old->next, new);

}

void debug(node *root) {
	printf("%s %d %d\n", root->path, root->is_dir, root->status);
	if (root->child != NULL)
		debug(root->child);
	if (root->next != NULL)
		debug(root->next);
}

void check_changes(node *cur) {

	if (!cur->is_dir) {
		if (cur->status == 0) {
			printf("%s created\n", cur->path);
			change_list[change_idx].time = cur->sb.st_mtime;
			strcpy(change_list[change_idx].path, cur->path);
			strcpy(change_list[change_idx++].change, "create");
		}
		else if (cur->status == 2) {
			printf("%s modified\n", cur->path);
			change_list[change_idx].time = cur->sb.st_mtime;
			strcpy(change_list[change_idx].path, cur->path);
			strcpy(change_list[change_idx++].change, "modify");
		}
	}

	if (cur->child != NULL)
		check_changes(cur->child);
	if (cur->next != NULL)
		check_changes(cur->next);

}

void print_changes(char *log_path) {
	sort_list();

	for (int i=0; i < change_idx; i++) {
		change_info info = change_list[i];
		char tmp[PATHMAX];
		if (snprintf(tmp, sizeof(tmp), "[%s][%s][%s]\n", get_time(info.time), info.change, info.path) > sizeof(tmp)) {
			return;
		}

		append_line(log_path, tmp);
	}
}

char *get_time(time_t time) {
	static char buf[100] = {0};
	struct tm *tm = localtime(&time);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);

	return buf;
}

void sort_list() {
	change_info temp;
	for (int i=0; i < change_idx; i++) {
		for (int j=i+1; j < change_idx; j++) {
			if (change_list[i].time > change_list[j].time) {
				temp = change_list[i];
				change_list[i] = change_list[j];
				change_list[j] = temp;
			}
		}
	}
}

void handler(int signo) {
	exit(0);
}
