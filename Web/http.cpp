#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<string>
#include<sys/types.h>
#include<sys/stat.h>
//window下sokcet头文件，链接"WS2_32.lib"动态库
#include<winsock2.h>
#pragma comment(lib,"WS2_32.lib")
using  namespace std;

void cat(SOCKET client, FILE* resource);

void error_die(const string str)
{
	perror(str.c_str());
	exit(1);
}

//初始化套接字 
//return:套接字	传入:port
SOCKET initSocket(unsigned short int& port)
{
	//初始化网络通讯 win下,linux不需要
	//初始化WSA
	WSADATA data;//WSADATA结构体变量的地址值
	//2.2版本协议
	if (WSAStartup(MAKEWORD(1, 1), &data))
	{
		error_die("WSAStartup() error!");
	}

	//创建套接字
	SOCKET server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1)
	{
		error_die("socket() error!");
	}

	//设置端口可复用
	int opt = 1;
	int ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	if (ret == -1)
	{
		error_die("setsockopt() error!");
	}

	//绑定地址和端口号
	sockaddr_in sin;						//ipv4的指定方法是使用struct sockaddr_in类型的变量
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);				//设置端口。htons将主机的unsigned short int转换为网络字节顺序
	sin.sin_addr.S_un.S_addr = INADDR_ANY;	//IP地址设置成INADDR_ANY，让系统自动获取本机的IP地址
	//bind函数把一个地址族中的特定地址赋给scket。
	if (bind(server_socket, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		error_die("bind error !");
	}

	//动态分配port
	int addrlen = sizeof(sin);
	if (port == 0)
	{
		if (getsockname(server_socket, (sockaddr*)&sin, &addrlen) < 0)
		{
			error_die("getsockname error!");
		}
		//传递port到main函数
		port = sin.sin_port;
	}

	//开始监听
	if (listen(server_socket, 5) == SOCKET_ERROR)
	{
		error_die("listen error !");
	}

	return server_socket;
}

//从通讯套接字，读取一行数据
//return 读取数据长度 
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

//发送提示还没有实现的错误
void unimplement(SOCKET clinet)
{
	
}
//发送提示，网页不存在的错误  发送404
void not_found(SOCKET client)
{
	char buff[1024];
	//构造响应头
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
//发送响应包的头信息
void heards(SOCKET client,const char* type)
{
	char buff[1024];
	//构造响应头
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
//发送请求的资源信息
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
	cout << "一共发送了: " << count << "个字节数据" << endl;
	fclose(resource);
}
//获取发送文件类型，设置响应头属性
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

//服务端发送 传入路径的文件数据 给客户端
void server_file(SOCKET client, const char* filename)
{
	int numchars = 1;
	char buff[1024];
	//读取请求行体剩余的数据
	while (numchars > 0 && strcmp(buff, "\n"))
	{
		numchars = get_line(client, buff, sizeof(buff));
		cout << buff << endl;
	}

	FILE* resource = NULL;
	//发送资源给浏览器 HTTP协议
	
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
		//发送响应包的头信息
		heards(client, getHeadType(filename));

		//发送请求的资源信息
		cat(client, resource);

		cout << "资源已发送!" << endl;
	}
}

//线程工作函数
DWORD WINAPI working(LPVOID arg)
{
	SOCKET client = (SOCKET)arg;	//通讯套接字
	char buff[1024];			//1K

	//读一行数据
	int numchars = get_line(client, buff, sizeof(buff));
	//输出当前接收到的数据
	cout << buff << endl;

	//解析数据
	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;		//'\0'
	cout <<"method: " << method << endl;

	//检查请求头，是否支持解析 GET POST
	if (_stricmp(method, "GET") != 0 && _stricmp(method, "POST") != 0)
	{
		unimplement(client);
		return 0;
	}
	//解析资源路径
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

	//使用stat函数获取路径path所指的文件或目录的状态信息，并将其保存在status结构体中。
	struct stat status;
	if (stat(path, &status) == -1)//如果stat函数返回-1，说明访问路径的文件或目录不存在
	{
		//读取缓存区剩余的请求体数据，防止下次误读
		while (numchars > 0 && strcmp(buff, "\n"))
		{
			numchars=get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else//否则,则说明文件或目录存在。如果是目录，服务器会将请求的路径修改为该目录下的index.html文件，并继续处理
	{
		//路径是否为一个目录，如果是目录，则将路径拼接上文件名 index.html。
		if ((status.st_mode & S_IFMT) == S_IFDIR)	//status.st_mode 是一个 struct stat 结构体中的成员，
		{											//它记录了文件的类型和访问权限等信息。在这里，S_IFMT 是一个宏，表示文件类型的掩码，
			strcat(path, "index.html");				//可以通过按位与运算符 & 与 st_mode 进行比较，判断文件的类型。
		}
		//如果当前是文件，则直接处理请求,
		server_file(client, path);
	}

	//关闭监听套接字
	closesocket(client);
	return 0;
}

int main(void)
{
	unsigned short port = 8000;					//port为0，动态分配空闲端口
	int server_socket = initSocket(port);
	cout << "http服务启动....正在监听" << port << "端口..." << endl;

	sockaddr_in client_addr;					//sockaddr_in常用于socket定义和赋值,sockaddr用于函数参数
	int client_addr_len = sizeof(client_addr);

	//等待连接客户端
	while (true)
	{
		//阻塞等待客户端的连接,return :通讯套接字 客户端地址
		SOCKET client_sock = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1)
		{
			error_die("accept error!");
		}

		//创建新线程
		DWORD threadId;
		CreateThread(0, 0, working, (void*)client_sock, 0, &threadId);
	}

	closesocket(server_socket);
}