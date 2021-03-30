/*****************************************************************************
Matteo Ciardullo - S4695637
Niccolò Parodi - S4668271
Emanuele Prella - S4636470

uBash
******************************************************************************/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>

#define _GNU_SOURCE
# define MAXCOM 1024 // max number of letters to be supported
# define MAXLIST 100 // max number of commands to be supported

// clearing the shell with "clear" command
# define clear() printf("\033[H\033[J")

//strsep function
char * strsep(char ** stringp, const char * delim) {
  char * begin, * end;
  begin = * stringp;
  if (begin == NULL)
    return NULL;
  end = begin + strcspn(begin, delim);
  if ( * end) {
    * end++ = '\0'; //EOF
    * stringp = end;
  } else
    *stringp = NULL;
  return begin;
}

// Function to take input
char * takeInput() {
  int position = 0;
  int position_s = 0;

  char * buf = malloc(sizeof(char) * MAXCOM);
  int c;
  while (1) {
    // Read a character
    c = getchar();
    if (c == '$') {
      char * dollar = malloc(sizeof(char) * MAXCOM);
      char * value;

      position_s = 0;
      while (1) {
        if (((c = getchar()) == ' ') || (c == '\n') || (c == EOF)) {
          break;
        }
        dollar[position_s] = c;
        position_s++;
      }
      value = getenv(dollar);
      sprintf(buf, "%s %s", buf, value);
      position = strlen(buf);

      if (buf[position] == '\0') {
        free(dollar);
        return buf;
      }
    }

    // when EOF, replace with null and return
    else if (c == EOF) {
      printf("\n");
      exit(EXIT_SUCCESS);
    }
    //if \n is found return
    else if (c == '\n') {
      buf[position] = '\0';
      return buf;
    } else {
      buf[position] = c;
    }
    position++;
  }
}

// function for parsing
void parseSpace(char * str, char ** parsed) {
  int i;

  for (i = 0; i < MAXLIST; i++) {
    parsed[i] = strsep( & str, " ");
    if (parsed[i] == NULL)
      break;
    if (strlen(parsed[i]) == 0)
      i--;
  }
}

