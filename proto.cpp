// #include <bits/stdc++.h>
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

using namespace std;

std::mutex for_ansidx;
std::vector<int> ansidx(3, INT_MAX);
//ansidx[i] is supposed to maintain the shortest among all strings that turned out to be
// sufficient for testnum = i.

void copy_solprog(string id){
    string command = "cp solution solution"; 
    command.append(id);
    std::system(command.c_str());
}

void del_solprog_and_pipe_out(string id){
    string command = "rm solution";
    command.append(id);
    std::system(command.c_str());
    command = "rm ppipe";
    command.append(id);
    std::system(command.c_str());
    command = "rm out";
    command.append(id);
    std::system(command.c_str());
}

std::mutex printer;
std::mutex for_idz;
std::set<string> idz;


void caller(std::stop_token stoptoken, string std_in, int testnum, int pos){
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
    bool finished = false;
    int pid = fork();
    
    if(pid == 0){
        // child process
        string cmdline = "./solution";
        cmdline.append(id);
        cmdline.append(" < "); cmdline.append(pipekanaam);
        cmdline.append(" > out"); cmdline.append(id);

        // cout << "in child process, going to do std::system() call next.\n" << cmdline << endl;
        std::system(cmdline.c_str());
        finished = true;
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
            printer.lock();
            cout << "solprog (testnum, pos): (" << testnum << "," << pos << ")" << " ended normally" << endl;
            printer.unlock();

            for_ansidx.lock();
                ansidx[testnum-1] = min(ansidx[testnum-1], pos);
            for_ansidx.unlock();
        } 
       else {
            printer.lock();
            cout << "solprog (testnum, pos): (" << testnum << "," << pos << ")" << " was terminated by a signal" << endl;
            cout << std_in << endl;
            printer.unlock(); 
        }
        close(fd);
    }    
    del_solprog_and_pipe_out(id);
}

int main(){

    vector<string> lines;
    lines.push_back("1 2 3");
    lines.push_back("2 3");
    lines.push_back("1 3");
    lines.push_back("1 2");
    lines.push_back("1 4 7");
    lines.push_back("2 5");
    lines.push_back("3 4");
    lines.push_back("2 4");
    lines.push_back("1 2");
    lines.push_back("3 5");
    lines.push_back("4 5");
    lines.push_back("1 5");
    lines.push_back("3 3 7");
    lines.push_back("1 2");
    lines.push_back("1 6");
    lines.push_back("2 3");
    lines.push_back("2 5");
    lines.push_back("3 4");
    lines.push_back("4 5");
    lines.push_back("4 6");

    int line_cnt = lines.size();
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
        for(int j = 1; j <= 3; ++j){
            string number = std::to_string(j);
            string tmp = ""; tmp.append(number); tmp.append("\n");
            tmp.append(cumu_lines[i-1]); tmp.append("\n");
            std_in[i-1].push_back(tmp);
        }
    }

    std::stop_source stopsource;
    for(int i = 1; i <= line_cnt; ++i){
        for(int j = 1; j <= 3; ++j){
            std::thread temp(&caller, stopsource.get_token(), std_in[i-1][j-1], j, i);
            // usleep(2000);
            stopsource.request_stop();
            temp.join();
        }
    }
    
    for(int i = 1; i <= 3; ++i){
        printf("ansidx[%d]: %d\n", i, ansidx[i-1]);
    }
    cout << endl;
    
    cout << "lets see the strings now" << endl;
    vector<string> answers(3,"");
    int prev = -1;
    for(int i = 1; i <= 3; ++i){
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
    
    cout << "--------------------------------------------------" << endl;
    cout << "idz \n";
    for(auto x : idz){
        cout << x <<", ";
    }
    cout << endl;
    cout << "FINAL RESULT" << endl;
    for(int i = 1; i <=3; ++i){
        cout << "For Test: " << i << "\n";
        cout << answers[i-1] << endl;
    }

    return 0;
}
