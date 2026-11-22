#include <chrono>
#include <pthread.h>
#include <time.h>
#include <vector>
// Custom libraries
#include "../libraries/tcp.hpp"
#include "../libraries/http.hpp"

// must required global variables
string ip = "0.0.0.0";
int port = 8000;
int threads = 1;
int requests = 1;
const int BUFFERSIZE = 64 * 1024; // 64kb
vector<string> files = {"/1kb.txt", "/2kb.txt", "/4kb.txt", "/1mb.txt", "/5mb.txt", "/10mb.txt"};

// variables for logging
double timetaken = 0;
int rejected_requests = 0;
int successfull_response = 0;
int unsuccessfull_response = 0;
unsigned long long total_bytesread = 0;
bool isOutputCSV = false;
pthread_mutex_t MUTEX_LOCK = PTHREAD_MUTEX_INITIALIZER;

// Functions
void *worker_function(void *args);
void cmd_handler(int argc, char *argv[]);
void logging()
{
    if (isOutputCSV)
    {
        cout << timetaken << "," 
        << threads * requests << "," 
        << rejected_requests << ","
        << successfull_response << ","
        << unsuccessfull_response << ","
        << total_bytesread / 1024 << ","
        << total_bytesread / (1024 * 1024) << endl;
    }
    else
    {
        cout << "Time taken: " << timetaken << " ms" << endl;
        cout << "Total requests: " << threads * requests << endl;
        cout << "Total rejected requests: " << rejected_requests << endl;
        cout << "Total successfull response: " << successfull_response << endl;
        cout << "Total unsuccessfull response: " << unsuccessfull_response << endl;
        cout << "Data received: " << total_bytesread / (1024) << " kb" << endl;
        cout << "Data received: " << total_bytesread / (1024 * 1024) << "mb" << endl;
    }
}
int main(int argc, char *argv[])
{
    auto timestart = chrono::high_resolution_clock::now();
    try
    {
        // handling commandline inputs
        cmd_handler(argc, argv);
        // creating thread id list
        vector<pthread_t> tids(threads);
        // intializing and setting thread attribtues
        pthread_attr_t thread_attribtues;
        pthread_attr_init(&thread_attribtues);
        pthread_attr_setscope(&thread_attribtues, PTHREAD_SCOPE_SYSTEM);
        // creating threads
        for (int i = 0; i < threads; i++)
        {
            pthread_create(&tids[i], &thread_attribtues, &worker_function, nullptr);
        }
        // waiting / joining threads to finish work
        for (int i = 0; i < threads; i++)
        {
            pthread_join(tids[i], nullptr);
        }
    }
    catch (const exception &e)
    {
        cout << e.what() << endl;
        return -1;
    }
    // time monitoring
    auto timefinish = chrono::high_resolution_clock::now();
    timetaken = chrono::duration_cast<chrono::milliseconds>(timefinish - timestart).count();
    // Logging
    logging();
}

