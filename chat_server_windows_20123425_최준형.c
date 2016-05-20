/*
 * chat_server_win.c
 * VidualStudio에서 console프로그램으로 작성시 multithread-DLL로 설정하고
 *  소켓라이브러리(ws2_32.lib)를 추가할 것.
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS //error ㄴㄴ
#define _CRT_SECURE_NO_WARNINGS			//error ㄴㄴ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>					//multiThread

#define BUFSIZE 100

void p2pMSG(char* name);
void SendLIST(SOCKET sock);
void addUser(char* name, int len);
DWORD WINAPI ClientConn(void *arg);		
void SendMSG(char* message, int len);
void ErrorHandling(char *message);

int clntNumber=0;
SOCKET clntSocks[10]; //클라이언트 갯수
char userList[5][30];
HANDLE hMutex;

int main(int argc, char **argv)
{
  WSADATA wsaData;
  SOCKET servSock;
  SOCKET clntSock;

  SOCKADDR_IN servAddr;
  SOCKADDR_IN clntAddr;
  int clntAddrSize;

  HANDLE hThread;
  DWORD dwThreadID;

  if(argc!=2){  //매개벼수 error 체크
    printf("Usage : %s <port>\n", argv[0]);
    exit(1);
  }
  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */ //winsocker 2.0ver 초기화
	  ErrorHandling("WSAStartup() error!");

  hMutex=CreateMutex(NULL, FALSE, NULL); //mutex변수 생성, 초기화, log데이터 보호
  if(hMutex==NULL){
	  ErrorHandling("CreateMutex() error");
  }

  servSock=socket(PF_INET, SOCK_STREAM, 0);   //소켓함수 internet외에서도 쓰겠다, tcp프로토콜 사용하겠다 
  if(servSock == INVALID_SOCKET)				//error 검출 (-값)
    ErrorHandling("socket() error");
 
  memset(&servAddr, 0, sizeof(servAddr)); //인터넷 주소구조체 변수 초기화
  servAddr.sin_family=AF_INET;			//인터넷 주소구조체 변수 초기화 소켓을 인터넷용도로 쓰겠다
  servAddr.sin_addr.s_addr=htonl(INADDR_ANY); //인터넷 주소구조체 변수 초기화 인터넷 주소 구조체 여기서는 주소 상관없음
  servAddr.sin_port=htons(atoi(argv[1])); //인터넷 주소구조체 변수 초기화 포트 넘버
  //atoi -> 문자열을 숫자값으로 변환 htons -> Host to network in short portnum = 2byte의 short형  bigendian, littleendian의 표준화 - Big Endian으로 
  // ntohs -> 역변환 ( cpu포맷으로 변환)

  if(bind(servSock, (SOCKADDR*) &servAddr, sizeof(servAddr))==SOCKET_ERROR) //2번째 함수 
    ErrorHandling("bind() error");

  if(listen(servSock, 5)==SOCKET_ERROR) //버퍼 5명
    ErrorHandling("listen() error");

  while(1){ //채팅 관련
	  clntAddrSize=sizeof(clntAddr);
	  clntSock=accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize); //새로운 손님 기다림 클라이언트용 소켓 생성
	  if(clntSock==INVALID_SOCKET)
		  ErrorHandling("accept() error");

	  WaitForSingleObject(hMutex, INFINITE); //mutex로 보호 (락)
	  clntSocks[clntNumber++]=clntSock; //소켓을 소켓배열에 저장
	  ReleaseMutex(hMutex);				//mutex로 보호 (릴리즈)
	  printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clntAddr.sin_addr));

	  hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID); // 새로운 손님을 위한 쓰레드 생성 쓰레드내부에서는 clientConn함수실행, 매개변수로 clntSock변수, 
	  if(hThread == 0) {
		  ErrorHandling("쓰레드 생성 오류");
	  }
  }

  WSACleanup();
  return 0;
}

