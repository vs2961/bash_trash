#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h> 

static void sighandler(int signo)
{
    if (signo == SIGINT)
    {
        printf("\n");
    }
}

char **parse_args(char *line, char *delim)
{
    //Using *line as the backing bytes, this function creates an array of tokens
    //*line can not change, or else this will break
    //*line must have at least one argument

    int arg_count = 1;
    {
        int i = 0;
        for(i = 0; i < strlen(line); i++){
            if(line[i] == *delim) arg_count++; 
        }
    }    

    char *token;
    char *p = line;
    int i = 0;
    char **args = malloc((arg_count + 1) * sizeof(char *)); //Plus one for the NULL terminator 
    args[arg_count] = NULL;
    
    while (p)
    {
        token = strsep(&p, delim);
        args[i] = token;
        i++;
    }
    if(arg_count == 2) printf("a0: %s, a1: %s, a2: %s\n", args[0], args[1], args[2]);
    //if(arg_count == 1) printf("a0: %s, a1: %s\n", args[0], args[1]);
    return args;
}

void redirect(char **args){
    int i = 0;
    while(args[i] != NULL){ // ls > hello.txt
        if(strcmp(args[i],">") == 0) { //Change out
            int file_desc = open(args[i+1], O_TRUNC | O_RDWR | O_CREAT, 0644); 
            dup2(file_desc, 1);
            args[i] = NULL;
            return;
        }

        if(strcmp(args[i],">>") == 0) { //Change out but append instead of overwriting
            int file_desc = open(args[i+1], O_RDWR | O_CREAT | O_APPEND, 0644); 
            dup2(file_desc, 1);
            args[i] = NULL;
            return;
        }
        
        if(strcmp(args[i],"<") == 0) { //Change stdin
            int file_desc = open(args[i+1], O_RDONLY, 0644); 
            dup2(file_desc, 0);
            args[i] = NULL;
            return;
        }
        i++;
    }
}

void run_child(char *command)
{
    char **args = parse_args(command, " ");
    if (!strcmp(args[0], "exit"))
    {
        exit(0);
    }
    else if (!strcmp(args[0], "cd"))
    {
        chdir(args[1]);
    }
    else
    {
        int f = fork();
        if (f)
        { //The parent
            int status;
            wait(&status);
        }
        else
        { //The child
            if(strcmp(args[0], "") == 0) {
                printf("WARNING: One of your arguments was blank!\n");
                free(args);
                return;
            } 
            redirect(args);
            int check_error = execvp(args[0], args);
            if (check_error == -1) {
                printf("Error with argument %s: %s\n", args[0],strerror(errno));
            }
        }
        free(args);
    }
}



char *get_rid_of_spaces(char *buffer, int max_size){
    char *newBuffer = malloc(max_size * sizeof(char));          
    int i = 0;
    int pos = 0;
    bool first_met = false;
    bool second_met = false;
    for(i = 0; i < max_size; i++){
        //Im not a space
        if(buffer[i] == ';') first_met = second_met = false;
        if(buffer[i] != ' '){
            if(first_met && second_met) {
                newBuffer[pos++] = ' ';
                first_met = second_met = false;
            }
            newBuffer[pos++] = buffer[i];
            continue;
        }
        //Im a space
        if( (i+1) < max_size && buffer[i+1] != ' ' && buffer[i+1] != ';' && buffer[i+1] != '\0') second_met = true;
        if( (i-1) >= 0 && buffer[i-1] != ' ' && buffer[i-1] != ';' && buffer[i-1] != '\0') first_met = true;
        
    }
    return newBuffer;
}

int main()
{
    char buffer[256];
    const int max_len = 256;
    while (1)
    {   
        //Print bash_trash:[current directory]
        char curr_dir[256];
        getcwd(curr_dir, 256);
        printf("\nbash_trash:%s$ ", curr_dir);
        
        //Read stdin for user arguments and get rid of spaces.
        fgets(buffer, max_len, stdin); 

        if(buffer[0] == '\n') continue;
        if (buffer != NULL)
        {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
            {
                buffer[--len] = '\0';
            }
        }

        char *no_space_buffer = get_rid_of_spaces(buffer, max_len); //Must be freed, but do not free until end of program!
        printf("No Space: %s\n", no_space_buffer);

        //Count commands
        int arg_count = 1;
        {
            int i = 0;
            for(i = 0; i < strlen(no_space_buffer); i++){
                if(no_space_buffer[i] == ';') {
                    arg_count++;
                }
            }
        }

        char **args = parse_args(no_space_buffer, ";"); //**args depends on no_space_buffer to work
        
        //Execute commands
        int i = 0;
        for (i = 0; i < arg_count; i++)
        {
            run_child(args[i]);
        }

        free(args);
        free(no_space_buffer);
    }
}