//function for chdir (built-in command)
int cd(char ** parsed) {
  if (strcmp(parsed[0], "cd")) {
    return 0;
  } else if (parsed[1] == NULL) {
    fprintf(stderr, "uBash: expected argument to \"cd\"\n");
  } else if (parsed[2] != NULL) {
    fprintf(stderr, "uBash: too many arguments to \"cd\"\n");
  }
  chdir(parsed[1]);
  return 1;
}
// function in order to execute command
int execArgsPiped(char ** strpiped, char ** parsed, int piped, char * filein, char * fileout, int i) {

  if (piped == 0) { //single command cases

    parseSpace(strpiped[0], parsed);

    if (cd(parsed))
      return 0; // cd
    else { // not cd

      pid_t pid = fork();
      if (pid == -1) {
        printf("\nFailed forking child..");
        exit(EXIT_FAILURE);
      } else if (pid == 0) {

        if (i == 1) { //input redirection
          int fw = open(filein, O_RDONLY, 0);
          //checks for open failure and cleans up resources / prints errors if so
          if (fw < 0) {
            printf("Failed to open %s.\n", filein);
          }

          dup2(fw, 0);
          close(fw);

        }
        if (i == 2) { //output redirection
          int fw = open(fileout, O_CREAT | O_TRUNC | O_WRONLY, 0644);
          //checks for open failure and cleans up resources / prints errors if so
          if (fw < 0) {
            printf("Failed to open %s.\n", fileout);
          }

          dup2(fw, 1); //use 1 as it is the integer assigned to stdout
          close(fw);

        }
        if (execvp(parsed[0], parsed) < 0) {
          printf("%s: command not found\n", parsed[0]);
        }
        exit(0);
      } else { // waiting for child to terminate
        wait(NULL);
        return 0;
      }
    }
  } else if (piped > 0) { //multi-pipe
    int p[2];
    pid_t pid;
    int fd_in = 0;
    while ( * strpiped != NULL) {
      parseSpace(strpiped[0], parsed);
      if (parsed[0] != NULL) {
        pipe(p);
        if ((pid = fork()) == -1) {
          exit(EXIT_FAILURE);
        } else if (pid == 0) {
          if (i == 1) { //input redirection
            int fw = open(filein, O_RDONLY, 0);
            //checks for open failure and cleans up resources / prints errors if so
            if (fw < 0) {
              printf("Failed to open %s.\n", filein);
              perror("Error: ");
            }

            dup2(fw, 0);
            close(fw);

          }
          if (i == 2) { //output redirection
            int fw = open(fileout, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            //checks for open failure and cleans up resources / prints errors if so
            if (fw < 0) {
              printf("Failed to open %s.\n", fileout);
              perror("Error: ");
            }

            dup2(fw, 1); //use 1 as it is the integer assigned to stdout
            close(fw);
          }

          if (i == 3) { // I/O redirection
            int fw1 = open(filein, O_RDONLY, 0);
            //checks for open failure and cleans up resources / prints errors if so
            if (fw1 < 0) {
              printf("Failed to open %s.\n", filein);
              perror("Error: ");
            }

            dup2(fw1, 0);
            close(fw1);
            int fw2 = open(fileout, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            //checks for open failure and cleans up resources / prints errors if so
            if (fw2 < 0) {
              printf("Failed to open %s.\n", fileout);
              perror("Error: ");
            }

            dup2(fw2, 1); //use 1 as it is the integer assigned to stdout
            close(fw2);

          }

          dup2(fd_in, 0); //change the input according to the old one
          if ( * (strpiped + 1) != NULL)
            dup2(p[1], 1);
          close(p[0]);
          if (execvp(parsed[0], parsed) < 0) {
            printf("%s: command not found\n", parsed[0]);
          }
          exit(0);

        } else {
          wait(NULL);
          close(p[1]);
          fd_in = p[0]; //save input for the next command
          strpiped++;
        }
      } else {
        return -1;
      }
    }
    fflush(stdout);
  }
  return 0;
}

/*******************************************************************************
                              MAIN
*******************************************************************************/
int main() {
  char * inputString;
  char cwd[MAXCOM];
  char * filein;
  char * fileout;
  char * input;
  int piped = 0;
  char * strpiped[10];
  char * aux;
  char * input2;
  char ** parsedArgs = malloc(sizeof(char) * MAXLIST);

  while (1) {
    filein = "\0";
    fileout = "\0";
    // print shell line
    getcwd(cwd, sizeof(cwd));
    printf("%s$: ", cwd);
    int i = 0;
    //input
    inputString = takeInput();
    //searching for I/O redirections
    if (strchr(inputString, '<') && (strchr(inputString, '>'))) {

      input = strsep( & inputString, "<");
      filein = strsep( & inputString, " ");
      input2 = strsep( & inputString, ">");
      fileout = strsep( & inputString, " ");
      i = 3;
    } else if (strchr(inputString, '<')) {
      input = strsep( & inputString, "<");
      filein = strsep( & inputString, " ");
      i = 1;
    } else if (strchr(inputString, '>')) {
      input = strsep( & inputString, ">");
      fileout = strsep( & inputString, " ");
      i = 2;
    } else {
      input = inputString;
    }

    if (strcmp(input, "\0") != 0) {
      // process

      int j = 0;
      if (i == 0 || i == 2) { //0= no redirection, 2=output redirection
        while (1) {
          strpiped[j] = strsep( & input, "|");
          if (strpiped[j] == NULL) {
            piped = j - 1;
            break;
          }
          j++;
        }
      }

      if (i == 1) { //input redirection
        strpiped[0] = input;
        j = 1;
        aux = strsep( & inputString, "|");
        while (1) {
          strpiped[j] = strsep( & inputString, "|");
          if (strpiped[1] == NULL) {
            piped = j;
            break;
          }
          if (strpiped[j] == NULL) {
            piped = j - 1;
            break;
          }
          j++;
        }
      }

      if (i == 3) { //I/O redirection
        strpiped[0] = input;
        j = 1;
        aux = strsep( & input2, "|");
        while (1) {
          if (input2 == NULL) {
            piped = j;
            break;
          }
          strpiped[j] = strsep( & input2, "|");

          if (strpiped[j] == NULL) {
            piped = j - 1;
            break;
          }
          j++;
        }

      }

      //execute and check if there is any empty command with pipe
      if (execArgsPiped(strpiped, parsedArgs, piped, filein, fileout, i) == -1) {
        printf("ubash: empty command within pipe \n");
      }

    }
    free(parsedArgs);
    free(inputString);
  }

  return 0;

}
