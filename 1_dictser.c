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
#include <time.h>
#include <arpa/inet.h>

#define ERR_LOG(msg) do{\
	fprintf(stderr, "%d %s ", __LINE__, __func__);\
	perror(msg);\
}while(0)

#define PATH "./dict.txt"

#define N 256
#define M 128

#define R 	1 		//注册
#define L 	2 		//登录
#define Q 	3 		//退出

#define S 	4 		//查词
#define H 	5 		//历史记录
#define T 	6 		//管理员注册
#define P	7 		//管理员登入
#define A 	8 		//增
#define D 	9 		//删
#define G 	10 		//改
#define C 	11 		//查
#define Y 	12 		//用户修改
typedef struct
{
	int fd ;
	struct sockaddr_in cin;
	sqlite3* usr_db;
	sqlite3* dict_db;
}CliCnt;

struct msg
{
	int type;
	char name[20];
	char text[M];
	char age_name[20];
	char age_phone[20];
	char age_add[20];
	int age_salary;
	int age_age;
	
};

int init_sqlite(sqlite3** , sqlite3** );
int dictToDatabase(sqlite3* );

int init_socket(int* , int  , const char* );

void* handle_cli_msg(void* );

//注册
int do_register(int , struct msg , sqlite3* );
int ad_register(int cntfd, struct msg recv_msg, sqlite3* usr_db);//管理员注册
int do_login(int , struct msg , sqlite3* );
int ad_login(int cntfd, struct msg recv_msg, sqlite3* usr_db); //管理员登入
int do_quit(int , struct msg ,sqlite3* );

int do_insert(int cntfd, struct msg recv_msg,sqlite3* dict_db);  //增
int ad_delete(int cntfd,struct msg recv_msg,sqlite3* dict_db); //删
int do_updata(int cntfd, struct msg recv_msg,sqlite3* dict_db); //改
int do_select(int cntfd, struct msg recv_msg,sqlite3* dict_db);  //查
int pe_updata(int cntfd, struct msg recv_msg,sqlite3* dict_db); //用户改
int main(int argc, const char *argv[])
{
	if(argc < 3)
	{
		fprintf(stderr, "请输入ip 端口号\n");
		return -1;
	}

	/*数据库初始化*/
	sqlite3* dict_db = NULL;
	sqlite3* usr_db = NULL;
	if(init_sqlite(&dict_db, &usr_db)<0)
	{
		return -1;
	}

	/*网络初始化*/
	int sfd = 0;
	if(init_socket(&sfd, atoi(argv[2]), argv[1]) < 0)
	{
		return -1;
	}

	//并发服务器模型---使用多线程
	//主线程管理连接,分支线程管理通信
	int newfd = 0;
	struct sockaddr_in cin;
	socklen_t clen = sizeof(cin);
	while(1)
	{
		newfd = accept(sfd, (void*)&cin, &clen);
		if(newfd < 0)
		{
			ERR_LOG("accept");
			return -1;
		}
		printf("----[%s:%d]连接成功----\n", (char*)inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));

		//连接成功,创建线程维护客户端
		CliCnt cli_msg = {newfd, cin, usr_db, dict_db};
		pthread_t tid;
		if(pthread_create(&tid, NULL, handle_cli_msg, &cli_msg) !=0)
		{
			ERR_LOG("pthread_create");
			return -1;
		}

	}
	
	return 0;
}

