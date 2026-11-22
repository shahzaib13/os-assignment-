#include <pthread.h>
#include <queue>
#include <signal.h>
// Custom libraries
#include "../libraries/tcp.hpp"
#include "../libraries/http.hpp"

// MUST required global variables
string ip = "0.0.0.0";
int port = 8000;
int threads = 1;
int max_reuqest_queue = 10;
bool debug_mode = false;
const int BUFFERSIZE = 64 * 1024; // 64kb
// thread-need specific global variables
queue<int> connection_queue;
pthread_mutex_t MUTEX_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t THREAD_COND = PTHREAD_COND_INITIALIZER;

// functions
void *worker_function(void *arg);
void cmd_handler(int argc, char *argv[]);

int main(int argc,char* argv[])
{
    // Ignoring error signals when client closes before handler finish
    signal(SIGPIPE,SIG_IGN);
    try
    {
        cmd_handler(argc,argv);
        // Creating Socket
        tcp::Server server(ip, port);
        // intializing and setting thread attribtues
        vector<pthread_t> tids(threads);
        pthread_attr_t thread_attribtues;
        pthread_attr_init(&thread_attribtues);
        pthread_attr_setscope(&thread_attribtues, PTHREAD_SCOPE_SYSTEM);
        // creating threads
        for (int i = 0; i < threads; i++)
        {
            try{
                
                int thread_response = pthread_create(&tids[i], &thread_attribtues, &worker_function, &server);
                if(thread_response != 0){
                    throw utils::custom_runtime_error(errno);
                }
            }catch(const exception &e){
                cout << e.what() << endl;
            }
        }
        // Binding server to ip:port
        server.bind_to_port();
        // Start listening to requests
        server.start_listening(max_reuqest_queue);
        cout << "Server is started at: http://"<<ip << ":" <<port<< endl;
        cout << "Number of threads: "<< threads << "\n\n";
        while (true)
        {
            // Accepting client connections and adding to request queues for worker thread
            sockaddr_in client_addr;
            int client_fd = -1;
            // Handling this seperately, so it does not stop the loop on connection rejection
            try{
                client_fd = server.accept_client_connections(client_addr);
            }
            catch(const exception &e){
                cout << e.what() << endl;
                continue;
            }
            pthread_mutex_lock(&MUTEX_LOCK);
            connection_queue.push(client_fd);
            pthread_cond_signal(&THREAD_COND);
            pthread_mutex_unlock(&MUTEX_LOCK);
        }
    }
    catch (const exception &e)
    {
        cout << e.what() << endl;
    }
}

void *worker_function(void *arg)
{
    tcp::Server server = *(tcp::Server *)arg;
    while (true)
    {
        int connection_fd = -1;
        try
        {
            pthread_mutex_lock(&MUTEX_LOCK);
            while (connection_queue.empty())
            {
                pthread_cond_wait(&THREAD_COND, &MUTEX_LOCK);
            }
            connection_fd = connection_queue.front();
            connection_queue.pop();
            pthread_mutex_unlock(&MUTEX_LOCK);
            // reading client request
            vector<char> buffer(BUFFERSIZE,'\0');
            server.read_request(connection_fd, buffer.data(), 1024);
            // converting raw request to request object
            http::Request request;
            request.fromString(buffer.data());
            // showing request [only in debug mode]
            if(debug_mode){
                cout << request.toString() << endl;
            }
            // generating response
            http::Response response = http::request_handler(request);
            // sending response
            server.send_response(connection_fd, response.toString(), response.get_length());
            close(connection_fd);
        }
        catch (const exception &e)
        {
            cout << e.what() << endl;
            close(connection_fd);
        }
    }
}
void cmd_handler(int argc, char *argv[]){

    for (int i = 1; i < argc; i++){
        string current_arg = argv[i];
        // Print help
        if (current_arg == "-h" || current_arg == "--help"){
            cout << "\nServer program v1.0.0\n"
            << "Command lists: \n"
            << "\t--help or -h : show command lists\n"
            << "\t--debug or -d : show request in output\n"
            << "\t--ip or -i : server ip address. [Default : 0.0.0.0]\n"
            << "\t--port or -p : server port number. [Default : 8000]\n"
            << "\t--thread or -t : number of threads. [Default : 1]\n"
            << "\t--backlog or -b : max request queue. [Default : 10]\n";
            exit(0);
        }
        // Enable Debug mode
        if (current_arg == "-d" || current_arg == "--debug"){
            debug_mode = true;
            continue;
        }
        // validating command argument must have value
        if (i + 1 >= argc) throw utils::custom_runtime_error(0, "Invalid Arguments");
        // storing ip
        if (current_arg == "-i" || current_arg == "--ip"){
            ip = argv[i + 1];
            i++;
        }
        // working with numeric arguments
        else{
            // converting value to int
            int current_arg_value = atoi(argv[i + 1]);
            // validating arguments must be greater than 0
            if (current_arg_value < 1) throw utils::custom_runtime_error(0, "Value must be greater than 0");
            // storing port
            else if (current_arg == "-p" || current_arg == "--port"){
                port = current_arg_value;
                i++;
            }
            // storing thread count
            else if (current_arg == "-t" || current_arg == "--thread"){
                threads = current_arg_value;
                i++;
            }    
            // storing max requests in queue   
            else if (current_arg == "-b" || current_arg == "--backlog"){
                max_reuqest_queue = current_arg_value;
                i++;
            }       
            // throwing error on invalid / unknown arguments
            else{
                throw utils::custom_runtime_error(0,"Invalid Arguments");
            }
        }
    }
}