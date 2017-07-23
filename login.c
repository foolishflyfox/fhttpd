#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USERNAME_LEN 30
#define PASSWORD_LEN 30

#define MAX_USER_COUNT 20

struct userItem{
    char userName[USERNAME_LEN];
    char password[PASSWORD_LEN];
};

struct userItem userItemArray[MAX_USER_COUNT];
int userItemCount = 0;

// 用户登陆查询的结果
enum CHECK_USER_RESULT{
    CHECK_NO_USER, // 用户名不存在
    CHECK_WRONG_PASSWORD, // 密码错误 
    CHECK_OK //通过校验
};

// 获得所有的可登录的用户，该程序的可登录用户在代码中定义
// 之后，可根据需要，变为从文件中或数据库中进行读取
void getUserItems(){
    // 第一个用户 用户名:admin 密码:123456
    strcpy(userItemArray[0].userName, "admin");
    strcpy(userItemArray[0].password, "123456");

    // 第二个用户 用户名:guest 密码:guest
    strcpy(userItemArray[1].userName, "guest");
    strcpy(userItemArray[1].password, "guest");

    userItemCount = 2;
}

void checkUser(char* username, char* password){
    enum CHECK_USER_RESULT result;
    printf("%s\n","<!DOCTYPE html>");
    printf("%s\n","<html lang='en'>");
    printf("%s\n","<head>");
    printf("%s\n","<meta charset='UTF-8'>");
    printf("%s\n","<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    printf("%s\n","<meta http-equiv='X-UA-Compatible' content='ie=edge'>");
    printf("%s\n","<title>Login</title>");
    printf("%s\n","</head>");
    printf("%s\n","<body>");
    if(username==NULL){
        result = CHECK_NO_USER;
    }else{
        int i = 0;
        for(; i < userItemCount; ++i){
            if(strcmp(userItemArray[i].userName, username)==0){
                if(strcmp(userItemArray[i].password, password)==0){
                    result = CHECK_OK;
                }else{
                    result = CHECK_WRONG_PASSWORD;
                }
                break;
            }
        }
        if(i==userItemCount){
            result = CHECK_NO_USER;
        }
    }
    switch(result){
        case CHECK_NO_USER:
            printf("<h1 style='color:red;'>用户%s不存在</h1>",username?username:"");
            break;
        case CHECK_WRONG_PASSWORD:
            printf("<h1 style='color:orange'>对不起, %s : 密码错误</h1>", username);
            break;
        case CHECK_OK:
            printf("<h1 style='color:lawngreen'>欢迎使用,%s</h1>",username);
            break;
    }
    printf("%s\n","</body>");
    printf("%s\n","</html>");
}

void main(int argc, char* argv[]){
    getUserItems();
    // argv[1] = "username=amin&password=123456";argc=2;
    if(argc <2){
        checkUser(NULL, NULL);
    }else{
        int queryStringLen = strlen(argv[1]);
        char* queryString = (char*)malloc(queryStringLen+1);
        memcpy(queryString,argv[1],queryStringLen);
        queryString[queryStringLen] = '\0';

        char username[USERNAME_LEN] = {0};
        char password[PASSWORD_LEN] = {0};
        int andIndex = 0;
        while(queryString[andIndex] && queryString[andIndex]!='&')++andIndex;
        queryString[andIndex] = '\0';
        sscanf(queryString,"username=%s",username);
        sscanf(queryString+andIndex+1,"password=%s",password);
        checkUser(username,password);
        free(queryString);
    }
    return;
}


