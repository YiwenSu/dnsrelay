#include <iostream>
#include <string>
#include <WinSock2.h>
#include <windows.h>
#include <fstream>
#include <vector>
#include<stdio.h>
#include <time.h>
#include<winbase.h>
#pragma  comment(lib, "Ws2_32.lib")
#include <unistd.h> //sleep();
#include <pthread.h>
#include <stdlib.h>
using namespace std;
#define table_size    210//909//209
#define PORT 53
#define OUT_ADDRESS  "202.106.0.20"//外部DNS服务器地址
int count = 0;

//dns域名解析表
class translate
{
    public:
	string ip;
	string domain;
};


//请求包ID转换
class IDtrans
{
    public:
	unsigned short id;
	bool finish;
	time_t start;
	string name;
	SOCKADDR_IN client; //请求者套接字地址
};


 translate DNStable[table_size];
vector <IDtrans> tr;
string url;
int order;
pthread_mutex_t mutex;
void* thread_1(void* args)
{
    time_t current;
    while (1)
        {
              pthread_mutex_lock (&mutex);
               current = time(NULL);
                for(int j = 0; j<tr.size();j++)
                {
                    if(difftime(current,tr[j].start) > 3 && tr[j].finish == false)
                    {
                        unsigned short current_id =htons(tr[j].id) ;
                        //create_response(sendbuf,recbuf,rnum, "notexist",length);
                        tr[j].finish = true;
                         //snum=sendto(host,sendbuf,rnum,0,(SOCKADDR*)&tr[j].client,sizeof(tr[j].client));
                         cout<<"***********************"<<tr[j].name<<"  timeout"<<endl;
                    }
                }
            pthread_mutex_unlock(&mutex);
            //cout<<"爸爸还活着"<<endl;
           //Sleep(200);
    }

}


//分割字符串
void split(const string& s, vector<string>& v, const string& c)
{
	string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	//cout << pos2 << "!!!!! " << endl;
	while (string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));
		//cout << s.substr(pos1, pos2 - pos1) << endl;
		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
	//cout << s.substr(pos1) << endl;

}

//获得ip-URL对照表
int get_table(char* Path)
{
	ifstream in(Path, ios::in);

	if (!in) {
		cout << endl << "文件不存在" << endl;
		exit(1);
	}

	string temp[table_size];
    int i;
	for (i =0; !in.eof(); i++)//非文件尾部继续读
            getline(in, temp[i]);//遇到换行符读一行。


	string s2(" ");

	for (int j = 0; j < i; j++)
	{
		vector<string> a;
		split(temp[j], a, s2);
		DNStable[j].ip = a[0];
		DNStable[j].domain = a[1];
		cout << DNStable[j].ip << " " << DNStable[j].domain << endl;
	}

	return 0;
}
//通过计数字节获取URL,
int get_url(string rec,int num)
{
     string temp;
     url="";
     //cout<<rec;
     temp=rec.substr(12, num-16);
    //cout<<"lalalllllll"<<endl;
     int lenth=0;//url长度
    for(int i=0;i<temp.size()-1;)
    {
        if(temp[i]>0&&temp[i]<=63)
        {
            int x=temp[i];//计数字节告诉我们后面有多少个字，获取后续的字节到url中
            i++;
            while(x>0)
            {
                url+=temp[i];
                i++;
                x--;
                lenth++;
            }
        }
        if(temp[i]>0)
        {
            url+=".";
            lenth++;
        }
    }

	url+='\0';

	return lenth;//返回url长度
}


string find_url()//在本地dnsrelay找url
{
      int i=0;
      //cout<<url.c_str();

      char a[100];
      strcpy(a,DNStable[i].domain.c_str());
      while(i<table_size-1)
      {
         if(url.substr(0,url.size()-1).compare(DNStable[i].domain)==0)
           {
             //cout<<"mqqq:"<<i<<endl;
              //cout<<DNStable[i].ip;
               return DNStable[i].ip;
               break;
           }
        i++;
      }
      return "notexist";//没找到则返回notexist
}

