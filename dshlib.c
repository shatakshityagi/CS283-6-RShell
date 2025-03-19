#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */




// Allocate memory for a command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;

    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;

    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }

    cmd_buff->argc = 0;
    return OK;
}

// Free memory allocated for a command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;

    free(cmd_buff->_cmd_buffer);
    cmd_buff->_cmd_buffer = NULL;

    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;

    return OK;
}

// Clear a command buffer for reuse
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff || !cmd_buff->_cmd_buffer) return ERR_MEMORY;

    cmd_buff->_cmd_buffer[0] = '\0';

    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;

    return OK;
}

// Parse command input and store into `cmd_buff_t`
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff || !cmd_buff->_cmd_buffer) return ERR_MEMORY;

    clear_cmd_buff(cmd_buff);

    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';

    char *curr = cmd_buff->_cmd_buffer;
    while (*curr && isspace(*curr)) curr++;

    if (*curr == '\0') return WARN_NO_CMDS;

    cmd_buff->argc = 0;
    bool in_quotes = false;
    char *token_start = curr;
    char *write_pos = curr;

    while (*curr) {
        if (*curr == '"') {
            in_quotes = !in_quotes;
            curr++;
        } else if (isspace(*curr) && !in_quotes) {
            *write_pos = '\0';
            write_pos++;

            if (token_start[0] != '\0') {
                if (cmd_buff->argc >= CMD_MAX) return ERR_TOO_MANY_COMMANDS;
                cmd_buff->argv[cmd_buff->argc++] = token_start;
            }

            while (*(curr + 1) && isspace(*(curr + 1))) curr++;
            curr++;
            token_start = write_pos;
        } else {
            *write_pos = *curr;
            write_pos++;
            curr++;
        }
    }

    if (token_start < write_pos) {
        *write_pos = '\0';
        if (cmd_buff->argc >= CMD_MAX) return ERR_TOO_MANY_COMMANDS;
        cmd_buff->argv[cmd_buff->argc++] = token_start;
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

// Parse a piped command input and store it in `command_list_t`
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *token;
    char *saveptr;
    int cmd_idx = 0;

    memset(clist, 0, sizeof(command_list_t));

    if (strlen(cmd_line) == 0) return WARN_NO_CMDS;

    // Create a copy of cmd_line to avoid modifying the original
    char cmd_copy[SH_CMD_MAX];
    strncpy(cmd_copy, cmd_line, SH_CMD_MAX - 1);
    cmd_copy[SH_CMD_MAX - 1] = '\0';

    token = strtok_r(cmd_copy, PIPE_STRING, &saveptr);
    while (token) {
        if (cmd_idx >= CMD_MAX) return ERR_TOO_MANY_COMMANDS;

        while (*token == SPACE_CHAR) token++;

        if (strlen(token) == 0) return WARN_NO_CMDS;

        // Allocate command buffer
        cmd_buff_t *cmd = &clist->commands[cmd_idx];
        if (alloc_cmd_buff(cmd) != OK) return ERR_MEMORY;

        if (build_cmd_buff(token, cmd) != OK) {
            free_cmd_buff(cmd);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }

        cmd_idx++;
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    clist->num = cmd_idx;
    return OK;
}

// Match built-in commands
Built_In_Cmds match_command(const char *input) {
    if (!input) return BI_NOT_BI;

    if (strcmp(input, EXIT_CMD) == 0) return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0) return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;

    return BI_NOT_BI;
}

// Execute built-in commands
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0 || !cmd->argv[0]) return BI_NOT_BI;

    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);

    switch (cmd_type) {
        case BI_CMD_EXIT:
            printf("exiting...\n");
            exit(OK_EXIT);

        case BI_CMD_DRAGON:
            printf("🐉 Drexel Dragon!\n");
            return BI_EXECUTED;

        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

// Execute a single command
int exec_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0) return WARN_NO_CMDS;

    Built_In_Cmds result = exec_built_in_cmd(cmd);
    if (result == BI_EXECUTED) return OK;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }

    return OK;
}

// Execute a pipeline of commands
int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    int pipes[CMD_MAX - 1][2];  // Pipe file descriptors
    pid_t pids[CMD_MAX];        // Store child PIDs

    // Create pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            return ERR_EXEC_CMD;
        }
    }

    // Execute commands in a pipeline
    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {  // Child process
            // Redirect input from previous pipe
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Redirect output to next pipe
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes in child
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

// Main shell loop
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) break;

        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        if (strcmp(cmd_line, EXIT_CMD) == 0) break;

        if (build_cmd_list(cmd_line, &clist) == OK) {
            execute_pipeline(&clist);
        }
    }

    return OK;
}