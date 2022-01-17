#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define ERR_LOG(send_msg) do{\
	fprintf(stderr, "%d %s ", __LINE__, __func__);\
	perror(send_msg);\
}while(0)

#define PATH "./dict.txt"

#define N 128

#define R 	1 		//注册
#define L 	2 		//登录
#define Q 	3		//退出

#define S 	4		//查词
#define H 	5 		//历史记录
#define T 	6 		//管理员注册
#define P 	7 		//管理员登入
#define A 	8 		//增
#define D 	9 		//删
#define G 	10 		//改
#define C 	11 		//查
#define Y 	12 		//用户修改

typedef struct
{
	int fd ;
	struct sockaddr_in cin;
}CliCnt;

struct msg
{
	int type;
	char name[20];
	char text[N];
	char age_name[20];
	char age_phone[20];
	char age_add[20];
	int age_salary;
	int age_age;
};

char name[20] = "";
int sockfd = 0;

int init_socket(int* , int  , const char* );

int do_history(int sfd);
int do_login(int sfd);
int loginSuccess(int sfd);
int do_register(int sfd);
int do_select(int sfd);
int do_quit(int sfd);
int Administrator(int sfd);
int ad_register(int sfd); //管理员注册
int ad_login(int sfd);    //管理员用户登入
int ad_loginSuccess(int sfd);   //用户登入页面
int do_insert(int sfd);   //增
int do_delete(int sfd); //删
int do_updata(int sfd); //改
int do_select(int sfd); //查
int pe_updata(int sfd); //用户改
int pe_select(int sfd); //用户查
typedef void(*sighandler_t)(int);
void handler(int sig)
{
	do_quit(sockfd);
	close(sockfd);
	exit(0);
}


int main(int argc, const char *argv[])
{
	sighandler_t s = signal(2, handler);
	if(argc < 3)
	{
		fprintf(stderr, "请输入ip 端口号\n");
		return -1;
	}

	//给 ctrl+c 注册信号处理函数
	if(s == SIG_ERR)
	{
		ERR_LOG("signal");
		return -1;
	}

	/*网络初始化*/
	int sfd = 0;
	if(init_socket(&sfd, atoi(argv[2]), argv[1]) < 0)
	{
		return -1;
	}
	//信号处理函数使用,
	//最后加的功能,所有的sfd都可以用sockfd替代
	sockfd = sfd; 		

	while(1)
	{
		system("clear");
		printf("******************\n");	
		printf("*****1.注册********\n");	
		printf("*****2.用户登录*****\n");	
		printf("*****3.退出********\n");	
		printf("******************\n");	
		printf("*****4.管理员登录***\n");	
		printf("***************\n");

		int choose = 0;
		scanf("%d", &choose);
		while(getchar()!=10);

		switch(choose)
		{
		case R:
			Administrator(sfd);
			break;
		case L:                           //用户登入
			if(do_login(sfd) == 0)
			{
				//进入查单词界面
				ad_loginSuccess(sfd);
			}
			break;
		case S:                          //管理员登入
			if(ad_login(sfd) == 0)
			{
				//进入查单词界面
				loginSuccess(sfd);
			}
			break;
		case Q:
		//	do_quit(sfd);
			close(sfd);
			exit(0);
		}

		printf("请输入任意字符清屏>>>");
		while(getchar()!=10);
	}
	return 0;
}

int do_quit(int sfd)
{
	struct msg send_msg = {htonl(Q)};
	strcpy(send_msg.name, name);
	//发送退出协议
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_LOG("send");
		return -1;
	}
	return 0;
}


int ad_loginSuccess(int sfd)   //用户
{
	printf("****1.请输入姓名****\n");	
	scanf("%s",name);
	while(1)
	{
		system("clear");
		printf("************\n");	
		printf("****1.修改个人信息****\n");	
		printf("****2.查看个人信息****\n");	
		printf("****3.返回上级****\n");	
		printf("*************\n");
		int choose = 0;
		scanf("%d", &choose);
		while(getchar()!=10);
		switch(choose)
		{
		case 1:
			pe_updata(sfd);  //用户修改
			break;
		case 2:
			pe_select(sfd);
			break;
		case 3:
			do_quit(sfd);
			return 0;
		}
		printf("请输入任意字符清屏>>>");
		while(getchar()!=10);
	}

	return 0;
}


int loginSuccess(int sfd)    //管理员
{
	while(1)
	{
		system("clear");
		printf("************\n");	
		printf("****8.增****\n");	
		printf("****9.删****\n");	
		printf("****10.改****\n");	
		printf("****11.查****\n");	
		printf("****5.返回上级****\n");	
		printf("*************\n");
		int choose = 0;
		scanf("%d", &choose);
		while(getchar()!=10);

		switch(choose)
		{
		case 8:
			//增
			do_insert(sfd);
			break;
		case 9:
		    do_delete(sfd);
			//删
			break;
		case 10:
			//改
			do_updata(sfd);
			break;
		case 11:
			//查
			do_select(sfd);
			break;
		case 5:
			do_quit(sfd);
			return 0;
		}
		printf("请输入任意字符清屏>>>");
		while(getchar()!=10);
	}

	return 0;
}


