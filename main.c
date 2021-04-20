//libraries
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

//defines
#define MAX_LINE   80 // The maximum length command

/* ----------------------------------- FUNCTION Declaration ----------------------------------- */

void parser(char *command, char **args, char **args_two, char **file_name, int *less_than, int *larger_than, int *command_two, int *non_waiting);

/* ----------------------------------------- MAIN ------------------------------------------ */

int main()
{
    
    
    //booleans
    int should_run = 1; //flag to determine when to run program
    int status = 0;
    int less_than = 0;
    int larger_than = 0;
    int command_two = 0;
    int non_waiting = 0;
    
    int i = 0;
    
    //strings
    char *args[MAX_LINE/2 + 1];
    char *args_two[MAX_LINE/2 + 1];
    char history [41] = {'\0'};
    char command [41];
    char *file_name;
    
    pid_t pid; //process id
    int fd[2]; //file descriptor table
    int file_desc;
    
    //initial stdout and stdin to and from terminal, used to reset later
    int initial_out = dup(STDOUT_FILENO);
    int initial_in = dup(STDIN_FILENO);
    
    //keeps running until "exit" string is entered
    while (should_run)
    {
        
        //Resetting the args for next command:
        while (args[i++] != NULL)
        {
            args[i++] = NULL;
        }
        
        //Reset values
        less_than = 0;
        larger_than = 0;
        non_waiting = 0;
        command_two = 0;
        i = 0;
        dup2(initial_out, STDOUT_FILENO);
        dup2(initial_in, STDIN_FILENO);
        
        printf("osh>");
        fflush(stdout);
        gets(command);
        
        /* ------------------------ START OF HISTORY FEAUTURE ------------------------ */
        
        // history feature allows a user to execute the most recent command by entering '!!'
        
        if (strncmp(command, "!!", 2) == 0) //if the difference between the command first 2 letters and string "!!" is 0
            
        {
            //if there is no recent command, error message
            if (history[0] == '\0')
            {
                printf("No commands in history.\n");
            }
            
            //placing the previous command into command that will be parserd
            for (int i = 0; i < 41; i++)
                command[i] = history[i];
            
        }
        //else if command is not '!!"
        else
        {
            
            //command is placed in history buffer
            for (int i = 0; i < 41; i++)
            {
                history[i] = command[i];
            }
        }
        
        
        /* ------------------------ END OF HISTORY FEATURE ------------------------ */
        
        if (history[0] != '\0') //if there is a command in the history buffer
        {
            
            parser(command, args, args_two, &file_name, &less_than, &larger_than, &command_two,
                  &non_waiting); //parser the command
            
            
            //terminate if 4 letters are "exit"
            if (strncmp(args[0], "exit", 4) == 0)
            {
                should_run = 0; //flag to run program set to zero
            }
            
            //if command is not to exit
            else {
                
                //output to given file name
                if (larger_than == 1)
                {
                    //O_WRONLY: write only, O_APPEND: all data gets written to the end, O_CREATE: create if does not exist
                    //O_TRUC: will truncate the file
                    file_desc = open(file_name, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC);
                    dup2(file_desc, STDOUT_FILENO);  // writes to terminal go to file_desc
                }
                
                //input from a given file name
                else if (less_than == 1)
                {
                    // if file of file_name does exist have in directory, create file
                    file_desc = open(file_name, O_RDONLY | O_CREAT); //O_RDONLY: read only
                    dup2(file_desc, STDIN_FILENO); // input comes from file instead of terminal 
                }
                
                
                
                
                /* --------------------------- Execution ------------------------------- */
                
                //Creating the pipe:
                if (pipe(fd) == -1) {
                    fprintf(stderr, "Pipe Error!");
                    return 1;
                }
                
                // forking the child process
                pid = fork();
                
                //incase of fork failing
                if (pid < 0) {
                    fprintf(stderr, "Fork Error!");
                    return 1;
                }
                
                //parent process
                if (pid > 0)
                {
                    
                    if (non_waiting == 0)
                    {
                        while (wait(&status) != pid);    //parent waits for child.
                        
                    }
                }
                
                //If there's no "|", progress normal operations:
                if (command_two == 0)
                {
                    // child process
                    if (pid == 0)
                    {
                        execvp(args[0], args); //the command to be performed: args[0], parameter: args
                    }
                }
                
                //If there are two commands now:
                else {
        
                    //first child process
                    if (pid == 0)
                    {
                        
                        close(STDOUT_FILENO);
                        dup(fd[1]); //dup where we will be writing
                        close(fd[0]);   //close reading
                        close(fd[1]);   //close writing
                        execvp(args[0], args); //execute
                    }
                    
                    //second child process
                    if (fork() == 0)
                    {
                        
                        close(STDIN_FILENO);
                        dup(fd[0]); //dup where we will be reading
                        close(fd[1]);   //close writing
                        close(fd[0]);   //close reading
                        execvp(args_two[0], args_two); //execute
                    }
                    
                    close(fd[0]);
                    close(fd[1]);
                    wait(0);
                    wait(0);
                }
                
            }
            
            /* -------------------------- End of Execution ----------------------------- */
            
        }
    }
    
    return 0;
}

/* ----------------------------------- parser FUNCTION ----------------------------------- */
void parser(char *command, char **args, char **args_two, char **file_name, int *less_than, int *larger_than, int *command_two, int *non_waiting) {
    
    char *token;
    char *tokenCommand;
    char *waste;
    
    tokenCommand = command;
    
    //strtok_r() is used for splitting a string into tokens by between spaces or new lines
    token = strtok_r(tokenCommand, " ", &waste);
    
    do {
        
        //Outputting:
        if (*token == '>')
        {
            *larger_than = 1;
            *file_name = strtok_r(NULL, " ", &waste);
            break;
        }
        
        //Inputting:
        else if (*token == '<')
        {
            *less_than = 1;
            *file_name = strtok_r(NULL, " ", &waste);
            break;
        }
        
        //parent and child run concurrently
        else if (*token == '&')
        {
            *non_waiting = 1;
        }
        
        //Part 5 - Two Commands:
        else if (*token == '|') {
            *command_two = 1;
            
            while((token = strtok_r(NULL," ", &waste)))
            {
                *args_two++ = token;
            }
        }        
        //if the token is not > , < , & , | , then the token is stored in args and the word count is incremented
        else {
            *args++ = token;
        }
        
    } while ((token = strtok_r(NULL, " ", &waste)));
}
