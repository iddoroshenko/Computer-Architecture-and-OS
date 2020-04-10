/*
 * Программа принимает на вход файл, который содержит нужную команду.
 * В файле может быть команда с любым числом параметров.
 * Сигналы:
 * 	SIGINT - выполнить команду из файла и записать результат в 'out.txt'.
 * 	SIGTERM - закончить работу демона.
 * 
 * 
 * gcc daemon.c -o daemon && ./daemon cmd.txt && kill -SIGINT $(pidof daemon) && kill -SIGTERM $(pidof daemon)
 * cat out.txt
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

int sigterm_h = 0;
int sigint_h = 0;

void sigterm_handler(int signum){
    sigterm_h = 1;
}

void sigint_handler(int signum)
{
    sigint_h = 1;
}
int Daemon(char** argv){
	openlog("MyDaemon", LOG_PID, LOG_DAEMON);
    signal(SIGTERM, sigterm_handler);							// Перехват -SIGTERM - закончить выполнение демона
    signal(SIGINT, sigint_handler);								// Перехват -SIGINT - вызвать команду из входного файла. Результат в out.txt
	char logText[] = "Daemon caught SIGINT\n";
	char buf[256] = "";

	int fd_read = open(argv[1], O_CREAT|O_RDWR, S_IRWXU);
	int count = read(fd_read, buf, sizeof(buf));
	close(fd_read);
	buf[count-1] = '\0';										

	char** res;
	char* tmp;
	int i = 0;
	count = 0;
	while(tmp = strtok(buf + i,' ')){
		res[count++] = tmp;
		i += sizeof(tmp);
	}
    while(1) {
        if(sigint_h) {
            int fd = open("log.txt", O_CREAT|O_RDWR, S_IRWXU);  			
            lseek(fd, 0, SEEK_END);											
            write(fd, logText, sizeof(logText));							
            close(fd);
            syslog(LOG_NOTICE, "Daemon caught SIGINT");						

            pid_t p;
            if((p = fork()) == 0 ){											// Форкаю, чтобы демон не умирал после вызова execv

                int fd_write = open("out.txt", O_CREAT|O_RDWR, S_IRWXU);

                close(1);
                dup2(fd_write, 1);											//Переопределяем дескриптор на вывод в файл
                execv(res[0], res);
            }

            sigint_h = 0;
        }
        if(sigterm_h) {

            char logText[] = "Daemon caught SIGTERM and dead\n";
            int fd = open("log.txt", O_CREAT|O_RDWR, S_IRWXU);
            lseek(fd, 0, SEEK_END);
            write(fd, logText, sizeof(logText));

            syslog(LOG_NOTICE, "Daemon caught SIGTERM and dead\n");
			close(fd);
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