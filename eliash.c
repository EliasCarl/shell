#include "eliash.h"

/* Begin read-eval loop. */
int main(void)
{
    static char inbuf[BUFLEN]; 
    while (1)
    {
        printf("$ ");

        if (fgets(inbuf, BUFLEN, stdin) == NULL)
        {
            fprintf(stderr, "Error reading input\n");
            exit(EXIT_FAILURE);
        }

        /* cd manipulates current process, no fork. */
        if (has_prefix(inbuf, "cd ") == 0)
        {
            inbuf[strlen(inbuf) - 1] = '\0';
            if (chdir(inbuf + 3) < 0)
            {
                perror("Cannot cd");
            }
            continue;
        }

        int pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            cmd *command = parse_input(inbuf);
            run_command(command);
        }
        else
        {
            wait(NULL);
        }
    }
    exit(EXIT_SUCCESS);
}

void run_command(cmd *command)
{
    if (command->type == CMD_EXEC)
    {
        execv(command->data.exec.argv[0], command->data.exec.argv);
    }
    else if (command->type == CMD_REDIR)
    {
        fprintf(stderr, "Redirection not implemented.");
    }
    else if (command->type == CMD_PIPE)
    {
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            perror("Error establishing pipe.");
            exit(EXIT_FAILURE);
        }

        /* leftcmd and rightcmd must be exec cmds for now.
         * Since the parser can't handle anything else. */

        int pid = fork();
        if (pid == 0)
        {
            /* Fork a new process for leftcmd. */
            close(pipefd[0]);  // Close read end of pipe.
            fclose(stdout);    // Free stdout.
            dup(pipefd[1]);    // Open write end on stdout.
            close(pipefd[1]);  // Close pipe after duped fd.

            /* stdout now writes to pipe. */
            run_command(command->data.pipe.left);
        }

        pid = fork();
        if (pid == 0)
        {
            /* Fork a new process for rightcmd. */
            close(pipefd[1]);  // Close write end of pipe.
            fclose(stdin);     // Free stdout.
            dup(pipefd[0]);    // Open write end on stdout.
            close(pipefd[0]);  // Close pipe after duped fd.

            /* stdout now writes to pipe. */
            run_command(command->data.pipe.right);
        }

        /* Close pipe in parent process. */
        close(pipefd[0]);
        close(pipefd[1]);

        /* Wair for both children to finish. */
        wait(NULL);
        wait(NULL);
    }
}
