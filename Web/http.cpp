#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<string>
#include<sys/types.h>
#include<sys/stat.h>
//window��sokcetͷ�ļ�������"WS2_32.lib"��̬��
#include<winsock2.h>
#pragma comment(lib,"WS2_32.lib")
using  namespace std;

void cat(SOCKET client, FILE* resource);

void error_die(const string str)
{
	perror(str.c_str());
	exit(1);
}

//��ʼ���׽��� 
//return:�׽���	����:port
SOCKET initSocket(unsigned short int& port)
{
	//��ʼ������ͨѶ win��,linux����Ҫ
	//��ʼ��WSA
	WSADATA data;//WSADATA�ṹ������ĵ�ֵַ
	//2.2�汾Э��
	if (WSAStartup(MAKEWORD(1, 1), &data))
	{
		error_die("WSAStartup() error!");
	}

	//�����׽���
	SOCKET server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1)
	{
		error_die("socket() error!");
	}

	//���ö˿ڿɸ���
	int opt = 1;
	int ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	if (ret == -1)
	{
		error_die("setsockopt() error!");
	}

	//�󶨵�ַ�Ͷ˿ں�
	sockaddr_in sin;						//ipv4��ָ��������ʹ��struct sockaddr_in���͵ı���
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);				//���ö˿ڡ�htons��������unsigned short intת��Ϊ�����ֽ�˳��
	sin.sin_addr.S_un.S_addr = INADDR_ANY;	//IP��ַ���ó�INADDR_ANY����ϵͳ�Զ���ȡ������IP��ַ
	//bind������һ����ַ���е��ض���ַ����scket��
	if (bind(server_socket, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		error_die("bind error !");
	}

	//��̬����port
	int addrlen = sizeof(sin);
	if (port == 0)
	{
		if (getsockname(server_socket, (sockaddr*)&sin, &addrlen) < 0)
		{
			error_die("getsockname error!");
		}
		//����port��main����
		port = sin.sin_port;
	}

	//��ʼ����
	if (listen(server_socket, 5) == SOCKET_ERROR)
	{
		error_die("listen error !");
	}

	return server_socket;
}