//服务器与客户端交互的函数
void* handle_cli_msg(void* arg)
{
	//线程分离
	pthread_detach(pthread_self());

	CliCnt cli_msg = *(CliCnt*)arg;
	int cntfd = cli_msg.fd;
	struct sockaddr_in cin = cli_msg.cin;
	sqlite3* usr_db = cli_msg.usr_db;
	sqlite3* dict_db = cli_msg.dict_db; 

	int recv_len;
	struct msg recv_msg;

	while(1)
	{
		recv_len = recv(cntfd, &recv_msg, sizeof(recv_msg), 0);
		if(recv_len < 0)
		{
			ERR_LOG("recv_len");
			break;
		}
		else if(0 == recv_len)
		{
			printf("----[%s:%d]断开连接----\n", (char*)inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));
			break;
		}

		int type = ntohl(recv_msg.type);

		switch(type)
		{
			case R:
				do_register(cntfd, recv_msg, usr_db);
				break;
			case T:
				ad_register(cntfd, recv_msg, usr_db);  //管理员注册
				break;
			case L:
				do_login(cntfd, recv_msg, usr_db);
				break;
			case P:
				ad_login(cntfd, recv_msg, usr_db); //管理员登入
				break;
			case Q:
				do_quit(cntfd, recv_msg, usr_db);
				break;
			case A:
				do_insert(cntfd, recv_msg, dict_db);
				break;
			case D:
				ad_delete(cntfd, recv_msg, dict_db); //删
				break;
			case G:
				do_updata(cntfd, recv_msg, dict_db); //改
				break;
			case C:
				do_select(cntfd, recv_msg, dict_db);   //查
				break;
			case Y:
				pe_updata(cntfd, recv_msg, dict_db);   //用户改
				break;
		}
	}

	close(cntfd);
	pthread_exit(NULL);
}



