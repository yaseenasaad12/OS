#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <string.h>
#include <sched.h>
#include <bits/waitflags.h>
#include <sys/wait.h>
#include <signal.h>

typedef enum {
    built_in_command, normal_command, exit_command
}command_type;

FILE *file_logger;


char *variables[100];
char *values[100];
int var_count = 0; 

void setup_environment()
  {
    //chdir(getenv("HOME"));
    chdir("/home/yaseen");
  }

  void write_to_log_file(pid_t pid, int status) 
  {
    file_logger = fopen("/home/yaseen/Desktop/Shell/log.txt", "a");
    if (file_logger != NULL) {
        fprintf(file_logger, "Child process %d %s with status code %d\n", pid,
            WIFEXITED(status) ? "exited" : WIFSIGNALED(status) ? "was terminated by signal" : "was stopped by signal",
            WEXITSTATUS(status));
        fflush(file_logger);
        fclose(file_logger);
    }
}

void reap_child_zombie() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        write_to_log_file(pid, status);
    }
}

void on_child_exit () {
    reap_child_zombie();
}

void handle_cd(char *command[]){

    if (command[1] == NULL || command[1] == "~") 
    {
        setup_environment();
    }
     else if (strcmp(command[1], "..") ==0 )  
    {
        chdir("..");
    }
    else if (command[1][0] == '/')
    {
        //absolute path
       chdir(command[1]);
    }
    else 
    {   
        //relative path
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, "/");
        strcat(cwd, command[1]);
        chdir(cwd);
    }
    /*if (chdir(command[1]) != 0)
            perror("Failed to execute command");*/
} 

void handle_export(char *command[]) {
    if (command[1] == NULL) {
        printf("export: missing argument\n");
        return;
    }

    char *assign_expr = command[1];  //string
    char *equal_sign = strchr(assign_expr, '=');
    size_t name_len = equal_sign - assign_expr;
    char *name = strndup(assign_expr, name_len);
    char *value = strdup(equal_sign+1);
    
    /*size_t len = strlen(assign_expr);
    char *value ;
    char name ;*/
    
    //handle full string inside double quotations
    if (value[0] == '"' ) {
        value[strlen(value) - 1] = '\0';  // Remove closing quote
        char *temp_value = strdup(value + 1);  // Create a new copy without modifying original
        free(value);
        value = temp_value;  // Update the value pointer
    }
    
    
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i], name) == 0) {
            free(values[i]);  // Free old value
            values[i] = strdup(value);
            free(name);
            free(value);
            return;
        }
    }
  

    if (var_count < 100) {
        variables[var_count] = strdup(name);
        values[var_count] = strdup(value);
        var_count++;
        /*printf("name: %s\n", variables[var_count-1]);
        printf("value: %s\n", values[var_count-1]);*/
    } else {
        printf("Max variable limit reached.\n");
    }
    } 
    

void handle_echo(char *command[]) {
        
        char *assign_expr = command[1];
    
      /*  if (assign_expr[0] != '"' ) {
            printf("echo: Input must be within double quotes.\n");
            return;
        }
       
           assign_expr++;  // Skip opening quote
           assign_expr[strlen(assign_expr) - 1] = '\0';  // Remove closing quote
        
    */
        while (*assign_expr != '\0') {
            if (*assign_expr == '$') {
                assign_expr++;  // Skip '$'
                
                // Extract full variable name
                char var_name[100];
                int i = 0;
                while (*assign_expr != ' ' && *assign_expr != '\0' && *assign_expr != '"') {
                    var_name[i++] = *assign_expr;
                    assign_expr++;
                }
                var_name[i] = '\0';
                //printf(".......................");
                
    
                // Search for the variable and print its value
                int found = 0;
                for (int j = 0; j < var_count; j++) {
                    if (strcmp(variables[j], var_name) == 0) {
                        printf("%s", values[j]);
                        found = 1;
                        break;
                    }
                }
            } else {
                printf("%c", *assign_expr);
                assign_expr++;
            }
        }
        printf("\n");
    }

