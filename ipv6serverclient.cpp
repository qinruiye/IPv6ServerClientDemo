#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#define AppVersion "1.02"

typedef enum SocketMode_
{
    SOCKET_MODE_NONE,
    SOCKET_MODE_SERVER,
    SOCKET_MODE_CLIENT
}SocketMode_t;

int tcp_connect(const char *host, const char *service)
{
    int sockfd, ret;
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_IP;

    if (0 != (ret = getaddrinfo(host, service, &hints, &res)))
    {
        cout << "getaddrinfo error: " << gai_strerror(ret) << endl;
        return -1;
    }

    ressave = res;
    while (NULL != res)
    {
        if (-1 == (sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)))
        {
            cout << "create socket error: " << strerror(errno) << endl;
            res = res->ai_next;
            continue;
        }

        if (-1 == connect(sockfd, res->ai_addr, res->ai_addrlen))
        {
            cout << "connect error: " << strerror(errno) << endl;
            close(sockfd);
            
            res = res->ai_next;
            continue;
        }


        printf("connect sucess.\n");
        break;
    }

    freeaddrinfo(ressave);

    if (NULL == res)
        return -1;

    return sockfd;
}

int tcp_listen(const char *host, const char *service, const int listen_num = 5)
{
    int listenfd, ret;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_IP;

    // 服务器端，host可为通配地址[::]
    if (0 != (ret = getaddrinfo(host, service, &hints, &res)))
    {
        cout << "getaddrinfo error: " << gai_strerror(ret) << endl;
        return -1;
    }

    ressave = res;
    while(NULL != res)
    {
        if (-1 == (listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)))
        {
            cout << "create socket error: " << strerror(errno) << endl;
            res = res->ai_next;
            continue;
        }

        if (-1 == setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
        {
            cout << "setsockopt error: " << strerror(errno) << endl;
            close(listenfd);
            res = res->ai_next;
            continue;
        }

        if (-1 == bind(listenfd, res->ai_addr, res->ai_addrlen))
        {
            cout << "bind error: " << strerror(errno) << endl;
            close(listenfd);
            res = res->ai_next;
            continue;
        }

        if (-1 == listen(listenfd, listen_num))
        {
            cout << "listen error: " << strerror(errno) << endl;
            close(listenfd);
            res = res->ai_next;
            continue;
        }

        break;
    }

    freeaddrinfo(ressave);

    if (NULL == res)
        return -1;

    return listenfd;
}

int get_addrinfo(const struct sockaddr *addr, string &ip, in_port_t &port)
{
    void *numeric_addr = NULL;
    char addr_buff[INET6_ADDRSTRLEN];

    if (AF_INET == addr->sa_family)
    {
        numeric_addr = &((struct sockaddr_in*)addr)->sin_addr;
        port = ntohs(((struct sockaddr_in*)addr)->sin_port);
    }
    else if (AF_INET6 == addr->sa_family)
    {
        numeric_addr = &((struct sockaddr_in6*)addr)->sin6_addr;
        port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
    }
    else
    {
        return -1;
    }

    switch (addr->sa_family) {
        case AF_INET:
        {
            printf("IPv4 family\n");
            break;
        }
        case AF_INET6:
        {
            printf("IPv6 family\n");
            break;
        }
        default:
        {
            printf("family:%d\n", addr->sa_family);
            break;
        }
    }
    //printf("family:%d\n", addr->sa_family);
    if (NULL != inet_ntop(addr->sa_family, numeric_addr, addr_buff, sizeof(addr_buff)))
        ip = addr_buff;
    else
        return -1;

    return 0;
}

//./IPv6ServerClientDemo SERVER IPv4 9527         #server mode, listen only on  ipv4
//./IPv6ServerClientDemo SERVER IPv6 9527         #server mode, listen only on  ipv6
//./IPv6ServerClientDemo SERVER [::] 9527         #server mode, listen ipv6 and ipv4
//./IPv6ServerClientDemo SERVER ::1 9527          #server mode, listen ipv6 local address
//./IPv6ServerClientDemo CLIENT [IPv4|IPv6] 9527  #client mode, listen ipv6 or ipv4
//./IPv6ServerClientDemo CLIENT 127.0.0.1 9527    #client mode, listen only on ipv4 local address
//./IPv6ServerClientDemo CLIENT ::1 9527          #client mode, listen only on ipv6 local address
int main(int argc, char *argv[])
{
    SocketMode_t SocketMode;
    SocketMode = SOCKET_MODE_NONE;

    printf("AppVersion:%s\n",AppVersion);

    if (3 > argc)
    {
        cout << "usage: " << argv[0] << " <mode> [<hostname/ipaddress>] <service/port>" << endl;
        return -1;
    }

    if (strcasecmp(argv[1],"client") == 0)
    {
        SocketMode = SOCKET_MODE_CLIENT;
    }
    else if (strcasecmp(argv[1],"server") == 0)
    {
        SocketMode = SOCKET_MODE_SERVER;
    }

    if ((SocketMode != SOCKET_MODE_CLIENT) && (SocketMode != SOCKET_MODE_SERVER))
    {
        cout << "usage: " << argv[0] << " [<mode>] [<hostname/ipaddress>] <service/port>" << endl;
        return -2;
    }

    if (SocketMode == SOCKET_MODE_CLIENT) //客户端模式
    {
        int sockfd, n;
        char buff[128];
        struct sockaddr_storage cliaddr;

        if (4 > argc)
        {
            cout << "usage: " << argv[0] << " <mode> [<hostname/ipaddress>] <service/port>" << endl;
            return -3;
        }

        sockfd = tcp_connect(argv[2], argv[3]);
        if (sockfd < 0)
        {
            cout << "call tcp_connect error" << endl;
            return -5;
        }

        bzero(buff, sizeof(buff));
        while ((n = read(sockfd, buff, sizeof(buff) - 1) > 0))
        {
            cout << buff << endl;
            bzero(buff, sizeof(buff));
        }
        close(sockfd);
    }
    else if (SocketMode == SOCKET_MODE_SERVER) //服务器模式
    {
        int listenfd, connfd;
        struct sockaddr_storage cliaddr;
        socklen_t len = sizeof(cliaddr);
        time_t now;
        char buff[128];

        if (3 > argc)
        {
            cout << "usage: " << argv[0] << " <mode> [<hostname/ipaddress>] <service/port>" << endl;
            return -4;
        }
        
        if(3 == argc)
        {
            listenfd = tcp_listen("::", argv[2]);
        }
        else
        {
            listenfd = tcp_listen(argv[2], argv[3]);
        }

        if (listenfd < 0)
        {
            cout << "call tcp_listen error" << endl;
            return -6;
        }

        while (true)
        {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

            string ip = "";
            in_port_t port = 0;
            get_addrinfo((struct sockaddr*)&cliaddr, ip, port);
            cout << "client " << ip << "|" << port << " login" << endl;

            now = time(NULL);
            snprintf(buff, sizeof(buff) - 1, "%.24s", ctime(&now));
            write(connfd, buff, strlen(buff));
            close(connfd);
        }

        close(listenfd);
    }
    return 0;
}