//��ͨѶ�׽��֣���ȡһ������
//return ��ȡ���ݳ��� 
int get_line(int sock, char* buff, int size)
{
	char c = 0;		//'\0'
	int i = 0;

	while (i < size - 1 && c != '\n')
	{
		int n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n')
				{
					recv(sock, &c, 1, 0);
				}
				else
				{
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}

	buff[i] = 0;//'\0'
	return i;
}

//������ʾ��û��ʵ�ֵĴ���
void unimplement(SOCKET clinet)
{
	
}
//������ʾ����ҳ�����ڵĴ���  ����404
void not_found(SOCKET client)
{
	char buff[1024];
	//������Ӧͷ
	strcpy(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Server: WebHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	FILE* resource = fopen("htdocs/notFound.html", "r");
	cat(client, resource);
}
//������Ӧ����ͷ��Ϣ
void heards(SOCKET client,const char* type)
{
	char buff[1024];
	//������Ӧͷ
	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff),0); 

	strcpy(buff, "Server: WebHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	sprintf(buff, "Content-type:%s\r\n", type);
	//strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}
//�����������Դ��Ϣ
void cat(SOCKET client,FILE* resource)
{
	char buff[4096];
	int count = 0;

	while (1)
	{
		int num = fread(buff, sizeof(char), sizeof(buff), resource);
		if (num <= 0)
		{
			break;
		}
		send(client, buff, num, 0);
		count += num;
	}
	cout << "һ��������: " << count << "���ֽ�����" << endl;
	fclose(resource);
}
//��ȡ�����ļ����ͣ�������Ӧͷ����
const char* getHeadType(const char* filename)
{
	const char* ret = "text/html";
	const char* p = strrchr(filename, '.');
	if (!p)
		return ret;
	++p;
	if (!strcmp(p, "css"))
		ret = "text/css";
	else if (!strcmp(p, "jpg"))
		ret = "image/jpeg";
	else if (!strcmp(p, "png"))
		ret = "image/png";
	else if (!strcmp(p, "js"))
		ret = "application/x-javascript";
	else if(!strcmp(p,".pdf"))
		ret = "application/octet-stream";
	return ret;
}

//����˷��� ����·�����ļ����� ���ͻ���
void server_file(SOCKET client, const char* filename)
{
	int numchars = 1;
	char buff[1024];
	//��ȡ��������ʣ�������
	while (numchars > 0 && strcmp(buff, "\n"))
	{
		numchars = get_line(client, buff, sizeof(buff));
		cout << buff << endl;
	}

	FILE* resource = NULL;
	//������Դ������� HTTPЭ��
	
	if (strcmp(filename, "htdocs/index/html")==0)
	{
		resource = fopen(filename, "r");
	}
	else
	{
		resource = fopen(filename, "rb");
		//heards(client, "application/octet-stream");
	}
	if (resource == NULL)
	{
		not_found(client);
	}
	else
	{
		//������Ӧ����ͷ��Ϣ
		heards(client, getHeadType(filename));

		//�����������Դ��Ϣ
		cat(client, resource);

		cout << "��Դ�ѷ���!" << endl;
	}
}

//�̹߳�������
DWORD WINAPI working(LPVOID arg)
{
	SOCKET client = (SOCKET)arg;	//ͨѶ�׽���
	char buff[1024];			//1K

	//��һ������
	int numchars = get_line(client, buff, sizeof(buff));
	//�����ǰ���յ�������
	cout << buff << endl;

	//��������
	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;		//'\0'
	cout <<"method: " << method << endl;

	//�������ͷ���Ƿ�֧�ֽ��� GET POST
	if (_stricmp(method, "GET") != 0 && _stricmp(method, "POST") != 0)
	{
		unimplement(client);
		return 0;
	}
	//������Դ·��
	char url[255];
	i = 0;
	while (isspace(buff[j]) && i < sizeof(buff)) j++;

	while (!isspace(buff[j]) && i < sizeof(url) - 1 &&j<sizeof(buff))
	{
		url[i++] = buff[j++];
	}
	url[i] = 0; 
	cout << "url: " << url << endl;

	//	htdocs/test.html
	char path[512] = "";
	sprintf(path, "htdocs%s", url);		//   = htdoce/
	if (path[strlen(path) - 1] == '/')
	{
		strcat(path, "index.html");		//   =htdoce/index.html
	}
	cout << "path: " << path << endl;

	//ʹ��stat������ȡ·��path��ָ���ļ���Ŀ¼��״̬��Ϣ�������䱣����status�ṹ���С�
	struct stat status;
	if (stat(path, &status) == -1)//���stat��������-1��˵������·�����ļ���Ŀ¼������
	{
		//��ȡ������ʣ������������ݣ���ֹ�´����
		while (numchars > 0 && strcmp(buff, "\n"))
		{
			numchars=get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else//����,��˵���ļ���Ŀ¼���ڡ������Ŀ¼���������Ὣ�����·���޸�Ϊ��Ŀ¼�µ�index.html�ļ�������������
	{
		//·���Ƿ�Ϊһ��Ŀ¼�������Ŀ¼����·��ƴ�����ļ��� index.html��
		if ((status.st_mode & S_IFMT) == S_IFDIR)	//status.st_mode ��һ�� struct stat �ṹ���еĳ�Ա��
		{											//����¼���ļ������ͺͷ���Ȩ�޵���Ϣ�������S_IFMT ��һ���꣬��ʾ�ļ����͵����룬
			strcat(path, "index.html");				//����ͨ����λ������� & �� st_mode ���бȽϣ��ж��ļ������͡�
		}
		//�����ǰ���ļ�����ֱ�Ӵ�������,
		server_file(client, path);
	}

	//�رռ����׽���
	closesocket(client);
	return 0;
}

int main(void)
{
	unsigned short port = 8000;					//portΪ0����̬������ж˿�
	int server_socket = initSocket(port);
	cout << "http��������....���ڼ���" << port << "�˿�..." << endl;

	sockaddr_in client_addr;					//sockaddr_in������socket����͸�ֵ,sockaddr���ں�������
	int client_addr_len = sizeof(client_addr);

	//�ȴ����ӿͻ���
	while (true)
	{
		//�����ȴ��ͻ��˵�����,return :ͨѶ�׽��� �ͻ��˵�ַ
		SOCKET client_sock = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1)
		{
			error_die("accept error!");
		}

		//�������߳�
		DWORD threadId;
		CreateThread(0, 0, working, (void*)client_sock, 0, &threadId);
	}

	closesocket(server_socket);
}