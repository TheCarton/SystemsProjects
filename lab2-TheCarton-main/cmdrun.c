/*
 * Skeleton code for Lab 2 - Shell processing
 * This file contains skeleton code for executing parsed commands.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "cmdparse.h"
#include "cmdrun.h"

/* cmd_exec(cmd, pass_pipefd)
 *
 *   Execute the single command specified in the 'cmd' command structure.
 *
 *   The 'pass_pipefd' argument is used for pipes.
 *
 *     On input, '*pass_pipefd' is the file descriptor that the
 *     current command should use to read the output of the previous
 *     command. That is, it's the "read end" of the previous
 *     pipe, if there was a previous pipe; if there was not, then
 *     *pass_pipefd will equal STDIN_FILENO.
 *
 *     On output, cmd_exec should set '*pass_pipefd' to the file descriptor
 *     used for reading from THIS command's pipe (so that the next command
 *     can use it). If this command didn't have a pipe -- that is,
 *     if cmd->controlop != PIPE -- then this function should set
 *     '*pass_pipefd = STDIN_FILENO'.
 *
 *   Returns the process ID of the forked child, or < 0 if some system call
 *   fails.
 *
 *   Besides handling normal commands, redirection, and pipes, you must also
 *   handle three internal commands: "cd", "exit", and "our_pwd".
 *   (Why must "cd", "exit", and "our_pwd" (a version of "pwd") be implemented
 *   by the shell, versus simply exec()ing to handle them?)
 *
 *   Note that these special commands still have a status!
 *   For example, "cd DIR" should return status 0 if we successfully change
 *   to the DIR directory, and status 1 otherwise.
 *   Thus, "cd /tmp && echo /tmp exists" should print "/tmp exists" to stdout
 *      if and only if the /tmp directory exists.
 *   Not only this, but redirections should work too!
 *   For example, "cd /tmp > foo" should create an empty file named 'foo';
 *   and "cd /tmp 2> foo" should print any error messages to 'foo'.
 *
 *   Some specifications:
 *
 *       --subshells:
 *         the exit status should be either 0 (if the last
 *         command would have returned 0) or 5 (if the last command
 *         would have returned something non-zero). This is not the
 *         behavior of bash.
 *
 *       --cd:
 *
 *          this builtin takes exactly one argument besides itself (this
 *          is also not bash's behavior). if it is given fewer
 *          ("cd") or more ("cd foo bar"), that is a syntax error.  Any
 *          error (syntax or system call) should result in a non-zero
 *          exit status. Here is the specification for output:
 *
 *                ----If there is a syntax error, your shell should
 *                display the following message verbatim:
 *                   "cd: Syntax error! Wrong number of arguments!"
 *
 *                ----If there is a system call error, your shell should
 *                invoke perror("cd")
 *
 *       --our_pwd:
 *
 *          This stands for "our pwd", which prints the working
 *          directory to stdout, and has exit status 0 if successful and
 *          non-zero otherwise. this builtin takes no arguments besides
 *          itself. Handle errors in analogy with cd. Here, the syntax
 *          error message should be:
 *
 *              "pwd: Syntax error! Wrong number of arguments!"
 *
 *       --exit:
 *
 *          As noted in the lab, exit can take 0 or 1 arguments. If it
 *          is given zero arguments (besides itself), then the shell
 *          exits with command status 0. If it is given one numerical
 *          argument, the shell exits with that numerical argument. If it
 *          is given one non-numerical argument, do something sensible.
 *          If it is given more than one argument, print an error message,
 *          and do not exit.
 *
 *
 *   Implementation hints are given in the function body.
 */

int is_wait_cmd(command_t *cmd) {
    return
    ((cmd->controlop == CMD_AND) ||
    (cmd->controlop == CMD_OR) ||
    (cmd->controlop == CMD_END) ||
    (cmd->controlop == CMD_SEMICOLON));
}

int is_continue_cmd(command_t *cmd) {
    return (cmd->controlop == CMD_PIPE || cmd->controlop == CMD_BACKGROUND);
}