void execute_shell_bultin(char *command[]) {
        char *assign_expr = command[0];
        if (strcmp(assign_expr, "cd") == 0) {
            handle_cd(command);
        }
        else if (strcmp(assign_expr, "echo") == 0) {
            handle_echo(command);
        }
        else if (strcmp(assign_expr, "export") == 0) {
            handle_export(command);
        }
    }    
char* get_variable_value(char *var_name) {
        for (int i = 0; i < var_count; i++) {
            if (strcmp(variables[i], var_name) == 0) {
                return values[i];
            }
        }
        return "";  
    }
void replace_variables(char input[]) {
        char input2[200];  // Buffer to store modified string
        int i = 0, j = 0;

        char var_name[100];  // Buffer for variable name
        char *var_value;
        int flag = 0;
    
        while (input[i] != '\0') {
            if (input[i] == '$') {
                flag = 1;
                i++; // Move past '$'
                int k = 0;
                while (input[i] != ' ' && input[i] != '\0') 
                 var_name[k++] = input[i++];
            
                var_name[k] = '\0';
                
    
                // Get the variable's value
                var_value = get_variable_value(var_name);
    
                // Copy the variable value into input2
                for (int m = 0; var_value[m] != '\0'; m++) {
                    input2[j++] = var_value[m];
                }
                i++;
            } else {
                // Copy normal characters
                input2[j++] = input[i++];
            }
        }
    
        input2[j] = '\0';  // Null-terminate final result
        strcpy(input, input2);  // Copy back to original input
    }

void execute_command(char *command[], int background) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Forking failed");
            exit(1);
        } else if (pid == 0) { // Child process
            if (background) {
                printf("[Background Process] PID: %d\n", getpid());
            }
            execvp(command[0], command);
            printf("%s: command not found\n", command[0]);
            exit(1);
        } else { // Parent process
            if (background) {
                printf("[Shell] Process %d running in background\n", pid);
                waitpid(pid, NULL, WNOHANG); // Do not wait for background process
            } else {
                waitpid(pid, NULL, 0); // Wait for foreground process
            }
        }
    }
    

void shell(){
    int exit = 0 ;
    command_type cmd_type;
    
    do{ 
        int background = 0;

        char input[200];
        scanf("%[^\n]", input); //allow spaces
        getchar();
        
        if(input[0] == 'e' && input[1] == 'c'){}
        else
         replace_variables(input);
        background = 1;

        if (input[strlen(input) - 1] == '&') {
            input[strlen(input) - 1] = '\0'; 
        }


        char *command[100]; 
        int i = 0;

        char *token = strtok(input, " ");
        if (token == NULL) return;
        
        command[i++] = token;  // Store command name
    
        if (strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0 || strcmp(command[0], "export") == 0) {
            token = strtok(NULL, "");  // Get the remaining string as is
            if (token != NULL) {
                command[i++] = token;  // Store it as a whole
            }
        } else {
            // Otherwise, tokenize normally
            while ((token = strtok(NULL, " ")) != NULL) {
                command[i++] = token;
            }
        }
    
        command[i] = NULL; 
        char *assign_expr = command[0];
        
        
        if (strcmp(command[0], "exit") == 0) {
            exit = 1;
            cmd_type = exit_command;
        }
        else if (strcmp(command[0], "cd") == 0 
                || strcmp(command[0], "echo") == 0 
                || strcmp(command[0], "export") == 0) {
            cmd_type = built_in_command;
        }
        else {
            cmd_type = normal_command;
        }

        switch (cmd_type) {
            case built_in_command:
                execute_shell_bultin(command);
                break;
            case normal_command:
                execute_command(command, background);
                break;
            case exit_command:
                break;
        }
            
     } 
    while(!exit);


}

int main()
{
 signal(SIGCHLD, on_child_exit);
 setup_environment();
 shell() ;  
}