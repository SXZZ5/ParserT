#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <set>
#include <string>
#include <cstring>

#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "stringy.hpp"
using namespace std;

void copy_solprog(string id){
    string command = "cp sksk_solution sksk_solution"; 
    command.append(id);
    std::system(command.c_str());
}

std::mutex for_ansidx;
std::vector<int> ansidx;
//ansidx[i] is supposed to maintain the shortest among all strings that turned out to be
//sufficient for testnum = i.
std::mutex for_idz;
std::set<string> idz;

bool prepareLog(std::ofstream &log, string filename){
    filename.append(".log");
    log.open(filename, std::ios::out);
    if(!log.is_open()){
        return false;
    }
    else{
        return true;
    }
}

void caller(std::stop_token stoptoken, string std_in, int testnum, int pos, int& res){
    string _testnum = std::to_string(testnum);
    string _pos = std::to_string(pos);
    string id = _testnum; id += 'x'; id.append(_pos);
    for_idz.lock();
    idz.insert(id);
    for_idz.unlock();

    copy_solprog(id);
    std_in.append("\n");
    
    string pipekanaam = "ppipe";
    pipekanaam.append(id); 
    // cout << "pipekanaam: " << pipekanaam << endl;
    
    mkfifo(pipekanaam.c_str(), 0777);
    int pid = fork();
    
    if(pid == 0){
        // child process
        string cmdline = "./sksk_solution";
        cmdline.append(id);
        cmdline.append(" < "); cmdline.append(pipekanaam);
        cmdline.append(" > out"); cmdline.append(id);

        // cout << "in child process, going to do std::system() call next.\n" << cmdline << endl;
        std::system(cmdline.c_str());
    }
    else{
        // parent process.
        int fd = open(pipekanaam.c_str() , O_WRONLY);
        char buffer[100000];
        const char * std_inc = std_in.c_str();
        sprintf(buffer, "%s", std_inc);

        write(fd, buffer, strlen(buffer));
        
        usleep(30000);
        while(!stoptoken.stop_requested()){
            //busywait for stop_request
        }
        
        int wstatus;
        kill(pid, SIGKILL);
        wait(&wstatus);
        
        if (WIFEXITED(wstatus)){
            for_ansidx.lock();
                cout << "######################\n";
                cout << "old ansidx value: " << ansidx[testnum-1] << "\n";
                ansidx[testnum-1] = min(ansidx[testnum-1], pos);
                cout << "new ansdix value: " << ansidx[testnum-1] << "\n";
            for_ansidx.unlock();
            res = 1;
        } 
       else {
            res = 0;
        }
        close(fd);
    }    
}

int main(int argc, char** argv){

    string input_name = argv[1];
    string sol_name = argv[2];
    std::ofstream log;
    if(!prepareLog(log, input_name)){
        cout << "Failed to open a log file\n";
        cout << "clear log files and try again\n";
        return 1;
    }

    copy_solution(sol_name);

    int test_cnt = 0;
    vector<string> lines = make_lines(input_name, test_cnt);
    int line_cnt = size(lines);
    log << "test_cnt: " << test_cnt << "\n";
    log << "line_cnt: " << line_cnt << "\n";

    // return 0;
    ansidx.resize(test_cnt, 69696969);

    vector<string> cumu_lines(line_cnt, "");
    for(int i = 1; i <= line_cnt; ++i){
        if(i == 1){
            cumu_lines[0] = lines[0];
            continue;
        }
        
        cumu_lines[i-1] = cumu_lines[i-2];
        cumu_lines[i-1].append("\n");
        cumu_lines[i-1].append(lines[i-1]);
    }
    
    vector<vector<string>> std_in(line_cnt);
    for(int i = 1; i <= line_cnt; ++i){
        for(int j = 1; j <= test_cnt; ++j){
            string number = std::to_string(j);
            string tmp = ""; tmp.append(number); tmp.append("\n");
            tmp.append(cumu_lines[i-1]); tmp.append("\n");
            std_in[i-1].push_back(tmp);
        }
    }

    std::stop_source stopsource;
    for(int i = 1; i <= line_cnt; ++i){
        for(int j = 1; j <= test_cnt; ++j){
            int res = -1;
            std::thread temp(&caller, stopsource.get_token(), 
                             std_in[i-1][j-1], j, i, std::ref(res));
            // usleep(200000);
            stopsource.request_stop();
            temp.join();
            if(res == 0){
                log << "solprog (testnum, pos): (" << j<< 
                "," << i << ")" << " was terminated by a signal\n";
            }
            else if(res == 1){
                log << "solprog (testnum, pos): (" << j << 
                "," << i << ")" << " ended normally\n";
            }
            else{
                log << "SOMETHING BAD HAPPENDED, THREAD DIDN'T GIVE ANY RESULT\n";
            }
            log << std_in[i-1][j-1] << endl;
        }
    }
    
    log << "ANSIDX RECORDED\n";
    for(int i = 1; i <= test_cnt; ++i){
        log << "ansidx[" << i <<"]: " << ansidx[i-1] << "\n";
    }
    log << endl;
    
    vector<string> answers(test_cnt,"");
    int prev = -1;
    for(int i = 1; i <= test_cnt; ++i){
        if(i == 1){
            int idx = ansidx[i-1];
            answers[i-1] = cumu_lines[idx-1];
            prev = idx;
            continue;
        }
        int idx = ansidx[i-1];
        string tmp = "";
        for(int j = prev + 1; j <= idx; ++j){
            tmp.append(lines[j-1]); tmp.append("\n");
        }
        prev = idx;
        answers[i-1] = tmp;
    }

    writeToFile(answers, input_name);
    
    log << "lets see the strings now" << endl;
    log << "--------------------------------------------------" << endl;
    log << "idz \n";
    for(auto x : idz){
        log << x <<", ";
    }
    log << endl;
    log << "FINAL RESULT" << endl;
    for(int i = 1; i <=test_cnt; ++i){
        log << "For Test: " << i << "\n";
        log << answers[i-1] << endl;
    }

    log.close();
    cleanup();
    return 0;
}
