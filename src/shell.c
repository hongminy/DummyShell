#include "shell_util.h"
#include "linkedList.h"
#include "helpers.h"

// Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#define PATH_MAX 4096

int terminated_child = 0;
int redirect = 0;
int in = 0;
char* inf = NULL;
int out = 0;
char* outf = NULL;
char* outfappend = NULL;
char* outfstderr = NULL;
int append = 0;
int restderr = 0;
pid_t p;
int pipeflag = 0;
char* strpiped[2] = {NULL};
int usr1sig = 0;

void chld_handler(int sig)
{
		terminated_child = 1;
}
void sigusr1_handler(int sig)
{
	usr1sig = 1;
}
void checkRedirectionOptions(char* args[], size_t num)
{
	// modify the global varible in, out
		int i = num-1;
		for (;i > 0;i--){
			if (strcmp(args[i], "<") == 0){
				// in
				redirect = 1;
				in = 1;
				if (i < num-1){
					inf = strdup(args[i+1]);
					args[i+1] = NULL;
				}
				else{
				}
				args[i] = NULL;
				i --;
			}

		 if (strcmp(args[i], ">") == 0){
				//out
				redirect = 1;
				out = 1;
				if (i < num-1){
					outf = strdup(args[i+1]);
					args[i+1] = NULL;
				}
				else
				{
				}
				args[i] = NULL;
				i--;
			}
			if (strcmp(args[i], ">>") == 0){
				 //out
				 redirect = 1;
				 append = 1;
				 if (i < num-1){
					 outfappend = strdup(args[i+1]);
					 args[i+1] = NULL;
				 }
				 else
				 {
				 }
				 args[i] = NULL;
				 i--;
			 }
			 if (strcmp(args[i], "2>") == 0){
					//out
					redirect = 1;
					restderr = 1;
					if (i < num-1){
						outfstderr = strdup(args[i+1]);
						args[i+1] = NULL;
					}
					else
					{
					}
					args[i] = NULL;
					i--;
				}

		}
}

void checkPipeOptions(const char* str)
{
	if (strchr(str, '|')==NULL) return;
	int i;
	char * dupstr = strdup(str);
	for (i = 0; i < 2; i++)
	{
		strpiped[i] = strsep(&dupstr, "|");
	}
	if (strchr(str, '|')!=NULL){
		pipeflag = 1;
	}
}

char** parsePipeArgs(char* buffer)
{
	return NULL;
}

int execPipe(char* mybuffer)
{
	pipeflag = 0;
	int execflag = 1;
	//char** parsed = strpiped;
	char* args1 [MAX_TOKENS+1];
	char* args2 [MAX_TOKENS+1];
	*args1 = NULL;
	*args2 = NULL;
	int numTokens1, numTokens2;
	numTokens1 = tokenizer(strpiped[0],args1);
	numTokens2 = tokenizer(strpiped[1],args2);
	if ((*args1== NULL) || (*args2 == NULL))
	{
		// args not complete
		fprintf(stderr, PIPE_ERR);
		execflag = 0;
	}
	else
	{
		int pipefd[2];
		pid_t p1,p2;
		if (pipe(pipefd) < 0)
		{
			// can not set up pipe
			fprintf(stderr, PIPE_ERR);
			execflag = 0;
		}

		p1 = fork();
		if (p1 == 0)
		{
			// ps 1

			dup2(pipefd[1], 1);
			close(pipefd[1]);
			close(pipefd[0]);
			int exec_result = 0;
			if (execflag == 1) exec_result = execvp(args1[0], &args1[0]);
			if (exec_result == -1)
			{
				fprintf(stderr, EXEC_ERR, args1[0]);
				return -1;
			}
			return 0;

		}
		else
		{
			// parent
			p2 = fork();
			// ps 2
			if(p2 == 0)
			{

				dup2(pipefd[0], 0);
				close(pipefd[1]);
				close(pipefd[0]);
				int exec_result;
				if (execflag == 1)exec_result = execvp(args2[0], &args2[0]);
				if (exec_result == -1)
				{

					fprintf(stderr, EXEC_ERR, args2[0]);
					return -1;
				}
				return 0;
			}

			// back to parent
			close(pipefd[1]);
			close(pipefd[0]);
			wait(NULL);
			wait(NULL);
			return 0;
			
		}
	}

}



