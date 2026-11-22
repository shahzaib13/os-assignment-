#include <iostream>
#include <vector>
#include <filesystem>
using namespace std;

int main()
{
    vector<int> st_list = {1,2,3,4,5}; // server thread
    vector<int> ct_list = {10,20,30,40,50,100}; // client thread
    vector<int> cr_list = {10,20,30,40,50,100}; // client request
    string filelist = "/1kb.txt"; // file list client requests 

    int st_list_size = st_list.size();
    int ct_list_size = ct_list.size();
    int cr_list_size = cr_list.size();
    for (int i = 0; i < st_list_size; i++)
    {
        // running server in outerloop so that it does not have to create on every client thread/request combo
        int st = st_list[i];
        string server_command = "make run-server args=\"-p 8001 -t " + to_string(st) + "\" &";
        system(server_command.c_str());
        system("sleep 2");
        for (int j = 0; j < ct_list_size; j++)
        {
            int ct = ct_list[j];
            for (int k = 0; k < cr_list_size; k++)
            {
                system("clear");
                int cr = cr_list[j];
                string client_command = "\nmake run-client args=\"-p 8001 -t " + to_string(ct) + " -r " + to_string(cr) + " -f " + filelist +" --csv\" >> ./tests/output/result.csv 2>&1";
                system(client_command.c_str());
                
            }
        }
        system("pkill -f ./server");
    }   
}