void *worker_function(void *)
{
    // local variables helpfull for logs/ metrics collection
    unsigned long long bytes_read_per_thread = 0;
    int successfull_response_per_thread = 0;
    int unsuccessfull_response_per_thread = 0;
    int rejected_request_per_thread = 0;
    // For randomly picking files

    unsigned int seed = (time(nullptr) ^ (unsigned long)pthread_self());
    // starting server requests
    for (int i = 0; i < requests; i++)
    {
        try
        {
            // creating socket and connecting with server
            tcp::Client client;
            client.connect_with_server(ip, port);
            // randomly picking file index
            int file_index = rand_r(&seed) % files.size();
            // sending request
            http::Request request(files[file_index]);
            client.send_request(request.toString(), request.get_length());
            // getting response
            vector<char> data_recv_per_chunck(BUFFERSIZE, '\0');
            ssize_t bytesrecv_per_chunck = 0;
            size_t bytesrecv_per_request = 0;
            string data_recv_per_request;
            do
            {
                bytesrecv_per_chunck = recv(client.get_fd(), data_recv_per_chunck.data(), data_recv_per_chunck.size(), 0);
                {
                    data_recv_per_request += data_recv_per_chunck.data();
                    bytesrecv_per_request += bytesrecv_per_chunck;
                }
            } while (bytesrecv_per_chunck > 0);

            http::Response response;
            response.fromString(data_recv_per_request);
            // updating metrices
            bytes_read_per_thread += bytesrecv_per_request;
            if (response.status_code == "200")
                successfull_response_per_thread++;
            else
            {
                cout << "Error: " << response.body << ". [Status Code: " << response.status_code << " Status Message: " << response.status_msg << "]" << endl;
                unsuccessfull_response_per_thread++;
            }
        }
        catch (const exception &e)
        {
            cout << e.what() << endl;
            rejected_request_per_thread++;
        }
    }
    // updating global logs collection variables
    pthread_mutex_lock(&MUTEX_LOCK);
    total_bytesread += bytes_read_per_thread;
    successfull_response += successfull_response_per_thread;
    unsuccessfull_response += unsuccessfull_response_per_thread;
    rejected_requests += rejected_request_per_thread;
    pthread_mutex_unlock(&MUTEX_LOCK);
    return nullptr;
}

void cmd_handler(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        string current_arg = argv[i];
        // print help message
        if (current_arg == "-h" || current_arg == "--help")
        {
            cout << "\nClient program v1.0.0\n"
                 << "Command lists: \n"
                 << "\t--help or -h : show command lists\n"
                 << "\t--ip or -i : server ip address. [Default : 0.0.0.0]\n"
                 << "\t--port or -p : server port number. [Default : 8000]\n"
                 << "\t--thread or -t : number of threads. [Default : 1]\n"
                 << "\t--request or -r : number of requests per thread [Default : 1]\n"
                 << "\t--files or -f : list of files names.[Example: /1kb.txt,/2kb.txt,/4kb.txt]\n"
                 << "\t--csv or -c : set output mode to test.[compatible for csv storage]\n";
            exit(0);
        }
        if(current_arg == "-c" || current_arg == "--csv"){
            isOutputCSV = true;
            i++;
            continue;
        }
        // validating command argument must have value
        if (i + 1 >= argc)
            throw utils::custom_runtime_error(0, "Invalid Arguments");
        // storing ip
        if (current_arg == "-i" || current_arg == "--ip")
        {
            ip = argv[i + 1];
            i++;
        }
        // parsing and storing files path
        else if (current_arg == "-f" || current_arg == "--files")
        {
            string raw_paths = argv[i + 1];
            string path = "";
            files.clear();
            // checking and storing all paths seperated by ','
            for (size_t j = 0; j < raw_paths.length(); j++)
            {
                if (raw_paths[j] == ',')
                {
                    if (!path.empty())
                    {
                        files.push_back(path);
                        path = "";
                    }
                }
                else
                {
                    path += raw_paths[j];
                }
            }
            // storing final path
            if (!path.empty())
                files.push_back(path);
            i++;
        }
        // working with numeric arguments
        else
        {
            int current_arg_value = atoi(argv[i + 1]);
            // validating arguments must be postive and greater than 0
            if (current_arg_value < 1)
                throw utils::custom_runtime_error(0, "Value must be greater than 0");
            // storing port
            else if (current_arg == "-p" || current_arg == "--port")
            {
                port = current_arg_value;
                i++;
            }
            // storing thread count
            else if (current_arg == "-t" || current_arg == "--thread")
            {
                threads = current_arg_value;
                i++;
            }
            // storing request count
            else if (current_arg == "-r" || current_arg == "--request")
            {
                requests = current_arg_value;
                i++;
            }
            // throwing error on invalid/ unknown arguments
            else
            {
                throw utils::custom_runtime_error(0, "Invalid Arguments");
            }
        }
    }
}