int main(int argc, char *argv[])
{
	int i; //loop counter
	char *args[MAX_TOKENS + 1];
	char *args2[MAX_TOKENS + 1];
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
    List_t bg_list;
	int usr_exit = 0;

    //Initialize the linked list
    bg_list.head = NULL;
    bg_list.length = 0;
    bg_list.comparator = timeComparator;  // Don't forget to initialize this to your comparator!!!

	// Setup segmentation fault handler
	if(signal(SIGSEGV, sigsegv_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}

	// setup child handler
	signal(SIGCHLD, chld_handler);
	signal(SIGUSR1, sigusr1_handler);


	while(1) {
		// DO NOT MODIFY buffer
		// The buffer is dynamically allocated, we need to free it at the end of the loop
		char * const buffer = NULL;
		size_t buf_size = 0;
		// Print the shell prompt
		display_shell_prompt();
		int background = 0;

		// Read line from STDIN
		ssize_t nbytes = getline((char **)&buffer, &buf_size, stdin);
		if (usr1sig==1)
		{
			int length = bg_list.length;
			node_t* head = bg_list.head;
			int i = 0;
			for(; i < length; i++){
				ProcessEntry_t* p = head->value;
				printBGPEntry(p);
				head = head->next;
				}
				usr1sig = 0;
		}
		//kill terminated child
		if (terminated_child == 1){
			pid_t pidtoreap;
			int status;
			while ((pidtoreap = waitpid(-1, &status, WNOHANG)) > 0){
				kill(pidtoreap, SIGUSR1);
				fprintf(stdout, BG_TERM, pidtoreap, findCommandByPid(&bg_list, pidtoreap));
				removeByPid(&bg_list, pidtoreap);

			}
			terminated_child = 0; //reset the flag
		}

		// No more input from STDIN, free buffer and terminate
		if(nbytes == -1) {
			free(buffer);
			break;
		}

		// Remove newline character from buffer, if it's there
		if(buffer[nbytes - 1] == '\n')
			buffer[nbytes- 1] = '\0';

		// Handling empty strings
		if(strcmp(buffer, "") == 0) {
			free(buffer);
			continue;
		}


		// Parsing input string into a sequence of tokens
		size_t numTokens;
		*args = NULL;
		*args2 = NULL;
		//copy mybuffer before tokenizer
		char* mybuffer;
		mybuffer = strdup(buffer);
		numTokens = tokenizer(buffer, args);
		if(strcmp(args[0],"exit") == 0) {
			// Terminating the shell
			int length = bg_list.length;
			node_t* head = bg_list.head;
			int i = 0;
			for(; i < length; i++){
				ProcessEntry_t* p = head->value;
				kill(p->pid,SIGUSR1);
				fprintf(stdout, BG_TERM,p->pid, findCommandByPid(&bg_list,p->pid));
				head = head->next;
				}
			free(buffer);deleteList(&bg_list); return 0;
		}

		if (strcmp(args[0], "cd") == 0){
			// change the current working directory
			if (numTokens == 1){
				// change to HOME
				if (chdir(getenv("HOME")) != 0)
				{/*sth goes wrong*/fprintf(stderr, "%s", DIR_ERR);}
				else
				{/*change successfully*/fprintf(stdout, "%s\n", get_current_dir_name());}
			}
			else if (numTokens > 1)
			{
				//ignore the following args after path
				if (chdir(args[1]) != 0)
				{/*error*/ fprintf(stderr, "%s", DIR_ERR);}
				else
				{/*change successfully*/fprintf(stdout, "%s\n", get_current_dir_name());}
			}
			free(buffer);
			continue;

		}

		if (strcmp(args[0], "estatus") == 0){
			// Print the status of most recent terminated child process
			fprintf(stdout, "%d\n", WEXITSTATUS(exit_status));
			free(buffer);
			continue;
		}

		if (strcmp(args[numTokens-1], "&") == 0){
			// & = background process
			background = 1; // set background to 1(true)
			args[numTokens-1] = "";// replace the & with null terminator
		}
		checkRedirectionOptions(&args[0], numTokens);
		checkPipeOptions(mybuffer);
		pid = fork();   //In need of error handling......
		ProcessEntry_t* process = malloc(sizeof(ProcessEntry_t));
		process->pid = pid;
		process->cmd = strdup(mybuffer);
		time_t rawtime;
		time(&rawtime);
		process->seconds = rawtime;
		if (background == 1){insertInOrder(&bg_list, process);}
		if (pid == 0)
		{ //If zero, then it's the child process
			if (pipeflag == 1){pipeflag = 0;execPipe(mybuffer);fflush(stdout);exit(EXIT_SUCCESS);}
			int exec = 1;
			if (redirect == 1)
			{
				// redirection
				if (in == 1)
				{
					if( access( inf, F_OK ) != -1 )
					{
						freopen(inf, "r", stdin);
					}
					else
					{
						fprintf(stderr,RD_ERR);
						exec = 0;
					}

				}
				if (out == 1)
				{
					if (outf == NULL){fprintf(stderr,RD_ERR);exec = 0;}
					freopen(outf,"w+", stdout);
				}

				if (append == 1){
					if (outfappend == NULL){fprintf(stderr,RD_ERR);exec = 0;}
					freopen(outfappend, "a+", stdout);
				}
				if (restderr == 1){
					if (outfstderr == NULL){fprintf(stderr,RD_ERR);exec = 0;}
					freopen(outfstderr, "w+", stderr);
				}
			}
			if (exec == 1){exec_result = execvp(args[0], &args[0]);}
			if(exec_result == -1){ //Error checking
				printf(EXEC_ERR, args[0]);
				exit(EXIT_FAILURE);
			}
		    exit(EXIT_SUCCESS);
		}
		 else{ // Parent Process
		 	if (background == 1){
		 		// don't wait for the child
		 		free(buffer);
		 		free(mybuffer);
		 		continue;
		 	}
		 	else{ //normal mode, wait for the child to terminate
		 		//printf("before \n");
		 		fflush(stdout);
				wait_result = waitpid(pid, &exit_status, 0);
				//printf("after\n");
				if(wait_result == -1){
					printf(WAIT_ERR);
					exit(EXIT_FAILURE);
				}
			}
		}

		// Free the buffer allocated from getline
		free(buffer);
		free(mybuffer);
	}
	return 0;
}
