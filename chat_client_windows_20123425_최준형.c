/*
 * chat_client_win.c
 * VidualStudio에서 console프로그램으로 작성시 multithread-DLL로 설정하고
 *  소켓라이브러리(ws2_32.lib)를 추가할 것.
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>


#define BUFSIZE 100
#define NAMESIZE 20

DWORD WINAPI SendMSG(void *arg);
DWORD WINAPI RecvMSG(void *arg);
void ErrorHandling(char *message);

char name[NAMESIZE]="[Default]";
char message[BUFSIZE];
char Tname[NAMESIZE];
char Joinname[20];

int main(int argc, char **argv)
{
  WSADATA wsaData;
  SOCKET sock;
  SOCKADDR_IN servAddr;
  
  HANDLE hThread1, hThread2;
  DWORD dwThreadID1, dwThreadID2;

  if(argc!=3){ // 매개변수
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }
  printf("Command : @@join @@member @@out @@talk [username] [MSG]\n"); //커맨드 종류
  printf("@@join ");												//Join 강제입력
  scanf("%s", Tname);												//이름 입력

  sprintf(name, "[%s]", Tname);
  sprintf(Joinname, "@@join [] %s", name); //join커맨드 강제 전송

  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */ 
	  ErrorHandling("WSAStartup() error!");

  sock=socket(PF_INET, SOCK_STREAM, 0); // 소켓 초기화
  if(sock == INVALID_SOCKET)
    ErrorHandling("socket() error");

  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family=AF_INET;
  servAddr.sin_addr.s_addr=inet_addr(argv[1]); //서버 주소
  servAddr.sin_port=htons(atoi(argv[2])); // 서버 포트
  
  if(connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr))==SOCKET_ERROR)
	  ErrorHandling("connect() error");
  
  
  hThread1 = (HANDLE)_beginthreadex(NULL, 0, SendMSG, (void*)sock, 0, (unsigned *)&dwThreadID1); //보낼때용 ㅡ레드
  hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvMSG, (void*)sock, 0, (unsigned *)&dwThreadID2); //수신할때용 쓰레드
  if(hThread1==0 || hThread2==0) {
	  ErrorHandling("쓰레드 생성 오류");
  }
  
  WaitForSingleObject(hThread1, INFINITE);
  WaitForSingleObject(hThread2, INFINITE);
 
  closesocket(sock);
  return 0;
}


DWORD WINAPI SendMSG(void *arg) // 메시지 전송 쓰레드 실행 함수
{

	int i,j;
	SOCKET sock = (SOCKET)arg;
   
	char nameMessage[NAMESIZE+BUFSIZE];
	send(sock, Joinname, strlen(nameMessage), 0); //처음 접속하면 join커맨드 강제전송

   while(1) {

	   bool isTrue = true;
      fgets(message, BUFSIZE, stdin);
      sprintf(nameMessage,"%s %s", name, message);
      if(!strcmp(message,"@@out\n")) {  // '@@out' 입력시 종료
		 send(sock, nameMessage, strlen(nameMessage), 0);
         closesocket(sock);
		 exit(1);
      }
	  else if (strstr(message, "@@join")) { //@@Join커맨드를 통해 이름 변경
		  i = 0;
		  while (message[i] != '[')
			  i++;
		  j = 0;
		  i++;
		  while (message[i] != ']') {
			  Tname[j++] = message[i++];
		  }
		  Tname[j] = '\0';
		  sprintf(name, "[%s]", Tname);
	  }
	  else if (strstr(message, "@@member") || strstr(message, "@@talk")) {}
	  else if (strstr(message, "@@")) {
		  printf("잘못된 명령어 입니다\n");
		  isTrue = false;
	  }
	  if(isTrue)
		send(sock, nameMessage, strlen(nameMessage), 0);
   }
}


DWORD WINAPI RecvMSG(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
  SOCKET sock = (SOCKET)arg;
  char nameMessage[NAMESIZE+BUFSIZE];
  int strLen;
  while(1){
    strLen = recv(sock, nameMessage, NAMESIZE+BUFSIZE-1, 0);
	if(strLen==-1) return 1;
	
    nameMessage[strLen]=0;
    fputs(nameMessage, stdout);
  }
}

void ErrorHandling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
