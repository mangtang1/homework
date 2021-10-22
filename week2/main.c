/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#define MAX_COMMAND_SIZE 1024
#define SUC_SIG 1
#define FAIL_SIG -1
#define HELP -5
/*
  Function Declarations for builtin shell commands:
 */

int argc;
int stp;

char months[13][4]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char stack[50][50];

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int my_ls(char **args);
int my_mv(char **args);
int exc_chk(char *arg, char **exc, int exs);
int opt_to_int(char **args);

char* find_last_filename(char* arg)
{
  char *i=arg, *res=arg;
  while(*i) if(*i++=='/') res=i;
  return res; 
}

void interruptHandler(int sig);
void print_curdir();

void interruptHandler(int sig)
{
  if(sig == SIGINT)
  {
    printf("KeyboardInterrupt!\n");
  }
  else if(sig == SIGTSTP) exit(0);
}

void print_curdir()
{
  char *curdir = (char *)malloc(sizeof(char)*MAX_COMMAND_SIZE);
  getcwd(curdir, 1024);
  printf("%s\n",curdir);
  printf(">");
}

void ls_Inode(struct stat *buf)
{
  printf("%d\t\t", (unsigned int)buf->st_ino);
}
void ls_Mode(struct stat *buf)
{
  char pot[10]="xwrxwrxwr";
  if(buf->st_mode&S_IFDIR) printf("-");
  else if(buf->st_mode&S_IFREG) printf("d");
  for(int i=8;i>=0;--i)
  {
    if((unsigned long)buf->st_mode&1<<i) printf("%c",pot[i]);
    else printf("-");
  }
  printf(" ");
}
void ls_FSize(struct stat *buf)
{
  printf("%6ld ",buf->st_size);
}
void ls_Time(struct stat *buf)
{
  struct tm *mtime;
  mtime=localtime(&buf->st_mtime);
  printf("%s %d %02d:%02d ",months[mtime->tm_mon+1],mtime->tm_mday,mtime->tm_hour,mtime->tm_min);
}
void ls_Nlink(struct stat *buf)
{
  printf("%ld ", buf->st_nlink);
}
void ls_Uid(struct stat *buf)
{
  struct passwd *pwd;
  pwd = getpwuid(buf->st_uid);
  printf("%s ", pwd->pw_name);
}
void ls_Gid(struct stat *buf)
{
  struct group *gr = NULL;
  gr = getgrgid(buf->st_uid);
  printf("%s ", gr->gr_name);
}
/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "ls",
  "mv"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &my_ls,
  &my_mv
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return SUC_SIG;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return SUC_SIG;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}
int my_mv(char **args)
{
  if(argc != 3)
  {
    printf("Error\n");
    return FAIL_SIG;
  }
  int sw1=0,sw2=0;
  struct stat buf;
  DIR * dir1 = NULL, * dir2 = NULL;
  if ((dir1 = opendir(args[1]))!=NULL) sw1=1;
  if ((dir2 = opendir(args[2]))!=NULL) sw2=1;
  if(sw1==0&&sw2==1)
  {
    char *file_start = find_last_filename(args[1]);
    char new_name[MAX_COMMAND_SIZE]="";
    strcpy(new_name, args[2]);
    strcat(new_name,"/");
    strcat(new_name,file_start);
    rename(file_start,new_name);
    return SUC_SIG;
  }
  if(sw1==1&&sw2==0) return FAIL_SIG;
  if(rename(args[1], args[2]) == -1)
  {
    return FAIL_SIG;
  }
  return SUC_SIG;
}
/**
 @brief find files
 @param args list of args. Not examined.
 @return 1 succeed, -1 error
 */
int my_ls(char **args) 
{
  char * curdir = (char *) malloc(sizeof(char) * MAX_COMMAND_SIZE);
  DIR * dir = NULL;
  struct dirent * files = NULL;
  struct stat buf;
  int opt = opt_to_int(args);

  if(stp>0)
  {
    strcpy(curdir,stack[0]);
  }
  else getcwd(curdir, 1024);
  if((dir = opendir(curdir)) == NULL) return FAIL_SIG;
  if(opt==HELP)
  {
    printf("This is my new ls scrypt. l is long, a is all. al or la is long all!\n");
    return SUC_SIG;
  }
  long long int total_blocks=0;
  while(files = readdir(dir))
  {
    lstat(files->d_name, &buf);
    total_blocks += buf.st_blocks/2;
  }
  dir=opendir(curdir);
  if(opt&(1<<('l'-'a'))) printf("total %lld\n",total_blocks);
  while(files = readdir(dir))
  {
    if(files->d_name[0]=='.'&&!(opt&(1<<('a'-'a')))) continue;
    lstat(files->d_name, &buf);
    if(opt&(1<<('l'-'a')))
    {
      ls_Mode(&buf);
      ls_Nlink(&buf);
      ls_Uid(&buf);
      ls_Gid(&buf);
      ls_FSize(&buf);
      ls_Time(&buf);
    }
    if(opt&(1<<('l'-'a'))) {if(buf.st_mode&S_IFDIR) printf("\033[50;10;42m");
    else printf("\033[50;10;32m"); }
    printf("%s ",files->d_name);
    printf("\033[0m");
    if(opt&(1<<('l'-'a'))) printf("\n");
  }
  printf("\n");
  return SUC_SIG;
}
/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int exc_chk(char *arg, char **exc, int exs)
{
  for(int i=0;i<exs;++i)
  {
    if(!strcmp(arg,exc[i])) return FAIL_SIG;
  }
  return SUC_SIG;
}

int opt_to_int(char **args)
{
  int ret=0;
  stp=0;
  for(int i=1;i<argc;++i)
  {
    if(!strncmp(args[i],"--",2))
    {
      if(!strcmp(args[i],"--help")) return HELP;
      else return FAIL_SIG;
    }
    else if(args[i][0]=='-')
    {
      for(int j=1;j<strlen(args[i]);++j) ret|=1<<(args[i][j]-'a');
    }
    else
    {
      strcpy(stack[stp++],args[i]);
    }
  }
  return ret;
}
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  argc=position;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    print_curdir();
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  signal(SIGINT, interruptHandler);
  system("clear");
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