int Administrator(int sfd)
{
	printf("1.管理员注册  2.用户注册\n");
	printf("请输入>>>>>>>\n");
	int Ad = 0;
	scanf("%d",&Ad);
	switch(Ad)
	{
	case 1:
			ad_register(sfd);
		break;
	
	case 2:
			do_register(sfd);
		break;
	}
	return 0;

}

int do_updata(int sfd) //改
{

	struct msg send_msg = {htonl(G)};
	printf("请输入要修改的姓名>>>");
	scanf("%s", send_msg.age_name);
	getchar();
	printf("请输入要修改人的薪资>>>");	
	scanf("%d",&send_msg.age_salary);
	getchar();
	//发送,等待接收意思
	if (send(sfd, &send_msg, sizeof(send_msg), 0) < 0)
	{
		ERR_LOG("send");
		return -1;
	}
}


int pe_updata(int sfd) //用户改
{

	struct msg send_msg = {htonl(Y)};
    strcpy(send_msg.age_name, name);
	printf("请输入要修改人的薪资>>>");	
	scanf("%d",&send_msg.age_salary);
	getchar();
	//发送,等待接收意思
	if (send(sfd, &send_msg, sizeof(send_msg), 0) < 0)
	{
		ERR_LOG("send");
		return -1;
	}
	return 0;
}


int do_delete(int sfd) //删
{
	struct msg send_msg = {htonl(D)};

	printf("请输入要删除的姓名>>>");
	scanf("%s", send_msg.age_name);
	getchar();
	//发送,等待接收意思
	if (send(sfd, &send_msg, sizeof(send_msg), 0) < 0)
	{
		ERR_LOG("send");
		return -1;
	}
}

int do_insert(int sfd)   //增
{
	struct msg send_msg = {htonl(A)};
	printf("请输入姓名>>>>>>>\n");
	scanf("%s", send_msg.age_name);
	getchar();
	printf("请输入手机号>>>>>>\n");
	scanf("%s", send_msg.age_phone);
	getchar();
	printf("请输入地址>>>>>>>\n");
	scanf("%s", send_msg.age_add);
	getchar();
	printf("请输入薪资>>>>>>>\n");
	scanf("%d", &send_msg.age_salary);
	getchar();
	printf("请输入年龄>>>>>>>\n");
	scanf("%d", &send_msg.age_age);

	//发送数据
	if (send(sfd, &send_msg, sizeof(send_msg), 0) < 0)
	{
		ERR_LOG("send");
		return -1;
	}
	return 0;
}



//查询
int do_select(int sfd)
{
	struct msg send_msg = {htonl(C)};

	while(1)
	{
		printf("请输入需要查询的名字(按#号退出)>>>");
		scanf("%s", send_msg.age_name);
		while(getchar()!=10);
		if(strcmp(send_msg.age_name, "#") == 0)
		{
			break;
		}

		//发送,等待接收意思
		if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
		{
			ERR_LOG("send");
			return -1;
		}

		bzero(send_msg.text, sizeof(send_msg.text));
		int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
		if(recv_len < 0)
		{
			ERR_LOG("recv");
			return -1;
		}
		else if(0 == recv_len)
		{
			printf("服务器关闭\n");
			exit(0);
		}

		printf("\t%s\n",send_msg.text);
	}
	return 0;

}

//用户查询
int pe_select(int sfd)
{
	struct msg send_msg = {htonl(C)};
    strcpy(send_msg.age_name, name);

		//发送,等待接收意思
		if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
		{
			ERR_LOG("send");
			return -1;
		}

		bzero(send_msg.text, sizeof(send_msg.text));
		int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
		if(recv_len < 0)
		{
			ERR_LOG("recv");
			return -1;
		}
		else if(0 == recv_len)
		{
			printf("服务器关闭\n");
			exit(0);
		}

		printf("\t%s\n",send_msg.text);
	
	return 0;

}