static pid_t subshell_exec(command_t *cmd, int *pass_pipefd) {
    (void) pass_pipefd;      // get rid of unused warning
    pid_t pid = -1;		// process ID for child
    int pipefd[2];

    if (cmd->controlop == CMD_PIPE) {
        pipe(pipefd);
    }

    pid = fork();
    if (pid == 0) {
        int subshell_status;
        char path[MAXTOKENS];
        sprintf(path, "/usr/bin/%s", cmd->argv[0]);
        char cwd[MAXTOKENS];
        getcwd(cwd, 300); // malloc

        if (cmd->redirect_filename[1] != NULL) {
            char redirect_filepath[MAXFILEPATH];
            sprintf(redirect_filepath, "%s/%s", cwd, cmd->redirect_filename[1]);
            int fd = open(redirect_filepath, O_RDWR|O_CREAT|O_TRUNC, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } if (cmd->redirect_filename[0] != NULL) {
            char rd_fp_in[MAXFILEPATH];
            sprintf(rd_fp_in, "%s/%s", cwd, cmd->redirect_filename[0]);
            int fd = open(rd_fp_in, O_RDONLY, 0666);
            dup2(fd, STDIN_FILENO);
            close(fd);
        } if (cmd->redirect_filename[2] != NULL) {
            char redirect_filepath[MAXFILEPATH];
            sprintf(redirect_filepath, "%s/%s", cwd, cmd->redirect_filename[2]);
            int fd = open(redirect_filepath, O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(fd, STDERR_FILENO);
            close(fd);
        } if (cmd->controlop == CMD_PIPE) { // left side of pipe
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }
        subshell_status = cmd_line_exec(cmd->subshell);
        exit(subshell_status);
    }
    if (cmd->controlop == CMD_PIPE) {
        *pass_pipefd = pipefd[0];
        close(pipefd[1]);
    } else {
        *pass_pipefd = STDIN_FILENO;
    }
    return pid;
}



static pid_t cmd_exec(command_t *cmd, int *pass_pipefd) {
    (void)pass_pipefd;      // get rid of unused warning
	pid_t pid = -1;		// process ID for child
	int pipefd[2] = {STDIN_FILENO, STDOUT_FILENO};		// file descriptors for this process's pipe

    if (cmd->subshell != NULL) {
        return subshell_exec(cmd, pass_pipefd);
    }
    if (cmd->controlop == CMD_PIPE) {
        pipe(pipefd);
    }




    // "cd","exit","our_pwd" Hints:
    //    Recall from the comments earlier that you need to return a status,
    //    and do redirections. How can you do these things for a
    //    command executed in the parent shell?
    //    Answer: It's easiest if you fork a child ANYWAY!
    //    You should divide functionality between the parent and the child.
    //    Some functions will be executed in each process. For example,
    //         --in the case of "cd", both parent and child should
    //         try to change directory (use chdir()), but only the child
    //         should print error messages
    //         --in the case of "our_pwd", see "man getcwd"
    //
    //    For the "cd" command, you should change directories AFTER
    //    the fork(), not before it.  Why?
    //    Design some tests with 'bash' that will tell you the answer.
    //    For example, try "cd /tmp ; cd $HOME > foo".  In which directory
    //    does foo appear, /tmp or $HOME?  If you chdir() BEFORE the fork,
    //    in which directory would foo appear, /tmp or $HOME?

    if (strcmp(cmd->argv[0], "exit") == 0) {

        if (cmd->argv[1] == NULL) {
            exit(0);
        } else if (cmd->argv[2] != NULL) {
            perror("Wrong number of arguments to exit");
        } else {
            int status = atoi(cmd->argv[1]);
            cmd_free(cmd);
            exit(status);
        }
    }


    pid = fork();
    if (pid == 0) {
        char cwd[MAXTOKENS];
        getcwd(cwd, 300); // malloc

        if (strcmp(cmd->argv[0], "our_pwd") == 0) {
            if (getcwd(cwd, 300) == NULL) {
                exit(1);
            }
            printf("%s", cwd);
            exit(0);
        }
        if (strcmp(cmd->argv[0], "cd") == 0) {
            int status = chdir(cmd->argv[1]);
            exit(status);
        }

        if (cmd->redirect_filename[1] != NULL) {
            char redirect_filepath[MAXFILEPATH];
            sprintf(redirect_filepath, "%s/%s", cwd, cmd->redirect_filename[1]);
            int fd = open(redirect_filepath, O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } if (cmd->redirect_filename[2] != NULL) {
            char redirect_filepath[MAXFILEPATH];
            sprintf(redirect_filepath, "%s/%s", cwd, cmd->redirect_filename[2]);
            int fd = open(redirect_filepath, O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(fd, STDERR_FILENO);
            close(fd);
        } if (cmd->redirect_filename[0] != NULL) {
            char rd_fp_in[MAXFILEPATH];
            sprintf(rd_fp_in, "%s/%s", cwd, cmd->redirect_filename[0]);
            int fd = open(rd_fp_in, O_RDONLY, 0666);
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else if (cmd->controlop == CMD_PIPE) { // left side of pipe
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        } if (*pass_pipefd != 0) { // right side of pipe
            dup2(*pass_pipefd, STDIN_FILENO);
            close(*pass_pipefd);
        }
        char path[MAXTOKENS];
        sprintf(path, "/usr/bin/%s", cmd->argv[0]);
        execve(path, cmd->argv, NULL);
    }
    char cwd[MAXTOKENS];
    getcwd(cwd, 300); // malloc
    // parent
    // In the parent, you should:
    //    1. Close some file descriptors.  Hint: Consider the write end
    //       of this command's pipe, and one other fd as well.
    //    2. Handle the special "exit", "cd", and "our_pwd" commands.
    //    3. Set *pass_pipefd as appropriate.

    if (strcmp(cmd->argv[0], "cd") == 0) {
        chdir(cmd->argv[1]);
    }


    if (cmd->controlop == CMD_PIPE) {
        *pass_pipefd = pipefd[0];
        close(pipefd[1]);
    } else {
        *pass_pipefd = STDIN_FILENO;
    }
    return pid;
}



/* cmd_line_exec(cmdlist)
 *
 *   Execute the command list.
 *
 *   Execute each individual command with 'cmd_exec'.
 *   String commands together depending on the 'cmdlist->controlop' operators.
 *   Returns the exit status of the entire command list, which equals the
 *   exit status of the last completed command.
 *
 *   The operators have the following behavior:
 *
 *      CMD_END, CMD_SEMICOLON
 *                        Wait for command to exit.  Proceed to next command
 *                        regardless of status.
 *      CMD_AND           Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status 0.  Otherwise
 *                        exit the whole command line.
 *      CMD_OR            Wait for command to exit.  Proceed to next command
 *                        only if this command exited with status != 0.
 *                        Otherwise exit the whole command line.
 *      CMD_BACKGROUND, CMD_PIPE
 *                        Do not wait for this command to exit.  Pretend it
 *                        had status 0, for the purpose of returning a value
 *                        from cmd_line_exec.
 */
int
cmd_line_exec(command_t *cmdlist)
{
	int cmd_status = 0;	    // status of last command executed
	int pipefd = STDIN_FILENO;  // read end of last pipe
    int wp_status;
    pid_t pid;
    // Use for waitpid's status argument!
    // Read the manual page for waitpid() to
    // see how to get the command's exit
    // status (cmd_status) from this value.

	while (cmdlist) {
		// EXERCISE 4: Fill out this function!
		// If an error occurs in cmd_exec, feel free to abort().
        pid = cmd_exec(cmdlist, &pipefd);
        if (is_wait_cmd(cmdlist)){
            waitpid(pid, &wp_status, 0);
            cmd_status = WEXITSTATUS(wp_status);
        } else if (is_continue_cmd(cmdlist)) {
            cmdlist = cmdlist->next;
            cmd_status = 0;
            continue;
        }
        if (((cmdlist->controlop == CMD_AND) && (cmd_status != 0)) || (((cmdlist->controlop == CMD_OR) && (cmd_status == 0)))) {
            cmdlist = cmdlist->next;
            if (!cmdlist) break;
        }
		cmdlist = cmdlist->next;
	}

        while (waitpid(0, &wp_status, WNOHANG) > 0);

done:
	return cmd_status;
}