int do_updata(int cntfd, struct msg recv_msg,sqlite3* dict_db) //改
{

	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql,"update dict set salary=%d where name=%s",recv_msg.age_salary,recv_msg.age_name);
	if(sqlite3_exec(dict_db,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
	printf("修改数据成功\n");
	return 0;
}



int pe_updata(int cntfd, struct msg recv_msg,sqlite3* dict_db) //用户改
{

	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql,"update dict set salary=%d where name=%s",recv_msg.age_salary,recv_msg.age_name);
	if(sqlite3_exec(dict_db,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
	printf("修改数据成功\n");
	return 0;
}

int ad_delete(int cntfd, struct msg recv_msg,sqlite3* dict_db) //删
{
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql,"delete from dict where name=(%s)",recv_msg.age_name);
	//调用sqlite3_exec函数
	if(sqlite3_exec(dict_db,sql,NULL,NULL,&errmsg) != 0)
	{
		printf("sqlite3_exec:%s __%d__\n",errmsg,__LINE__);
		return -1;
	}
	printf("删除数据成功\n");
	return 0;
}


int do_insert(int cntfd, struct msg recv_msg,sqlite3* dict_db)  //增
{
	char* errmsg = NULL;
	char sql[N] = "";
	bzero(sql, sizeof(sql));
	sprintf(sql, "insert into dict values(\"%s\", \"%s\", \"%s\", \"%d\" ,\"%d\")", \
			recv_msg.age_name,recv_msg.age_phone,recv_msg.age_add,recv_msg.age_salary,recv_msg.age_age);
	if(sqlite3_exec(dict_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	printf("插入历史记录成功\n");
	return 0;
}

//客户端退出
int do_quit(int cntfd, struct msg recv_msg, sqlite3* usr_db)
{
	//客户端退出,修改用户登录状态
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "update usr set stage = 0 where name=\"%s\";", recv_msg.name);
	char** pres = NULL;
	int row, column;
	if(sqlite3_get_table(usr_db, sql, &pres, &row, &column,  &errmsg) != 0)
	{
		//查找失败
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}

	printf("修改用户登录状态成功 __%d__\n", __LINE__);
	printf("用户退出成功 __%d__\n", __LINE__);
	return 0;
}


int do_select(int cntfd, struct msg recv_msg,sqlite3* dict_db)  //查
{
	//存储要查询的单词,以及返回的意思
	char na[20] = "";       //名字
	char ph[20] = "";      //手机
	char add[20] = "";       //地址
	char salary[20] ="";         //工资
	char age[20]="";      	     //年龄

	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "select name,phone,adda,salary,age from dict where name=\"%s\";", recv_msg.age_name);
	printf("sql = %s %d\n", sql, __LINE__);

	char** pres = NULL;
	int row, column;
	if(sqlite3_get_table(dict_db, sql, &pres, &row, &column,  &errmsg) != 0)
	{
		//查找失败
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}

	//如果没有找到相应的单词的意思,发送not FOUND
	//如果找到了发送单词相应的意思
	if(0 == row)
	{
		strcpy(na, "NOT FOUND!!");
	}
	else
	{
		strcpy(na, pres[5]);
		strcpy(ph, pres[6]);
		strcpy(add, pres[7]);
		strcpy(salary, pres[8]);
		strcpy(age, pres[9]);
	
	}

	//拼接: 单词:意思,
	sprintf(recv_msg.text, "%s %s %s %s %s", na,ph,add,salary,age);
	if(send(cntfd, &recv_msg, sizeof(recv_msg), 0) <0)
	{
		ERR_LOG("send");
		return -1;
	}


	return 0;
}



//登录
int do_login(int cntfd, struct msg recv_msg, sqlite3* usr_db)
{
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "select * from usr where name=\"%s\" and passwd=\"%s\";", recv_msg.name, recv_msg.text);

	char** pres = NULL;
	int row, column;
	if(sqlite3_get_table(usr_db, sql, &pres, &row, &column,  &errmsg) != 0)
	{
		//查找失败,返回用户未注册
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	if(0 == row)
	{
		strcpy(recv_msg.text, "**NOTEXISTS**");
	}
	else
	{
		//判断状态是否是已经登录状态
		if(strcmp(pres[(row+1)*column-1], "0") == 0) 	//未登录状态
		{
			strcpy(recv_msg.text, "**OK**");
			//将状态设置位已经登录状态 stage =1;
			bzero(sql, sizeof(sql));
			sprintf(sql, "update usr set stage = 1 where name=\"%s\" ;", recv_msg.name);
			printf("sql = %s __%d__\n", sql, __LINE__);
			if(sqlite3_exec(usr_db, sql, NULL, NULL, &errmsg) != 0)
			{
				printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
				return -1;
			}
		}
		else
		{
			strcpy(recv_msg.text, "**EXISTS**"); 	//已登录状态
		}
	}

	if(send(cntfd, &recv_msg, sizeof(recv_msg), 0) <0)
	{
		ERR_LOG("send");
		return -1;
	}

	return 0;
}


//管理员登录
int ad_login(int cntfd, struct msg recv_msg, sqlite3* usr_db)
{
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "select * from Administrator where name=\"%s\" and passwd=\"%s\";", recv_msg.name, recv_msg.text);

	char** pres = NULL;
	int row, column;
	if(sqlite3_get_table(usr_db, sql, &pres, &row, &column,  &errmsg) != 0)
	{
		//查找失败,返回用户未注册
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	if(0 == row)
	{
		strcpy(recv_msg.text, "**NOTEXISTS**");
	}
	else
	{
		//判断状态是否是已经登录状态
		if(strcmp(pres[(row+1)*column-1], "0") == 0) 	//未登录状态
		{
			strcpy(recv_msg.text, "**OK**");
			//将状态设置位已经登录状态 stage =1;
			bzero(sql, sizeof(sql));
			sprintf(sql, "update usr set stage = 1 where name=\"%s\" ;", recv_msg.name);
			printf("sql = %s __%d__\n", sql, __LINE__);
			if(sqlite3_exec(usr_db, sql, NULL, NULL, &errmsg) != 0)
			{
				printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
				return -1;
			}
		}
		else
		{
			strcpy(recv_msg.text, "**EXISTS**"); 	//已登录状态
		}
	}

	if(send(cntfd, &recv_msg, sizeof(recv_msg), 0) <0)
	{
		ERR_LOG("send");
		return -1;
	}

	return 0;
}

//注册
int do_register(int cntfd, struct msg recv_msg, sqlite3* usr_db)
{
	int ret_value = 0;
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "insert into usr values(\"%s\", \"%s\" , 0);", recv_msg.name, recv_msg.text);
	printf("%s\n", sql);

	if(sqlite3_exec(usr_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		if(strstr(errmsg, "UNIQUE constraint failed"))
		//if(strcmp("column name is not unique", errmsg) != NULL)
		{
			strcpy(recv_msg.text, "**EXISTS**");
		}
		else
		{
			strcpy(recv_msg.text, "**ERROR**");
		}
		ret_value = -1;
	}
	else
	{
		printf("注册成功\n");
		strcpy(recv_msg.text, "**OK**");
	}

	if(send(cntfd, &recv_msg, sizeof(recv_msg), 0) <0)
	{
		ERR_LOG("send");
		return -1;
	}

	return ret_value;
}


//管理员注册
int ad_register(int cntfd, struct msg recv_msg, sqlite3* usr_db)
{
	int ret_value = 0;
	char* errmsg = NULL;
	char sql[N] = "";
	sprintf(sql, "insert into Administrator values(\"%s\", \"%s\" , 0);", recv_msg.name, recv_msg.text);
	printf("%s\n", sql);

	if(sqlite3_exec(usr_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		if(strstr(errmsg, "UNIQUE constraint failed"))
		//if(strcmp("column name is not unique", errmsg) != NULL)
		{
			strcpy(recv_msg.text, "**EXISTS**");
		}
		else
		{
			strcpy(recv_msg.text, "**ERROR**");
		}
		ret_value = -1;
	}
	else
	{
		printf("注册成功\n");
		strcpy(recv_msg.text, "**OK**");
	}

	if(send(cntfd, &recv_msg, sizeof(recv_msg), 0) <0)
	{
		ERR_LOG("send");
		return -1;
	}

	return ret_value;
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
	int values = 1;
	if(setsockopt(*psfd, SOL_SOCKET, SO_REUSEADDR, &values, sizeof(int))<0)
	{
		ERR_LOG("setsockopt");
		return -1;
	}


	//bind
	struct sockaddr_in sin;
	sin.sin_family 		= AF_INET;
	sin.sin_port  		= htons(port);
	sin.sin_addr.s_addr = inet_addr(ip);

	if(bind(*psfd, (void*)&sin, sizeof(sin)) < 0)
	{
		ERR_LOG("bind");
		return -1;
	}

	//listen
	if(listen(*psfd, 5) < 0)
	{
		ERR_LOG("listen");
		return -1;
	}
	
	return 0;
}



//数据库初始化
int init_sqlite(sqlite3** pdict_db, sqlite3** pusr_db)
{
	char sql[N] = "";
	char* errmsg = NULL;
	//创建打开dict.db数据库
	//如果dict表格存在,则不导入
	//如果dict表格不存在,则创建表格,并将dict.txt导入数据库
	if(sqlite3_open("./dict.db", pdict_db)!=0)
	{
		printf("sqlite3_open:%s %d\n", sqlite3_errmsg(*pdict_db), __LINE__);
		return -1;
	}
	
	sprintf(sql, "create table dict (name char primary key, phone char, adda char, salary int, age int);");
	if((sqlite3_exec(*pdict_db, sql, NULL, NULL, &errmsg)) != 0)
	{
		if(strcmp("table dict already exists", errmsg) == 0)
		{
			printf("table dict already exists\n");
		}
		else
		{
			printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
			return -1;
		}
	}

	printf("员工管理信息库打开成功\n");


	//创建打开 usr.db数据库 
	//创建历史记录表
	//创建用户注册表格
	//清空用户登录状态 0为不在线 1为在线
	if(sqlite3_open("./usr.db", pusr_db)!=0)
	{
		printf("sqlite3_open:%s %d\n", sqlite3_errmsg(*pusr_db), __LINE__);
		return -1;
	}

	bzero(sql, sizeof(sql));
	sprintf(sql,"create table if not exists Administrator(name char primary key, passwd char, stage int);");
	if(sqlite3_exec(*pusr_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	printf("用户数据表格打开成功\n");


	bzero(sql, sizeof(sql));
	sprintf(sql,"create table if not exists usr(name char primary key, passwd char, stage int);");
	if(sqlite3_exec(*pusr_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	printf("用户数据表格打开成功\n");


	bzero(sql, sizeof(sql));
	sprintf(sql,"update usr set stage=0;");
	if(sqlite3_exec(*pusr_db, sql, NULL, NULL, &errmsg) != 0)
	{
		printf("sqlite3_exec:%s %d\n", errmsg, __LINE__);
		return -1;
	}
	printf("用户状态清空成功\n");
	return 0;
}



