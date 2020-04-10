/*
 * Программа принимает на вход файл, который содержит нужную команду.
 * В файле может быть команда с любым числом параметров.
 * Сигналы:
 * 	SIGINT - выполнить команду из файла и записать результат в 'out.txt'.
 * 	SIGTERM - закончить работу демона.
 * 
 * Столкнулся с проблемой передачи аргументов мейна в обработчик сигнала.
 * Решил ее, сделав массив строк с этими аргументам глобальным.
 * Сомневаюсь в корректности этого решения, но в данной задаче это 
 * не критично на мой взгяд.
 * 
 * gcc daemon.c -o && daemon && ./daemon && kill -SIGINT $(pidof daemon) && kill -SIGTERM $(pidof daemon) && cat out.txt
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
char** argv;
char** split(char* buf, char ch);

void sigterm_handler(int signum){
	char logText[] = "Daemon caught SIGTERM and dead\n";
	int fd = open("log.txt", O_CREAT|O_RDWR, S_IRWXU);
	lseek(fd, 0, SEEK_END);
	write(fd, logText, sizeof(logText));
	
	syslog(LOG_NOTICE, "Daemon caught SIGTERM and dead");
	
	exit(0);
}

void sigint_handler(int signum)
{		
	char logText[] = "Daemon caught SIGINT\n";						// Запись о перехвате в файл log.txt
	int fd = open("log.txt", O_CREAT|O_RDWR, S_IRWXU);  			//	
	lseek(fd, 0, SEEK_END);											//
	write(fd, logText, sizeof(logText));							//
	
	
	syslog(LOG_NOTICE, "Daemon caught SIGINT");						// Запись о перехвате в syslog.
	
	pid_t p;
	if((p = fork()) == 0 ){											// Форкаю, чтобы демон не умирал после вызова execv
		char buf[256] = "";
		
		int fd_read = open(argv[1], O_CREAT|O_RDWR, S_IRWXU);      
		int count = read(fd_read, buf, sizeof(buf));
		
		buf[count-1] = '\0';										// Буффер с командой
		
		char** res = split(buf, ' ');								// Разбиваю строку из входного файла на множество строк, если есть дополнительные параметры
		
		int fd_write = open("out.txt", O_CREAT|O_RDWR, S_IRWXU);	

		close(1);
		dup2(fd_write, 1);											//Переопределяем дескриптор на вывод в файл
		execv(res[0], res);
	}
	
}
int Daemon(void){
	openlog("MyDaemon", LOG_PID, LOG_DAEMON);
	while(1){
		signal(SIGTERM, sigterm_handler);							// Перехват -SIGTERM - закончить выполнение демона
		signal(SIGINT, sigint_handler);								// Перехват -SIGINT - вызвать команду из входного файла. Результат в out.txt
		pause();
	}
}

int main(int argc, char* argv1[])
{
	argv = argv1;
	pid_t pid;
	if(pid = fork() == 0)  
	{
		setsid();
		Daemon();
	}
	else if(pid < 0){
		printf("Error!");
		exit(1);
	}

	return 0;
}
char** split(char* buf, char ch) {
	char* tmp = (char*)malloc(sizeof(char) * 100);
	char** array;
	int n = strlen(buf);
	int N = 0;
	int count = 0;
	int j = 0;
	for (int i = 0; i <= n; i++) {
		count++;
		if (buf[i] == '\0' || buf[i] == ch) {
			{
				for (int k = 0; k < count-1; k++) {
					tmp[k] = buf[k + j];
				}
				tmp[count-1] = '\0';
				j = i + 1;
				
				array[N] = (char*)malloc(sizeof(tmp));
				for(int q = 0; q <= sizeof(tmp); q++)
					array[N][q] = tmp[q];
				
				count = 0;
				for(int q = 0; q <= sizeof(tmp); q++)
					tmp[q] = '\0';
			}
			(N)++;
		}
	}
	array[N] = NULL;
	return array;
}