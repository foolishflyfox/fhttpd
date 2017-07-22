//启动http服务器命令：  ./fhttpd 端口号
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>

#define MAX_CLIENT_COUNT 10
#define RECV_BUF_SIZE 512
#define SEND_BUF_SIZE 512

// 浏览器的请求类型
enum RequestType{REQUEST_GET, REQUEST_POST, REQUEST_UNDEFINED};

//在本机启动tcp服务
int startTcpServer(int portNum){
    // 创建套接字 socket
    int httpdSocket = socket(AF_INET,SOCK_STREAM,0);
    if(httpdSocket==-1){
        fprintf(stderr,"Error: can't create socket,errno is %d\n",errno);
        exit(1);
    }

    //绑定端口 bind
    struct sockaddr_in tcpServerSockAddr;
    memset(&tcpServerSockAddr, 0, sizeof(tcpServerSockAddr));
    tcpServerSockAddr.sin_family = AF_INET;
    tcpServerSockAddr.sin_port = htons(portNum);
    //地址0.0.0.0 表示本机
    tcpServerSockAddr.sin_addr.s_addr = 0;
    if(bind(httpdSocket,(const struct sockaddr*)&tcpServerSockAddr,
        sizeof(tcpServerSockAddr))==-1){
        fprintf(stderr, "Error: can't bind port %d,errno is %d\n",portNum,errno);
        exit(1);
    }

    //侦听 listen
    if(listen(httpdSocket,MAX_CLIENT_COUNT)==-1){
        fprintf(stderr,"Error: can't listen port %d,error is %d\n",portNum,errno);
        exit(1);
    }

    return httpdSocket;
}

// 功能: 从网络套接字socketFd读取一行到buf中，buf大小为bufLength,每行以\r\n结尾
// 返回: 读取的一行的字节数
// 注意: 每一行 行尾的 \r\n 在存入buf中时,只保留 \n
int getOneLineFromSocket(int socketFd,char* buf,int bufLength){
    int byteCount = 0;
    char tmpChar;
    memset(buf, 0, bufLength);
    while(read(socketFd,&tmpChar,1) && byteCount<bufLength){
        if(tmpChar=='\r'){
            if(recv(socketFd,&tmpChar,1,MSG_PEEK)==-1){
                fprintf(stderr, "Error: fail to recv char after \\r\n");
                exit(1);
            }
            //如果 \r后面紧跟\n，表示一行结束
            if(tmpChar=='\n' && byteCount<bufLength){
                read(socketFd,&tmpChar,1);
                buf[byteCount++] = '\n';
                break;
            }
            buf[byteCount++] = '\r';
        }else{
            buf[byteCount++] = tmpChar;
        }
    }
    return byteCount;
}

//将指定字符串通过套接字发送出去
ssize_t socketSendMsg(int socketFd, const char* msg){
    return write(socketFd, msg, strlen(msg));
}

//向客户端发送静态文件
void responseStaticFile(int socketFd, int returnNum, char* filePath,
    char* contentType)
{
    char sendBuf[SEND_BUF_SIZE] = {0};

    if(strcmp(filePath,"./")==0){
        // 没有指定访问文件的，直接访问index.html
        filePath = "./index.html";
    }

    if(contentType==NULL){
        int tpFilePath = strlen(filePath)-1;
        while(tpFilePath>0){
            if(filePath[tpFilePath]!='.'){
                --tpFilePath;
            }else{
                break;
            }
        }
        if(tpFilePath){
            if(strcmp(filePath+tpFilePath+1,"html")==0){
                contentType = "text/html";
            }else if(strcmp(filePath+tpFilePath+1,"txt")==0){
                contentType = "text/plain";
            }else if(strcmp(filePath+tpFilePath+1,"css")==0){
                contentType = "text/css";
            }else if(strcmp(filePath+tpFilePath+1,"js")==0){
                contentType = "text/javascript";
            }else if(strcmp(filePath+tpFilePath+1,"ico")==0){
                //注：如果第一次打开每个网站，会向该网站请求 favicon.ico文件
                // 如果请求到了, 后续的访问将不会再请求该ico文件，否则，每次请求的同时
                // 也会发送对ico文件的请求，直到得到 favicon.ico文件
                contentType = "image/x-icon";
            }else if(strcmp(filePath+tpFilePath+1,"png")==0){
                contentType = "image/png";
            }else if(strcmp(filePath+tpFilePath+1,"gif")==0){
                contentType = "image/gif";
            }else if(strcmp(filePath+tpFilePath+1,"jpeg")==0){
                contentType = "image/jpeg";
            }else if(strcmp(filePath+tpFilePath+1,"bmp")==0){
                contentType = "image/bmp";
            }else if(strcmp(filePath+tpFilePath+1,"webp")==0){
                contentType = "image/webp";
            }else if(strcmp(filePath+tpFilePath+1,"svg")==0){
                contentType = "image/svg+xml";
            }else if(strcmp(filePath+tpFilePath+1,"wav")==0){
                contentType = "audio/wav";
            }else if(strcmp(filePath+tpFilePath+1,"pdf")==0){
                contentType = "application/pdf";
            }
        }
    }

    FILE* pFile = fopen(filePath, "r");
    printf("%d %p : %s\n",returnNum,pFile,filePath);

    if(pFile==NULL){
        returnNum = 404;
        filePath = "./err404.html";
        contentType = "text/html";
        pFile = fopen(filePath,"r");
    }
    switch(returnNum){
        case 200:
            sprintf(sendBuf,"HTTP/1.0 200 OK\r\n");
            break;
        case 400:
            sprintf(sendBuf,"HTTP/1.0 400 BAD REQUEST\r\n");
            break;
        case 404:
            sprintf(sendBuf,"HTTP/1.0 404 NOT FOUND\r\n");
            break;
        case 501:
            sprintf(sendBuf,"HTTP/1.0 501 Method Not Implemented\r\n");
            break;
        default:
            sprintf(sendBuf,"HTTP/1.0 %d Undefined Return Number\r\n",returnNum);
            break;
    }
    socketSendMsg(socketFd, sendBuf);

    sprintf(sendBuf, "Content-type: %s\r\n", contentType);
    socketSendMsg(socketFd, sendBuf);

    socketSendMsg(socketFd, "\r\n");
    
    //向浏览器发送文件
    int readDataLen = 0;
    while((readDataLen=fread(sendBuf, 1, SEND_BUF_SIZE, pFile))!=0){
        write(socketFd, sendBuf, readDataLen);
        sendBuf[readDataLen] = 0;
    }

    fclose(pFile);
}

