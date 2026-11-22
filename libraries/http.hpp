#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include "utils.hpp"
using namespace std;

namespace http
{
    class Request
    {
    public:
        string method;
        string path;
        string version;
        string headers;
        string body;

        // Constructor
        Request(
            string path = "/",
            string method = "GET",
            string version = "HTTP/1.1",
            string headers = "",
            string body = "")
        {
            this->method = method;
            this->path = path;
            this->version = version;
            this->headers = headers;
            this->body = body;
        }

        size_t get_length()
        {
            return toString().length();
        }

        void fromString(string rawrequest)
        {
            size_t request_line_end = rawrequest.find("\r\n");
            if (request_line_end == string::npos)
            {
                throw utils::custom_runtime_error(errno, "Invalid request");
            }
            string request_line_text = rawrequest.substr(0, request_line_end);
            stringstream ss(request_line_text);
            ss >> method >> path >> version;
            size_t header_line_start = request_line_end + 2;
            size_t header_line_end = rawrequest.find("\r\n\r\n", header_line_start);

            size_t body_start = header_line_start;
            if (header_line_end == string::npos)
            {

                headers = "";
            }
            else
            {
                headers = rawrequest.substr(header_line_start, header_line_end - header_line_start);
                body_start = header_line_end + 4;
            }
            body = rawrequest.substr(body_start);
        }

        string toString()
        {
            return method + " " + path + " " + version + "\r\n" +
                   headers + "\r\n\r\n" +
                   body;
        }
    };
    class Response
    {
    public:
        string status_msg;
        string status_code;
        string version;
        string headers;
        string body;

        // Constructor
        Response(
            string body = "",
            string status_code = "200",
            string status_msg = "OK",
            string headers = "",
            string version = "HTTP/1.1")
        {
            this->status_msg = status_msg;
            this->status_code = status_code;
            this->version = version;
            this->headers = headers;
            this->body = body;
        }

        size_t get_length()
        {
            return toString().length();
        }

        void fromString(string rawrequest)
        {
            size_t response_line_end = rawrequest.find("\r\n");
            if (response_line_end == string::npos)
            {
                throw utils::custom_runtime_error(errno, "Invalid request");
            }
            string response_line_text = rawrequest.substr(0, response_line_end);
            stringstream ss(response_line_text);
            ss >> version >> status_code >> status_msg;
            size_t header_line_start = response_line_end + 2;
            size_t header_line_end = rawrequest.find("\r\n\r\n", header_line_start);

            size_t body_start = header_line_start;
            if (header_line_end == string::npos)
            {

                headers = "";
            }
            else
            {
                headers = rawrequest.substr(header_line_start, header_line_end - header_line_start);
                body_start = header_line_end + 4;
            }
            body = rawrequest.substr(body_start);
        }

        string toString()
        {
            return version + " " + status_code + " " + status_msg + "\r\n" +
                   headers + "\r\n\r\n" +
                   body;
        }
    };

    inline Response request_handler(const Request &request)
    {
        try
        {
            // Validating Request Method
            if (request.method != "GET")
                return Response("Invalid Request", "400", "Bad Request");
            // Path. taversal Security Check
            if (request.path.find("..") != string::npos)
                return Response("Access Denied", "400", "Bad Request");
            // Correcting path
            string safepath = "./files" + request.path;
            // Opening File
            ifstream file(safepath, ios::binary);
            if (!file.is_open())
                return Response("File does not exist", "404", "Not Found");
                
            stringstream file_stream;
            file_stream << file.rdbuf();
            file.close();
            // Converting File Stream to String Data
            string file_data = file_stream.str();
            return Response(file_data, "200", "OK", "Content-Length: " + to_string(file_data.length()));
        }
        catch (const exception &e)
        {
            return Response(e.what(), "400", "Bad Request");
        }
    }
}