int create_response(char* sendbuf,char* recbuf, int rnum,string findout, int &length) //创建本地回复包
{
    unsigned short flag=htons(0x8180);


    memcpy(sendbuf,recbuf,rnum);
    memcpy(sendbuf+2,&flag,2);//ID不变，变标志位
    unsigned short send;
   // int length;
    if(findout=="0.0.0.0")
    {
        //屏蔽
        send=htons(0x0000);//回答数=0
        memcpy(sendbuf+6,&send,2);
        length=rnum;
       return 0;
    }
    else
    {
        //不屏蔽
        send=htons(0x0001);
        memcpy(sendbuf+6,&send,2);
        length=rnum;
        //构造响应
        send=htons(0xc00c);//NAME域
        memcpy(sendbuf+length,&send,2);
        length+=2;
        send=htons(0x0001);//TYPE A
        memcpy(sendbuf+length,&send,2);
        length+=2;
        send=htons(0x0001);//CLASS A
        memcpy(sendbuf+length,&send,2);
        length+=2;
        send=htons(0x2A300);//生存时间2天
        memcpy(sendbuf+length,&send,4);
        length+=4;
        send=htons(0x0004);//资源数据长度
        memcpy(sendbuf+length,&send,2);
        length+=2;
        unsigned long ip_data;//资源数据
        ip_data=(unsigned long)inet_addr(findout.c_str());
       memcpy(sendbuf+length,& ip_data,findout.size());
       length+=findout.size();
       return 1;
    }
}

unsigned short get_new_id (unsigned short old_id, SOCKADDR_IN addr, bool ifdone,string name)
{
	IDtrans temp;
	//time_t time;
	temp.id=old_id;
	temp.client=addr;
	temp.finish=ifdone;
	time(&temp.start);
	temp.name = name;
    pthread_mutex_lock (&mutex);
	tr.push_back(temp);
	pthread_mutex_unlock(&mutex);
	return (unsigned short)(tr.size() - 1);	//以表中下标作为新的ID
}

 unsigned short  create_outside(char* recbuf, SOCKADDR_IN clientname)
{
    unsigned short *old_id = new unsigned short;//因为memcpy要地址
    memcpy(old_id, recbuf, sizeof(unsigned short));
    unsigned short newid = htons( get_new_id(ntohs(*old_id), clientname, false,url));
    memcpy(recbuf, &newid, 2);
    free(old_id);
    return newid;//
}


