
/*
The priority queue is implemented as a binary heap. The heap stores an index into its data array, so it can quickly update the weight of an item already in it.
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
// 쓸데없는 에러를 경고로 변환
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <time.h>
#include <math.h>

#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUFSIZE 100

DWORD WINAPI ClientConn(void *arg);
void SendMSG(char* message, int len);
void ErrorHandling(char *message);
char mess[100];
int clntNumber = 0;
SOCKET clntSocks[10];
HANDLE hMutex;
int numvert, numedge, maxcost, minedge, maxedge, startID, mode, fileID;
int fh1, fh2;
int bytes, cnt, r, *costarray;
int *temp, *edgearray;
char filename[100], buffer[12]; 

#define BILLION 1000000000L;

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif


struct timezone
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tmpres /= 10;  /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
	}

	return 0;
} // 시간을 계산할때 성능 구려서 신경X
 
typedef struct {
    int vertex;
    int weight;
} edge_t; //선구조체
 
typedef struct {
    edge_t **edges;
    int edges_len;
    int edges_size;
    int dist;
    int prev;
    int visited;
} vertex_t; // 노드 구조체
 
typedef struct {
    vertex_t **vertices;
    int vertices_len;
    int vertices_size;
} graph_t; //

graph_t* g;
 
typedef struct {
    int *data;
    int *prio;
    int *index;
    int len;
    int size;
} heap_t;
 
void add_vertex ( int i) {
    if (g->vertices_size < i + 1) {
        int size = g->vertices_size * 2 > i ? g->vertices_size * 2 : i + 4;
        g->vertices = realloc(g->vertices, size * sizeof (vertex_t *));
        for (int j = g->vertices_size; j < size; j++)
            g->vertices[j] = NULL;
        g->vertices_size = size;
    }
    if (!g->vertices[i]) {
        g->vertices[i] = calloc(1, sizeof (vertex_t));
        g->vertices_len++;
    }
}//신경X (노드 추가할때)
 
int add_edge ( int a, int b, int w) {//a,b는 점좌표 w는코스트
	a = a - 1;
    b = b - 1;
    add_vertex(a);
    add_vertex(b);
    vertex_t *v = g->vertices[a];
    if (v->edges_len >= v->edges_size) {
        v->edges_size = v->edges_size ? v->edges_size * 2 : 4;
        v->edges = realloc(v->edges, v->edges_size * sizeof (edge_t *));
    }
	int i;
	for (i = 0; i < v->edges_len; i++)
		if (v->edges[i]->vertex == b) break;
	if (i == v->edges_len)
	{
		edge_t *e = calloc(1, sizeof(edge_t));
		e->vertex = b;
		e->weight = w;
		v->edges[v->edges_len++] = e;
		return 1;
	}
	return 0;
}

heap_t *create_heap (int n) {
    heap_t *h = calloc(1, sizeof (heap_t));
    h->data = calloc(n + 1, sizeof (int));
    h->prio = calloc(n + 1, sizeof (int));
    h->index = calloc(n, sizeof (int));
    return h;
}
 
void push_heap (heap_t *h, int v, int p) {
	int i = h->index[v];
	if(!i) i = ++h->len;
    int j = i / 2;
    while (i > 1) {
        if (h->prio[j] < p)
            break;
        h->data[i] = h->data[j];
        h->prio[i] = h->prio[j];
        h->index[h->data[i]] = i;
        i = j;
        j = j / 2;
    }
    h->data[i] = v;
    h->prio[i] = p;
    h->index[v] = i;
} //새로운 노드를 집어 넣을때
 
int min2 (heap_t *h, int i, int j, int k) {
    int m = i;
    if (j <= h->len && h->prio[j] < h->prio[m])
        m = j;
    if (k <= h->len && h->prio[k] < h->prio[m])
        m = k;
    return m;
} // 최소값을 가진 것을 찾아내서 업데이트
 
int pop_heap (heap_t *h) {
    int v = h->data[1];
    h->data[1] = h->data[h->len];
    h->prio[1] = h->prio[h->len];
    h->index[h->data[1]] = 1;
    h->len--;
    int i = 1;
    while (1) {
        int j = min2(h, i, 2 * i, 2 * i + 1);
        if (j == i)
            break;
        h->data[i] = h->data[j];
        h->prio[i] = h->prio[j];
        h->index[h->data[i]] = i;
        i = j;
    }
    h->data[i] = h->data[h->len + 1];
    h->prio[i] = h->prio[h->len + 1];
    h->index[h->data[i]] = i;
    return v;
} //미니멈 찾음
 
void dijkstra( int a) {//그래프, 시작노드
    int i, j;
	a = a - 1;
    for (i = 0; i < g->vertices_len; i++) {
        vertex_t *v = g->vertices[i];
        v->dist = INT_MAX;
        v->prev = 0;
        v->visited = 0;
    }
    vertex_t *v = g->vertices[a];
    v->dist = 0;
    heap_t *h = create_heap(g->vertices_len);
    push_heap(h, a, v->dist);
    while (h->len) {
        i = pop_heap(h);
        v = g->vertices[i];
        v->visited = 1;
        for (j = 0; j < v->edges_len; j++) {
            edge_t *e = v->edges[j];
            vertex_t *u = g->vertices[e->vertex];
            if (!u->visited && v->dist + e->weight <= u->dist) {
                u->prev = i;
                u->dist = v->dist + e->weight;
                push_heap(h, e->vertex, u->dist);
            }
        }
    }
} //다이제스트라 참고하여 만듬
void send_path(SOCKET sock) {
	int n, j;
	vertex_t *v, *u;
	unsigned char m[4];
	for (int k = 0; k < g->vertices_len; k++)
	{
		v = g->vertices[k];
		if (v->dist == INT_MAX) {
			unsigned int temp = v->dist;
			m[3] = temp % 256;
			temp /= 256;
			m[2] = temp % 256;
			temp /= 256;
			m[1] = temp % 256;
			temp /= 256;
			m[0] = temp % 256;
			send(sock, m, 4, 0);
			printf("%s", m);
			continue;
		}
		for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
			;
		unsigned int temp = v->dist;
		m[3] = temp % 256;
		temp /= 256;
		m[2] = temp % 256;
		temp /= 256;
		m[1] = temp % 256;
		temp /= 256;
		m[0] = temp % 256;
		send(sock, m, 4, 0);
		printf("%s", m);
		printf("%d (%d): ", k + 1, v->dist);
		int *path = malloc(n * sizeof(int));
		path[n - 1] = 1 + k;
		for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
			path[n - j - 2] = 1 + u->prev;
		for (j = 0; j < n; j++)
			printf("%d ", path[j]);
		printf("\n");
	}
} //출력

void print_path() {
    int n, j;
    vertex_t *v, *u;

	for (int i = 0; i < g->vertices_len; i++)
	{
		printf("%d: ", i + 1);
		for (int j = 0; j < g->vertices[i]->edges_len; j++)
			printf("(%d, %d), ", g->vertices[i]->edges[j]->vertex + 1, g->vertices[i]->edges[j]->weight);
		printf("\n");
	}

	for (int k = 0; k < g->vertices_len; k++)
	{
		v = g->vertices[k];
		if (v->dist == INT_MAX) {
			printf("%d (%d): no path\n", k + 1, v->dist);
			continue;
		}
		for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
			;
		printf("%d (%d): ", k + 1, v->dist);
		int *path = malloc(n * sizeof(int));
		path[n - 1] = 1 + k;
		for (j = 0, u = v; u->dist; u = g->vertices[u->prev], j++)
			path[n - j - 2] = 1 + u->prev;
		for (j = 0; j < n; j++)
			printf("%d ", path[j]);
		printf("\n");
	}
} //출력

int datread(int fh, char *buffer, int size)
{
	int bytes;
	if ((bytes = _read(fh, buffer, size)) == -1)
	{
		switch (errno)
		{
		case EBADF:
			perror("Bad file descriptor!");  exit(-1);
			break;
		case ENOSPC:
			perror("No space left on device!");  exit(-1);
			break;
		case EINVAL:
			perror("Invalid parameter: buffer was NULL!");  exit(-1);
			break;
		default:
			// An unrelated error occured
			perror("Unexpected error!");  exit(-1);
		}
	}
	return bytes;
}

void datwrite(int fh, char *buffer, int size)
{
	int bytes;
	if ((bytes = _write(fh, buffer, size)) == -1 || bytes != size)
	{
		if (bytes != size) { perror("less bytes were writen!");  exit(-1); }
		switch (errno)
		{
		case EBADF:
			perror("Bad file descriptor!");  exit(-1);
			break;
		case ENOSPC:
			perror("No space left on device!");  exit(-1);
			break;
		case EINVAL:
			perror("Invalid parameter: buffer was NULL!");  exit(-1);
			break;
		default:
			// An unrelated error occured
			perror("Unexpected error!");  exit(-1);
		}
	}
}

void SPA_compute( int numvert, int startID)
{
	struct timeval start, end;

	gettimeofday(&start, NULL);

	dijkstra( startID);

	gettimeofday(&end, NULL);
	printf("computing time = %ld(microsecond)\n", ((end.tv_sec * 1000000 + end.tv_usec)
	- (start.tv_sec * 1000000 + start.tv_usec)));
}

DWORD WINAPI ClientConn(void *arg)
{
	SOCKET clntSock = (SOCKET)arg;
	vertex_t *v, *u;
	int n, j;
	int strLen = 0;
	char message[BUFSIZE];
	unsigned char me[12];
	int i;
	unsigned int temp;
	struct timeval start, end;
	gettimeofday(&start, NULL);
	temp = startID;
	me[11] = temp % 256;
	temp /= 256;
	me[10] = temp % 256;
	temp /= 256;
	me[9] = temp % 256;
	temp /= 256;
	me[8] = temp % 256;
	temp = numedge;
	me[7] = temp % 256;
	temp /= 256;
	me[6] = temp % 256;
	temp /= 256;
	me[5] = temp % 256;
	temp /= 256;
	me[4] = temp % 256;
	temp = numvert;
	me[3] = temp % 256;
	temp /= 256;
	me[2] = temp % 256;
	temp /= 256;
	me[1] = temp % 256;
	temp /= 256;
	me[0] = temp % 256;
	send(clntSock, me, 12, 0);

	for (i = 0; i < g->vertices_len; i++) {
		for (int j = 0; j < g->vertices[i]->edges_len; j++) {
			//printf("(%d, %d), ", g->vertices[i]->edges[j]->vertex + 1, g->vertices[i]->edges[j]->weight);
			temp = g->vertices[i]->edges[j]->weight;
			me[11] = temp % 256;
			temp /= 256;
			me[10] = temp % 256;
			temp /= 256;
			me[9] = temp % 256;
			temp /= 256;
			me[8] = temp % 256;
			temp = g->vertices[i]->edges[j]->vertex;
			me[7] = temp % 256;
			temp /= 256;
			me[6] = temp % 256;
			temp /= 256;
			me[5] = temp % 256;
			temp /= 256;
			me[4] = temp % 256;
			temp = i ;
			me[3] = temp % 256;
			temp /= 256;
			me[2] = temp % 256;
			temp /= 256;
			me[1] = temp % 256;
			temp /= 256;
			me[0] = temp % 256;
			send(clntSock, me, 12, 0);
		}
	}
	unsigned char m[4];
	bool isTrue = true;
	int count = 0;
	for (int k = 0; k < g->vertices_len; k++)
	{
		strLen = recv(clntSock, m, 4, 0);
		v = g->vertices[k];
		if (v->dist == INT_MAX) {
			printf("%d (%d): no path", k + 1, v->dist);
			continue;
		}
		for (n = 1, u = v; u->dist; u = g->vertices[u->prev], n++)
			;
		int bin = 1;
		temp = 0;
		for (int i = 3; i >= 0; i--, bin *= 256) {
			temp += bin*m[i];
		}
		if (v->dist != temp) {
			isTrue = false;
			count++;
		}
	}

	gettimeofday(&end, NULL);
	if (isTrue) {
		//sprintf(message,"%s -> SPA cost match passed!, computing-time = %d(usec)", mess, ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
		sprintf(message, "SPA cost match passed! computing-time = %d(usec)", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
		printf("%s\n", message);
		send(clntSock, message, strlen(message), 0);
	}
	else {
		//sprintf(message, "%s -> SPA cost match failed!, (wrong # of distance=%d)", mess,count);
		sprintf(message, "SPA cost match failed! (wrong # of distance=%d)", count);
		printf("%s\n", message);
		send(clntSock, message, strlen(message), 0);
	}

	/*while ((strLen = recv(clntSock, message, BUFSIZE, 0)) != 0)
		SendMSG(message, strLen);

	WaitForSingleObject(hMutex, INFINITE);*/
	for (i = 0; i<clntNumber; i++) {   // 클라이언트 연결 종료시
		if (clntSock == clntSocks[i]) {
			for (; i<clntNumber - 1; i++)
				clntSocks[i] = clntSocks[i + 1];
			break;
		}
	}
	clntNumber--;
	ReleaseMutex(hMutex);

	closesocket(clntSock);
	return 0;
}

