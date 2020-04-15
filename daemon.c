/*
 * Программа принимает на вход файл, который содержит нужные команды - каждая в новой строке.
 * В файле может быть команда с любым числом параметров.
 * Сигналы:
 * 	SIGINT - выполнить команды из файла и записать результат в 'out1.txt', 'out2.txt'...
 * 	SIGTERM - закончить работу демона.
 * 
 * 
 * gcc daemon.c -o daemon -lpthread && ./daemon cmd.txt && kill -SIGINT $(pidof daemon) 
 * kill -SIGTERM $(pidof daemon)
 */


#include <stdio.h> 
#include <string.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <semaphore.h>

int sigterm_h = 0;
int sigint_h = 0;
int commandsSize = 0;

void sigterm_handler(int signum){
    sigterm_h = 1;
}

void sigint_handler(int signum)
{
    sigint_h = 1;
}

void newLogRecord(const char* logPath, const char* message);
char*** split(char* buf);

int Daemon(char** argv){
	openlog("MyDaemon", LOG_PID, LOG_DAEMON);
    signal(SIGTERM, sigterm_handler);							
    signal(SIGINT, sigint_handler);			
    
	    
    char buf[256] = "";
	int fd_read = open(argv[1], O_CREAT|O_RDWR, S_IRWXU);
	int buf_size = read(fd_read, buf, sizeof(buf));
	close(fd_read);

    char*** commands = split(buf);


    const char* sem_name = "/sem";
    sem_unlink(sem_name);
    sem_t* sem = sem_open(sem_name, O_CREAT);
    sem_post(sem);
    while(1) {
        if(sigint_h) {
            newLogRecord("log.txt", "Daemon caught SIGINT\n");
            syslog(LOG_NOTICE, "Daemon caught SIGINT");	
            
            pid_t p;
            for(int i = 0; i < commandsSize; i++) {
                sem_wait(sem);
                if((p = fork()) == 0 ){	
                    char fileOut[] = "out";
                    char number[5];
                    sprintf(number, "%d", i+1);
                    strcat(fileOut, number);
                    strcat(fileOut, ".txt");
                    int fd_write = open(fileOut, O_CREAT|O_RDWR, S_IRWXU);

                    close(1);
                    dup2(fd_write, 1);

                    newLogRecord("log.txt", "make command and save result in ");
                    newLogRecord("log.txt", fileOut);
                    newLogRecord("log.txt", "\n");
                    
                    sem_post(sem);
                    execv(commands[i][0], commands[i]);
                    exit(0);
                }
            }

            sigint_h = 0;
        }

        if(sigterm_h) {
            free(commands);
            
            newLogRecord("log.txt", "Daemon caught SIGTERM and dead\n");
            syslog(LOG_NOTICE, "Daemon caught SIGTERM and dead\n");
			
            exit(0);
        }

        pause();
    }
}


int main(int argc, char* argv[])
{
	pid_t pid;
	if(pid = fork() == 0)  
	{
		setsid();
		Daemon(argv);
	}
	else if(pid < 0){
		printf("Error!");
		exit(1);
	}

	return 0;
}

void newLogRecord(const char* logPath, const char* message) {
    int fd = open(logPath, O_CREAT|O_RDWR, S_IRWXU);  			
    lseek(fd, 0, SEEK_END);											
    write(fd, message, strlen(message));							
    close(fd);
}

char*** split(char* buf) {
    char** lines = (char**)malloc(sizeof(char) * strlen(buf));
    
    char delim = '\n';
    char* tmp = strtok(buf, &delim); 
	do {
		lines[commandsSize] = tmp;
        lines[commandsSize++][strlen(tmp)] = '\0';
	} while(tmp = strtok(NULL, &delim));

    char*** commands = (char***)malloc(sizeof(char) * strlen(buf));
    
    delim = ' ';
    for(int i = 0; i < commandsSize; i++) {
        
        char** line = (char**)malloc(sizeof(char) * 50);
        tmp = strtok(lines[i], &delim);
        int count = 0; 
        do {
            line[count] = tmp;
            line[count++][strlen(tmp)] = '\0';
        } while(tmp = strtok(NULL, &delim));
        commands[i] = line;
    }
    free(lines);
    return commands;
}