void Display2(string  findout)
{
	//打印时间
	SYSTEMTIME time;
    GetLocalTime(&time);
	string str;
	if (findout == "notexist")
	{
		//中继功能
		cout<<order<<":"<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<"  "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<<"  "<<"127.0.0.1"<<"  "<<"中继"<<url<<endl;
	}

	//在表中找到DNS请求中的域名
	else
        {
	    if( findout == "0.0.0.0")  //不良网站拦截
		    cout<<order<<":"<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<"  "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<<"  "<<"127.0.0.1"<<" "<<"屏蔽"<<url<<endl;
		else
		    cout<<order<<":*"<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<"  "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<<"  "<<"127.0.0.1"<<" "<<"服务器"<<url<<"  "<< findout<<endl;
	}
}

 string Hex2String(char *pszHex, int nLen)
{
    if (NULL == pszHex || 0 == nLen)
    {
        return "";
    }

    string strRet = "";
    for (int i = 0; i < nLen; i++)
    {
        char szTmp[4] = {0};
        sprintf(szTmp, "%02x", (byte)pszHex[i]);
        strRet += szTmp;
        strRet += "  ";
    }

    return strRet;
}
int main( int argc, char** argv)
{
    WSADATA wsaData;//初始化动态链接库用的，之后socket才能用
    SOCKET server, host;//本地DNS和外部DNS两个套接字
    SOCKADDR_IN servername ,hostname,clientname;//本地DNS、外部DNS、请求端三网络套接字地址
	char sendbuf[512];//发缓存
    char recbuf[512];//收缓存
    char out_add[16];//外部DNS地址
    int clientsize, rnum,snum,rnum1;
    int dlength;
    string findout;
    int id_count=0;
   int length;
    //string str= OUT_ADDRESS;

   //string path;
   char path[100];
   int flag=0;
   order=1;
   struct timeval timeout;
    //strcpy(out_add,str.c_str());

    if(argc == 1)
    {
        cout<< "无调试信息输出"<<endl;
        strcpy(out_add, "202.106.0.20");
        //path="dnsrelay.txt";
        strcpy(path,"dnsrelay.txt");
        flag=1;
    }
    else if(argc == 3)
	{
	   cout<<"调试信息级别2"<<endl;
        strcpy(out_add, argv[2]);
	  //path="dnsrelay.txt";
	   strcpy(path,"dnsrelay.txt");
	  flag =3;
	}
else if(argc == 4)
  {
      cout<<"调试信息级别1"<<endl;
        strcpy(out_add, "202.106.0.20");
         strcpy(path,argv[3]);
        flag=2;
  }

    WSAStartup(MAKEWORD(2,2), &wsaData);

    server=socket(AF_INET,SOCK_DGRAM,0);//UDP（非TCP/IP）,数据报（非流），通信协议
	host=socket(AF_INET,SOCK_DGRAM,0);

	hostname.sin_family = AF_INET;//指定地址族，表示是UDP协议族的套接字；
	hostname.sin_port = htons(PORT);//指明端口号
	hostname.sin_addr.s_addr = inet_addr("127.0.0.1");//sin_addr：用inet_addr()把字符串形式的IP地址转换成unsigned long型的整数值后再置给s_addr，当一个应用程序调用WSAStartup函数时，操作系统根据请求的Socket版本来搜索相应的Socket库，然后绑定找到的Socket库到该应用程序中。以后应用程序就可以调用所请求的Socket库中的其它Socket函数了。该函数执行成功后返回0。

	servername.sin_family = AF_INET;
	servername.sin_port = htons(PORT);
	servername.sin_addr.s_addr = inet_addr(out_add);//OUT_ADDRESS自己define的



	if(bind(host,(SOCKADDR*)&hostname,sizeof(hostname)))
	{
        cout<<"滚蛋"<<endl;
		exit(1);
	}
	else
        cout<<"竟然成功了！"<<endl;
    get_table(path);
    time_t t;
   SYSTEMTIME time1;
   GetLocalTime(&time1);
	fd_set fdset;//套接字集合

   int fd;
   if(server > host)
       fd = server +1;
   else
       fd = host + 1;
   int Flage=0;
   pthread_t time_out;
   pthread_mutex_init (&mutex,NULL);
   pthread_create(&time_out,NULL,thread_1,NULL);
	while(1)
    {


		FD_ZERO(&fdset);//清空
		FD_SET(server,&fdset);
		FD_SET(host,&fdset);


		int ret;
		ret = select(0,&fdset,NULL,NULL,NULL);

       clientsize=sizeof(clientname);
       memset(recbuf,0,512) ;
    switch (ret)
    {
    case 0:
          /* if(!FD_ISSET(host, &fdset))
                  cout<<"timeout host"<<endl;
             if(!FD_ISSET(server, &fdset))
                  cout<<"timeout server"<<endl;*/
            continue;
           break;
    case -1:
          cout<<"error select"<<endl;
           break;
    default:
    if(FD_ISSET(host, &fdset))
       {
            //处理请求包

           rnum=recvfrom(host,recbuf,512,0,(SOCKADDR*)&clientname,&clientsize);


           if(rnum==0)
            {
                cout<<"connect over!"<<endl;
                exit(0);
            }
            else if(rnum==-1)
            {
               //cout<<"receive error!"<< WSAGetLastError() <<"OOOOOOOOOO"<<endl;
            }
    else
    {
        string recstr;
        for(int i=0;i<rnum;i++)
            recstr+=recbuf[i];
        unsigned short *ipv=new unsigned short;
		 memcpy(ipv,&recstr[rnum-4],2);
         get_url(recstr,rnum);
        findout=find_url();
         if(ntohs(*ipv)!=0x0001)
		 {
			// cout<<"不是ipv4"<<endl;
		   findout="notexist";
		 }

        if(findout=="notexist")
        {
                               //去外部dns找
                    cout<<"向外部服务器询问"<<endl;
                    unsigned short *old_id = new unsigned short;
                    memcpy(old_id, recbuf, sizeof(unsigned short));
                    unsigned short nid = create_outside(recbuf, clientname);//

                   if(flag==2)
                        {
                        Display2(findout);
                        order++;
                        }

                   if(flag == 3){
                    cout<<"-------------------------------------------------------------------------------"<<endl;
                    Display2(findout);
                    order++;
                    cout<<"中继SEND"<<ntohs(*old_id)<<" TO "<<nid<<endl<<Hex2String(recbuf,rnum)<<endl;
                    }

                    // cout<<"PPPPP"<<endl;
                    snum=sendto(server,recbuf,rnum,0,(SOCKADDR*)&servername,sizeof(servername));
                    //snum=sendto(server,recbuf,rnum,0,(SOCKADDR*)&hostname,sizeof(hostname));
                    cout<<"snum:"<<snum<<endl;
                    if(snum==SOCKET_ERROR)
                        cout<<"send error!"<<endl;
                    else if( snum==0)
                        cout<<"connect over001"<<endl;
                    //else
                   // cout<<"send 成功!"<<endl;
                   else
                       Flage ++;

                  }
                  else
                    {
                        //找到则自己构造response
                        count++;
                        unsigned short *old_id = new unsigned short;
                        if(flag==2)
                        {
                        Display2(findout);
                        order++;
                        }
                        if(flag ==3){
                        cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;

                        Display2(findout);
                        order++;
                        memcpy(old_id, recbuf, sizeof(unsigned short));
                        cout<<"服务器SEND"<<ntohs(*old_id)<<endl<<Hex2String(recbuf,rnum)<<endl;}

                        int length=0;
                        int  flags = create_response(sendbuf,recbuf,rnum,findout,length);

                        snum=sendto(host,sendbuf,length,0,(SOCKADDR*)&clientname,sizeof(clientname));
                        if(flag ==3){
                        cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
                         cout<<"服务器RECV"<<ntohs(*old_id)<<endl<<Hex2String(sendbuf,snum)<<endl;}
                        if(snum==SOCKET_ERROR)
                            cout<<"send error!"<<endl;
                        else if( snum==0)
                            cout<<"connect over002"<<endl;
                    }
            }

       }
    if(FD_ISSET(server, &fdset))
       {
           count++;
           //接收来自外部DNS服务器的响应报文
            int serversize=sizeof(servername);
         rnum=recvfrom(server,recbuf,512,0,(SOCKADDR*)&servername,&serversize);
            if(rnum==0)
               {
                   cout<<"connect over!"<<endl;
                   exit(0);
               }
               else if(rnum==-1)
               {
                  // cout<<"receive error!"<< WSAGetLastError() <<"KKKKKKKKKKK"<<endl;
               }
               else
               {
                   cout<<"从外部得到回应包"<<endl;

			       //发给本地
                 unsigned short  gid;
                 memcpy(&gid, recbuf,2);
                 int i = ntohs(gid);//转换成计算机格式
                 unsigned short old_id =htons(tr[i].id) ;

                 memcpy(recbuf, &old_id, sizeof(unsigned short));
                  tr[i].finish=true;
                  snum=sendto(host,recbuf,rnum,0,(SOCKADDR*)&tr[i].client,sizeof(tr[i].client));

                  if(flag == 3){
                   cout<<"**************************************************************"<<endl;
                   cout<<time1.wYear<<"-"<<time1.wMonth<<"-"<<time1.wDay<<"  "<<time1.wHour<<":"<<time1.wMinute<<":"<<time1.wSecond<<"  "<<url<<endl;
                  cout<<"中继RECV"<<gid<<" TO "<<ntohs(old_id)<<endl<<Hex2String(recbuf,rnum)<<endl;
                  }
                  Flage --;
			       if (snum== SOCKET_ERROR)
		         	{
				        cout << "sendto Failed: " << WSAGetLastError() <<"DDDDDDDDDDD"<< endl;
				       continue;
			         }
                  else if (snum == 0)
			       {
				      cout<<"CONNECT OVER"<<endl;
				     continue;
			      }

               }

        }
        break;
    }
    cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~:"<<count<<endl;
   }
   closesocket(server);	//关闭套接字
	closesocket(host);
    WSACleanup();				//释放ws2_32.dll动态链接库初始化时分配的资源

	system("pause");
    return 0;
       }