void SendMSG(char* message, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i<clntNumber; i++)
		send(clntSocks[i], message, len, 0);
	ReleaseMutex(hMutex);
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


int main () {
	WSADATA wsaData;
	SOCKET servSock;
	SOCKET clntSock;
	SOCKET sock;
	unsigned char addr[100], name[20], ID[20], msg[100];
	int port;
	struct timeval start, end;

	SOCKADDR_IN servAddr;
	SOCKADDR_IN clntAddr;
	int clntAddrSize;

	HANDLE hThread;
	DWORD dwThreadID;

	while (1)
	{
		printf("mode(1=graph generation, 5=client, 6=online graph server, 9=quit): ");
		scanf("%d", &mode);
		if (mode == 9) break;
		switch (mode)
		{
		case 1: // 1=graph generation
			g = calloc(1, sizeof(graph_t));
			printf("vertex#, startID, maxcost, min_edge#, max_edge#: ");
			scanf("%d %d %d %d %d", &numvert, &startID, &maxcost, &minedge, &maxedge);
			int range = maxedge - minedge + 1;
			numedge = 0;
			for (int i = 0; i < numvert; i++)
			{
				int curvert = rand() % range + minedge;
				int upper = i + 1 + maxedge * 2; if (upper > numvert - 1) upper = numvert - 1;
				int buttom = upper - maxedge * 2; if (buttom < 0) buttom = 0;
				int r2 = upper - buttom + 1;
				while (g->vertices_size < i + 1 || !g->vertices[i] || curvert > g->vertices[i]->edges_len)
				{
					int loc = rand() % r2 + buttom;
					if (loc == i) continue;
					int ed = rand() % maxcost + 1;
					numedge += add_edge( i + 1, loc + 1, ed);
					numedge += add_edge( loc + 1, i + 1, ed);
				}
			}
			SPA_compute( numvert, startID);
			print_path();

			break;

		case 5: // client 실습2
			g = calloc(1, sizeof(graph_t));
			printf("IP address, port #, 이름, 학번 : ");
			int  port, sid;
			char ip[100];
			char sname[20] ;
			scanf("%s %d %s %d", &ip, &port, &sname, &sid);

			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
				ErrorHandling("WSAStartup() error!");

			sock = socket(PF_INET, SOCK_STREAM, 0); // 소켓 초기화
			if (sock == INVALID_SOCKET)
				ErrorHandling("socket() error");

			memset(&servAddr, 0, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			//servAddr.sin_addr.s_addr = inet_addr(ip); //서버 주소
			//servAddr.sin_port = htons(port); // 서버 포트
			servAddr.sin_addr.s_addr = inet_addr(ip); //서버 주소
			servAddr.sin_port = htons(port); // 서버 포트
			if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
				ErrorHandling("connect() error");
			unsigned char message[100];
			sprintf(message, "%s %d", sname, sid);
			send(sock, message, strlen(message), 0);
			unsigned int numNode, numLink, start;
			unsigned int *link;
			numNode = numLink = start = 0;
			unsigned char m[12];
			unsigned int temp = 1;
			int strLen = recv(sock, m, 12, 0);
			for (int i = 3; i >= 0; i--, temp *= 256) {
				numNode += temp*m[i];
				numLink += temp*m[i + 4];
				start += temp*m[i + 8];
			}
			
			int ver1, ver2, cost;
			for (int i = 0; i < numLink; i++) {
				strLen = recv(sock, m, 12, 0);
				temp = 1;
				ver1 = ver2 = cost = 0;
				for (int j = 3; j >= 0; temp *= 256, j--) {
					ver1 += temp*m[j];
					ver2 += temp*m[j + 4];
					cost += temp*m[j + 8];
				}
				add_edge( (ver1 + 1), (ver2 + 1), cost);
			}
			SPA_compute( numNode, start);
			send_path(sock);
			strLen = recv(sock, message,100, 0);
			message[strLen] = 0;
			fputs(message, stdout);
			printf("\n");
			closesocket(sock);
			break;


		case 6: // online graph server 실습 3
			printf("test-serer-IP-address, test-server-port #:");
			
			//scanf("%d", &port);
			scanf("%s %d",&ip, &port);
			printf("student-serer-IP-address, student-server-port #, 이름 학번:");
			char message1[100];
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */
				ErrorHandling("WSAStartup() error!");

			sock = socket(PF_INET, SOCK_STREAM, 0); // 소켓 초기화
			if (sock == INVALID_SOCKET)
				ErrorHandling("socket() error");

			memset(&servAddr, 0, sizeof(servAddr));
			servAddr.sin_family = AF_INET;
			//servAddr.sin_addr.s_addr = inet_addr(ip); //서버 주소
			//servAddr.sin_port = htons(port); // 서버 포트
			servAddr.sin_addr.s_addr = inet_addr(ip); //서버 주소
			servAddr.sin_port = htons(port); // 서버 포트
			if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
				ErrorHandling("connect() error");
			scanf("%s %d %s %d", &ip,&port,&name, &sid);
			sprintf(message1, "%s %d %s %d", ip, port, name, sid);
			send(sock, message1, strlen(message1), 0);
			closesocket(sock);
			
			printf("startID = %d\n", startID);
			WSADATA wsaData;
			SOCKET servSock;
			SOCKET clntSock;

			SOCKADDR_IN servAddr;
			SOCKADDR_IN clntAddr;
			int clntAddrSize;

			HANDLE hThread;
			DWORD dwThreadID;

			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) /* Load Winsock 2.2 DLL */ //winsocker 2.0ver 초기화
				ErrorHandling("WSAStartup() error!");

			hMutex = CreateMutex(NULL, FALSE, NULL); //mutex변수 생성, 초기화, log데이터 보호
			if (hMutex == NULL) {
				ErrorHandling("CreateMutex() error");
			}

			servSock = socket(PF_INET, SOCK_STREAM, 0);   //소켓함수 internet외에서도 쓰겠다, tcp프로토콜 사용하겠다 
			if (servSock == INVALID_SOCKET)				//error 검출 (-값)
				ErrorHandling("socket() error");

			memset(&servAddr, 0, sizeof(servAddr)); //인터넷 주소구조체 변수 초기화
			servAddr.sin_family = AF_INET;			//인터넷 주소구조체 변수 초기화 소켓을 인터넷용도로 쓰겠다
			servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //인터넷 주소구조체 변수 초기화 인터넷 주소 구조체 여기서는 주소 상관없음
			servAddr.sin_port = htons(port); //인터넷 주소구조체 변수 초기화 포트 넘버
													  //atoi -> 문자열을 숫자값으로 변환 htons -> Host to network in short portnum = 2byte의 short형  bigendian, littleendian의 표준화 - Big Endian으로 
													  // ntohs -> 역변환 ( cpu포맷으로 변환)

			if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) //2번째 함수 
				ErrorHandling("bind() error");

			if (listen(servSock, 5) == SOCKET_ERROR) //버퍼 5명
				ErrorHandling("listen() error");

			while (1) { 
				clntAddrSize = sizeof(clntAddr);
				clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize); //새로운 손님 기다림 클라이언트용 소켓 생성
				if (clntSock == INVALID_SOCKET)
					ErrorHandling("accept() error");

				WaitForSingleObject(hMutex, INFINITE); //mutex로 보호 (락)
				clntSocks[clntNumber++] = clntSock; //소켓을 소켓배열에 저장
				ReleaseMutex(hMutex);				//mutex로 보호 (릴리즈)
				
				int strLen;
				strLen=recv(clntSock, mess, 100, 0);
				mess[strLen] = 0;
				printf("새로운 연결, 클라이언트 : %s \n", mess);

				hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void*)clntSock, 0, (unsigned *)&dwThreadID); // 새로운 손님을 위한 쓰레드 생성 쓰레드내부에서는 clientConn함수실행, 매개변수로 clntSock변수, 
				if (hThread == 0) {
					ErrorHandling("쓰레드 생성 오류");
				}
			}

			break;

		default:
			break;
		}
	}
    return 0;
}