int do_login(int sfd)    //用户登入
{
	bzero(name, sizeof(name));
	struct msg send_msg = {htonl(L)};
	printf("请输入登录账户名>>>");
	scanf("%s", send_msg.name);
	strcpy(name, send_msg.name );
	while(getchar()!=10);

	printf("请输入登录密码>>>");
	scanf("%s", send_msg.text);
	while(getchar()!=10);

	//发送,等待接收应答
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_LOG("send");
		return -1;
	}

	memset(&send_msg, 0, sizeof(send_msg));
	int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
	if(recv_len < 0)
	{
		ERR_LOG("recv");
		return -1;
	}
	else if(0 == recv_len)
	{
		printf("服务器关闭\n");
		exit(0);
	}

	if(strcmp(send_msg.text, "**OK**") == 0)
	{
		printf("登录成功!\n");
	}
	else if(strcmp(send_msg.text, "**EXISTS**") == 0) 
	{
		printf("登录失败,重复登录!\n");
		return -1;
	}
	else
	{
		printf("登录失败,账号/密码错误!\n");
		return -1;
	}
	return 0;
}


int ad_login(int sfd)    //管理员用户登入
{
	bzero(name, sizeof(name));
	struct msg send_msg = {htonl(P)};
	printf("请输入登录账户名>>>");
	scanf("%s", send_msg.name);
	strcpy(name, send_msg.name );
	while(getchar()!=10);

	printf("请输入登录密码>>>");
	scanf("%s", send_msg.text);
	while(getchar()!=10);

	//发送,等待接收应答
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_LOG("send");
		return -1;
	}

	memset(&send_msg, 0, sizeof(send_msg));
	int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
	if(recv_len < 0)
	{
		ERR_LOG("recv");
		return -1;
	}
	else if(0 == recv_len)
	{
		printf("服务器关闭\n");
		exit(0);
	}

	if(strcmp(send_msg.text, "**OK**") == 0)
	{
		printf("登录成功!\n");
	}
	else if(strcmp(send_msg.text, "**EXISTS**") == 0) 
	{
		printf("登录失败,重复登录!\n");
		return -1;
	}
	else
	{
		printf("登录失败,账号/密码错误!\n");
		return -1;
	}
	return 0;
}



//注册 
int do_register(int sfd)
{
	struct msg send_msg = {htonl(R)};
	printf("请输入账户名>>>");
	scanf("%s", send_msg.name);
	while(getchar()!=10);

	printf("请输入密码>>>");
	scanf("%s", send_msg.text);
	while(getchar()!=10);

	//发送,等待接收应答
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_LOG("send");
		return -1;
	}

	memset(&send_msg, 0, sizeof(send_msg));
	int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
	if(recv_len < 0)
	{
		ERR_LOG("recv");
		return -1;
	}
	else if(0 == recv_len)
	{
		printf("服务器关闭\n");
		exit(0);
	}

	if(strcmp(send_msg.text, "**OK**") == 0)
	{
		printf("注册成功!\n");
	}
	else if(strcmp(send_msg.text, "**EXISTS**") == 0)
	{
		printf("注册失败,该账户已经被注册!\n");
	}
	else
	{
		printf("注册失败,未知错误请联系管理员!\n");
	}
	return 0;
}


//管理员注册 
int ad_register(int sfd)
{
	struct msg send_msg = {htonl(T)};
	printf("请输入账户名>>>");
	scanf("%s", send_msg.name);
	while(getchar()!=10);

	printf("请输入密码>>>");
	scanf("%s", send_msg.text);
	while(getchar()!=10);

	//发送,等待接收应答
	if(send(sfd, &send_msg, sizeof(send_msg), 0)<0)
	{
		ERR_LOG("send");
		return -1;
	}

	memset(&send_msg, 0, sizeof(send_msg));
	int recv_len = recv(sfd, &send_msg, sizeof(send_msg), 0);
	if(recv_len < 0)
	{
		ERR_LOG("recv");
		return -1;
	}
	else if(0 == recv_len)
	{
		printf("服务器关闭\n");
		exit(0);
	}

	if(strcmp(send_msg.text, "**OK**") == 0)
	{
		printf("注册成功!\n");
	}
	else if(strcmp(send_msg.text, "**EXISTS**") == 0)
	{
		printf("注册失败,该账户已经被注册!\n");
	}
	else
	{
		printf("注册失败,未知错误请联系管理员!\n");
	}
	return 0;
}



//网络初始化
int init_socket(int* psfd, int port , const char* ip)
{
	//socket
	*psfd = socket(AF_INET, SOCK_STREAM, 0);
	if(*psfd < 0)
	{
		ERR_LOG("socket");
		return -1;
	}

	//允许端口快速重用
	int values = 0 ;
	if(setsockopt(*psfd, SOL_SOCKET, SO_REUSEADDR, &values, sizeof(int))<0)
	{
		ERR_LOG("setsockopt");
		return -1;
	}


	//填充服务器信息
	struct sockaddr_in sin;
	sin.sin_family 		= AF_INET;
	sin.sin_port  		= htons(port);
	sin.sin_addr.s_addr = inet_addr(ip);

	if(connect(*psfd, (void*)&sin, sizeof(sin)) < 0)
	{
		ERR_LOG("bind");
		return -1;
	}
	
	return 0;
}