//线程: 接收浏览器端的数据,并返回一个http包
void* responseBrowserRequest(void* ptr){
    int browserSocket = *(int*)ptr;
    char c;
    char recvBuf[RECV_BUF_SIZE+1] = {0};
    int contentLength = 0;
    enum RequestType requestType = REQUEST_UNDEFINED;
    
    // 定义存放请求文件名的内存块
    #define FILE_NAME_LENGTH 128
    char requestFileName[FILE_NAME_LENGTH] = {0};

    // 定义存放查询参数字符串的内存库
    #define QUERY_STRING_LENGTH 128
    char requestQueryString[QUERY_STRING_LENGTH] = {0};

    // 将 http数据包的信息头读完(信息头和正文间以空行分隔)
    while(getOneLineFromSocket(browserSocket,recvBuf,RECV_BUF_SIZE)){
        // printf("%s",recvBuf);
        if(strcmp(recvBuf, "\n")==0){
            break;
        }

        if(requestType==REQUEST_UNDEFINED){
            int pFileName = 0;
            int pQueryString = 0;
            int pRecvBuf = 0;

            if(strncmp(recvBuf,"GET",3)==0){
                // GET 请求
                requestType = REQUEST_GET;
                pRecvBuf = 4;
                
            }else if(strncmp(recvBuf,"POST",4)==0){
                // POST 请求
                requestType = REQUEST_POST;
                pRecvBuf = 5;
            }

            // 获取请求的文件路径 查询参数
            if(pRecvBuf){
                requestFileName[pFileName++] = '.';
                while(pFileName<FILE_NAME_LENGTH && recvBuf[pRecvBuf]
                    && recvBuf[pRecvBuf]!=' ' && recvBuf[pRecvBuf]!='?'){
                    requestFileName[pFileName++] = recvBuf[pRecvBuf++];
                }

                if(pFileName<FILE_NAME_LENGTH && recvBuf[pRecvBuf]=='?'){
                    ++pRecvBuf;
                    while(pQueryString<QUERY_STRING_LENGTH && 
                        recvBuf[pRecvBuf] && recvBuf[pRecvBuf]!=' '){
                        requestQueryString[pQueryString++] = recvBuf[pRecvBuf];
                    }
                }
            }
        }else if(requestType==REQUEST_GET){

        }else if(requestType==REQUEST_POST){
            if(strncmp(recvBuf, "Content-Length:", 15)==0){
                contentLength = atoi(recvBuf+15);
            }
        }
    }

    //如果是REQUEST_GET或REQUEST_UNDEFINED类型，不再读取http正文的内容
    //如果是REQUEST_POST类型,在读取contentLength长度的数据
    if(requestType==REQUEST_POST && contentLength){
        if(contentLength > QUERY_STRING_LENGTH){
            fprintf(stderr, "Query string buffer is smaller than content length\n");
            contentLength = QUERY_STRING_LENGTH;
        }
        read(browserSocket, requestQueryString, contentLength);
    }
    switch(requestType){
        case REQUEST_GET:
            {
                responseStaticFile(browserSocket,200,requestFileName,NULL);
            }
            break;
        case REQUEST_POST:
            break;
        case REQUEST_UNDEFINED:
            {
                responseStaticFile(browserSocket, 501, "./err501.html","text/html");
            }
            break;
        default:
            break;
    }
    close(browserSocket);
    return NULL;
}

int main(int argc,char* argv[]){
    //判断命令是否正确
    if(argc < 2){
        fprintf(stderr,"USAGE: %s portNum\n",argv[0]);
        exit(1);
    }
    int portNum = atoi(argv[1]);

    //限定开启http服务的端口号为1024~65535或者是80
    if((portNum!=80)&&(portNum<1024 || portNum>65535)){
        fprintf(stderr,"Error: portNum range is 1024~65535 or 80\n");
        exit(1);
    }

    int httpdSocket = startTcpServer(portNum);

    while(1){
        struct sockaddr_in browserSocketAddr;
        int browserLen = sizeof(browserSocketAddr);
        int browserSocket = accept(httpdSocket,
            (struct sockaddr*)&browserSocketAddr,&browserLen);
        if(browserSocket==-1){
            fprintf(stderr,"Error: fail to accept, error is %d\n",errno);
            exit(1);
        }
        printf("%s:%d linked !\n",inet_ntoa(browserSocketAddr.sin_addr),
            browserSocketAddr.sin_port);
        pthread_t responseThread;
        //创建线程处理浏览器请求
        int threadReturn = pthread_create(&responseThread,
            NULL,responseBrowserRequest,&browserSocket);
        // 如果pthread_create返回不为0,表示发生错误
        if(threadReturn){
            fprintf(stderr,"Error: fail to create thread, error is %d\n",threadReturn);
            exit(1);
        }
    }

    return 0;
}