DWORD WINAPI ClientConn(void *arg)
{
  SOCKET clntSock=(SOCKET)arg;
  int strLen=0;
  char message[BUFSIZE];
  int i,j;
  char name[20];
  while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0) //소켓에서 문자가 오는걸 기다림 message에 저장 strLen=문자열의 갯수
  {
	  if (strstr(message, "@@join")) { //join
		  i = 0;
		  while (message[i] != '[')
			  i++;
		  i++;
		  while (message[i] != ']') {
			  i++;
		  }
		  while (message[i] != '[')
			  i++;
		  j = 0;
		  i++;
		  while (message[i] != ']') {
			  name[j] = message[i];
			  j++;
			  i++;
		  }
		  name[j] = '\0';
		  addUser(name, strlen(name));
	  }
	  else if (strstr(message, "@@out")) { //out
		  break;
	  }
	  else if (strstr(message, "@@member")) { //member
		  SendLIST(clntSock);
	  }
	  else if (strstr(message, "@@talk")) { //talk
		  p2pMSG(message);
	  }
	  else {
		  SendMSG(message, strLen); // 브로드케스트 메시지
	  }
  }
  
  WaitForSingleObject(hMutex, INFINITE); // 락
  for (i = 0; i<clntNumber; i++) {   // 클라이언트 연결 종료시
	  if (clntSock == clntSocks[i]) {
		  strcpy(name, userList[i]);
		  sprintf(message, "User [%s] Quit\n", name);
		  SendMSG(message, strlen(message));
		  for (; i < clntNumber - 1; i++) {
			  clntSocks[i] = clntSocks[i + 1];
			  strcpy(userList[i], userList[i + 1]);
		  }
		  break;
	  }
  }
  clntNumber--;
  ReleaseMutex(hMutex); // 릴리즈

  closesocket(clntSock);
  return 0;
}
void p2pMSG(char* message) {
	char Sendname[20]; //보내는 사람 이름 
	char Reciname[20]; //받는 사람 이름
	char contents[80]; //메시지 내용
	char msg[100];     //보내는 메시지
	int sendID;
	int i;
	int j;
	i = 0;
	while (message[i] != '[')
		i++;
	j = 0;
	i++;
	while (message[i] != ']') {
		Sendname[j] = message[i];
		j++;
		i++;
	}
	Sendname[j] = '\0';
	while (message[i] != '[')
		i++;
	j = 0;
	i++;
	while (message[i] != ']') {
		Reciname[j] = message[i];
		j++;
		i++;
	}
	Reciname[j] = '\0';
	while (message[i] != '[')
		i++;
	j = 0;
	i++;
	while (message[i] != ']') {
		contents[j] = message[i];
		j++;
		i++;
	}
	contents[j] = '\0';
	sprintf(msg, "비밀글 : [%s] [%s]\n",Sendname,contents);
	WaitForSingleObject(hMutex, INFINITE);
	i = clntNumber;
	while (1) {
		if (!strcmp(userList[i-1], Reciname)) { //받는사람 검색
			send(clntSocks[i-1], msg, strlen(msg), 0);
			break;
		}
		if (!strcmp(userList[i-1],Sendname)) //보내는 사람검색
			sendID = i-1;
		if (i==0)
		{
			sprintf(msg, "[%s] User not Join\n", Reciname); //받는사람이 list에 없으면 메시지 반환
			send(clntSocks[sendID], msg, strlen(msg), 0);
			break;
		}
		i--;
	}
	ReleaseMutex(hMutex);
}
void SendLIST(SOCKET sock) { //리스트 전송
	int i;
	char List[100]; //List 메시지
	sprintf(List, "User List : ");
	WaitForSingleObject(hMutex, INFINITE); // 락
	for (i = 0; i<clntNumber; i++)    // 클라이언트 연결 종료시
			sprintf(List,"%s [%s]",List,userList[i]); //메시지에 유저 리스트 저장
	sprintf(List,"%s\n", List);
	send(sock, List, strlen(List), 0);
	ReleaseMutex(hMutex); // 릴리즈

}
void addUser(char* name, int len) { // join했을때 userList에 추가
	char Message[BUFSIZE];
	sprintf(Message, "User [%s] Join\n", name);
	strcpy(userList[clntNumber - 1] ,name);
	SendMSG(Message, strlen(Message)); //접속자 이름 유저들에게 전송
}

void SendMSG(char* message, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE); 
	for(i=0; i<clntNumber; i++)
		send(clntSocks[i], message, len, 0);
	ReleaseMutex(hMutex);	
}

void ErrorHandling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
