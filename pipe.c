#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

void report_and_exit(const char *message)
{
  int err = errno;
  perror(message);
  exit(err);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    return EINVAL;
  }
  //number of pipes needed = argc-1 (argv[0] is "./pipe")
  int num_pipes = argc-1;
  //each pipe needs 2 FDs (read & write)
  int fds[2*num_pipes];
  //initialize each pipe's read/write
  for (int p=0; p<2*num_pipes; p+=2) {
    if (pipe(fds + p) == -1) {
      report_and_exit("pipe failed");
    }
  }
  //wait status
  int wstatus;
  //loop through all argv[1]...argv[argc-1]
  for (int i=1; i<argc; i++) {
    pid_t pid = fork();
    if (pid == -1) {
      report_and_exit("fork failed");
    }
    if (pid == 0) {
      //any non-first argument's stdin is the previous argument's stdout
      if (i != 1) {
	if (dup2(fds[2*(i-1)-2], 0) == -1) {
	  report_and_exit("dup2 failed");
	}
      }
      //any non-last argument's stdout is the next argument's stdin
      if (i < argc-1) {
	if (dup2(fds[2*(i-1)+1], 1) == -1) {
	  report_and_exit("dup2 failed");
	}
      }
      //close all pipe fds in each child
      for (int i=0; i<2*num_pipes; i++) {
	close(fds[i]);
      }
      //change process to current argument
      int exec_return = execlp(argv[i], argv[i], (char *)NULL);
      if (exec_return == -1) {
       	report_and_exit("execlp failed");
      }
    }
  }
  //parent closes all pipe fds
  for (int i=0; i<2*num_pipes; i++) {
    close(fds[i]);
  }
  //parent waits for children
  for (int i=0; i<(2*num_pipes)+1; i++) {
    wait(&wstatus);
    if (!WIFEXITED(wstatus)) {
      exit(EXIT_FAILURE);
    }
    int child_exit = WEXITSTATUS(wstatus);
    if (child_exit != 0) {
      exit(child_exit);
    }
  }
  return 0;
}
