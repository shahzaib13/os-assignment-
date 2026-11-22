#pragma once

#include <arpa/inet.h>
#include <string>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
// custom header
#include "utils.hpp"

namespace tcp
{
    class Server
    {
        int server_fd;
        sockaddr_in serverinfo;
        int create_socket()
        {
            int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return socket_fd;
        }

    public:
        Server(const string &host_ip, int port)
        {
            serverinfo.sin_family = AF_INET;
            serverinfo.sin_addr.s_addr = inet_addr(host_ip.c_str());
            serverinfo.sin_port = htons(port);
            server_fd = create_socket();
        }
        // Destructor
        ~Server()
        {
            if (server_fd >= 0)
            {
                close(server_fd);
            }
        }
        // Getters
        int get_fd() { return server_fd; }
        sockaddr_in get_address() { return serverinfo; }
        // binding server to ip:port
        void bind_to_port()
        {
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            int bindingResponse = ::bind(server_fd, (const sockaddr *)&serverinfo, sizeof(serverinfo));
            if (bindingResponse < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
        }
        // started listing clients requests
        void start_listening(int maxrequestqueue = 4)
        {
            int listnerResponse = listen(server_fd, maxrequestqueue);
            if (listnerResponse < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
        }
        // accepting cleints connections
        int accept_client_connections(sockaddr_in &client_address)
        {
            socklen_t client_address_length = sizeof(client_address);
            int clientfd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_length);
            if (clientfd < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return clientfd;
        }

        // reading cleints requets
        ssize_t read_request(int fd, char *buffer, int buffersize)
        {
            ssize_t btyesread = recv(fd, buffer, buffersize, 0);
            if (btyesread < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return btyesread;
        }
        // sending response
        ssize_t send_response(int fd, string buffer, int buffersize)
        {
            ssize_t btyessend = send(fd, buffer.c_str(), buffersize, 0);
            if (btyessend < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return btyessend;
        }
    };

    class Client
    {
        int fd;
        int create_socket()
        {
            int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return socket_fd;
        }

    public:
        Client()
        {
            fd = create_socket();
        }
        // Destructor
        ~Client()
        {
            if (fd >= 0)
            {
                close(fd);
            }
        }
        // Getters
        int get_fd() { return fd; }
        // Connect with server
        void connect_with_server(const string &server_ip, int port)
        {
            sockaddr_in server_address;
            server_address.sin_family = AF_INET;
            server_address.sin_addr.s_addr = inet_addr(server_ip.c_str());
            server_address.sin_port = htons(port);
            int response = connect(fd, (const sockaddr *)&server_address, sizeof(server_address));
            if (response < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
        }
        // reading server response
        ssize_t read_response(char *buffer, int buffersize)
        {
            ssize_t btyesread = recv(fd, buffer, buffersize, 0);
            if (btyesread < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return btyesread;
        }
        // sending request
        ssize_t send_request(string buffer, int buffersize)
        {
            ssize_t btyessend = send(fd, buffer.c_str(), buffersize, 0);
            if (btyessend < 0)
            {
                throw utils::custom_runtime_error(errno);
            }
            return btyessend;
        }
    };
}