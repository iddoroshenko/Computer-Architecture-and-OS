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

void sigterm_handler(int signum){
    sigterm_h = 1;
}

void sigint_handler(int signum)
{
    sigint_h = 1;
}

void newLogRecord(const char* logPath, const char* message);


int Daemon(char** argv){
	openlog("MyDaemon", LOG_PID, LOG_DAEMON);
    signal(SIGTERM, sigterm_handler);							
    signal(SIGINT, sigint_handler);	

    const char* sem_name = "/sem";
    sem_unlink(sem_name);
    sem_t* sem = sem_open(sem_name, O_CREAT);
    sem_post(sem);   
	
    while(1) {
        if(sigint_h) {
            char buf[256] = "";
            int fd_read = open(argv[1], O_CREAT|O_RDWR, S_IRWXU);
            int buf_size = read(fd_read, buf, sizeof(buf));
            close(fd_read);

            newLogRecord("log.txt", "Daemon caught SIGINT\n");
            syslog(LOG_NOTICE, "Daemon caught SIGINT");	
            
            int fd_write = open("out.txt", O_CREAT|O_RDWR, S_IRWXU);
            lseek(fd_write, 0, SEEK_END);
            close(1);
            dup2(fd_write, 1);
            
            pid_t p;
            char* delim = "\n";
            int commandsSize = 0;
            char* tmp = strtok(buf, delim);
            char* commands[256];
            do{
                commands[commandsSize] = tmp;
                commands[commandsSize++][strlen(tmp)] = '\0';
            
            } while(tmp = strtok(NULL, delim));
            commands[commandsSize] = NULL;
            for(int i = 0; i < commandsSize; i++) {

                char cmd_copy[256];
                strcpy(cmd_copy, commands[i]);

                char* lines[256];
                int count = 0;
                char* delim2 = " ";
                char* tmp2 = strtok(commands[i], delim2);
                do {
                    lines[count] = tmp2;
                    lines[count++][strlen(tmp2)] = '\0';
                } while(tmp2 = strtok(NULL, delim2));
                lines[count] = NULL;

                if((p = fork()) == 0 ){
                    sem_wait(sem);	
                    newLogRecord("log.txt", "make command '");
                    newLogRecord("log.txt", cmd_copy);
                    newLogRecord("log.txt", "' and save result in out.txt\n");
                    
                    sem_post(sem);
                    execv(lines[0], lines);
                    exit(0);
                }
                
            }
            sigint_h = 0;
        }

        if(sigterm_h) {
            sem_wait(sem);
            newLogRecord("log.txt", "Daemon caught SIGTERM and dead\n");
            syslog(LOG_NOTICE, "Daemon caught SIGTERM and dead\n");
			sem_post(sem);
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
