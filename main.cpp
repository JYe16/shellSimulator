#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <csignal>
#include <sys/wait.h>

using namespace std;

//if there's error
void file_error();
void general_error();
void ept_error();
//for direct commands
void readcmd();
//for input from fils
void readfile(string fileName);
void execmd(vector<string> cmds);
void tokenize(string input);
void tokenize_p(string input);
void external_cmd(vector<string> cmds, string path);

//declare global variables
vector<string> path_vec;

int main(int argc, char *argv[]){

    //declare variables
    //file input
    ifstream file;
    //command input
    string input;
    //for multiple files input
    int i;
    string ini = "/bin";
    int j;
    path_vec.push_back(ini);

    if(argc >= 2){
        //if there are multiple files input
        for(i = 1; i < argc; i++){
            file.open(argv[i]);
            if(!file){
                ept_error();
            }

            j = 0;
            while(!file.eof()){
                //get the command line by line
                getline(file, input);
                if(input == "" && file.eof() != 0){
                    if(j == 0){
                        ept_error();
                    }else{
                        exit(0);
                    }
                }
                readfile(input);
                j++;
            }
            file.close();
        }
        //if no 'exit' detected
        readcmd();
    }else{
        readcmd();
    }

    return(0);
}

//for parallel commands
void tokenize_p(string input){
    vector<string> cmds_p;
    int pos;
    long unsigned int a = -1;
    pid_t pid;
    int status;

    //seperate 2 parallel commands
    while(input.find('&') != a){
        pos = input.find('&');
        if(pos == 0){
            return;
        }
        if(input.c_str()[pos - 1] != ' '){
            cmds_p.push_back(input.substr(0, pos));
        }
        else{
            cmds_p.push_back(input.substr(0, pos - 1));
        }

        if(input.c_str()[pos + 1] != ' '){
            input = input.substr(pos + 1, input.length());
        }
        else{
            input = input.substr(pos + 2, input.length());
        }
    }
    cmds_p.push_back(input);

    //fork them and run at the same time
    if(cmds_p.size() > 1){
        for(a = 0; a < cmds_p.size(); a++){
            pid = fork();
            if(pid == 0){
                tokenize(cmds_p[a]);
            }else{
                if (waitpid(pid, &status, 0) > 0) {

                    if (WIFEXITED(status) && !WEXITSTATUS(status)) {
                        exit(0);
                    }

                    else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        if (WEXITSTATUS(status) == 127){
                        }
                        else{
                            kill(pid, SIGKILL);
                        }
                    }
                    else
                        general_error();
                }
                else {
                    // waitpid() failed
                    kill(pid, SIGKILL);
                }
            }
        }
    }
        //if there's only 1 command
    else{
        tokenize(cmds_p[0]);
    }
    return;
}

//break commands into pieces and put them into a vector of strings
void tokenize(string input){
    vector<string> cmds;
    int pos;
    long unsigned int a = -1;
    string red = "";
    string redSign = ">";

    //if there are redirections
    if(input.find('>') != a){
        pos = input.find('>');
        if(pos == (int)input.length() - 1){
            general_error();
            return;
        }else if(pos == 0){
            general_error();
            return;
        }
        if(input.c_str()[pos + 1] == ' '){
            red = input.substr(pos + 2, input.length());
        }else{
            red = input.substr(pos + 1, input.length());
        }
        if(input.c_str()[pos - 1] == ' '){
            input = input.substr(0, pos - 1);
        }else{
            input = input.substr(0, pos);
        }
        if(red.find('>') != a){
            general_error();
            return;
        }
        if(red.find(' ') != a){
            general_error();
            return;
        }
    }

    while(input.find(' ') != a){
        pos = input.find(' ');
        if(pos == 0){
            input = input.substr(1, input.length());
        }else{
            cmds.push_back(input.substr(0, pos));
            input = input.substr(pos + 1, input.length());
        }
    }
    cmds.push_back(input);

    if(red != ""){
        cmds.push_back(redSign);
        cmds.push_back(red);
    }

    execmd(cmds);
    return;
}

//read commands from stdin
void readcmd(){
    string input;

    //keep reading inputs from stdin until detect 'exit'
    while(input != "exit"){
        printf("hfsh> ");
        getline(cin, input);
        if(input == ""){
            ept_error();
        }
        tokenize_p(input);
    }

    return;
}

//read commands from file
void readfile(string input){
    if(input != "exit"){
        tokenize_p(input);
    }else{
        exit(0);
    }
}

//execute commands
void execmd(vector<string> cmds){
    int result;
    int j;
    long unsigned int i;
    string path = "";
    pid_t  pid;
    int status;
    string check = "";
    char acc[30];

    //for command cd
    if(cmds[0] == "cd"){
        if(cmds.size() != 2){
            general_error();
        }else{
            char p[30] = "";
            for(i = 0; i < cmds[1].length(); i++){
                p[i] = cmds[1].c_str()[i];
            }
            result = chdir(p);
            if(result == -1){
                general_error();
            }
        }
    }
        //for command path
    else if(cmds[0] == "path"){
        //for 0 arguments
        if(cmds.size() == 1){
            path_vec.clear();
        }
            //for more arguments
        else{
            path_vec.clear();
            for(i = 1; i < cmds.size(); i++){
                path = cmds[i];
                path_vec.push_back(path);
            }
        }
    }
        //for empty command
    else if(cmds[0] == ""){
        return;
    }
        //for external commands
    else{
        if(cmds[0] == "exit"){
            if(cmds.size() == 1){
                exit(0);
            }else{
                general_error();
            }
        }else{
            //if there is no path avaliable
            if(path_vec.size() == 0){
                general_error();
                return;
            }
            j = 0;
            while(j < (int)path_vec.size()){
                check = path_vec[j] + "/" + cmds[0];
                strcpy(acc, check.c_str());
                //check the file if it's runnable
                result = access(acc, X_OK);
                if(result == 0){
                    break;
                }
                j++;
            }

            //if none of the path can run the command
            if(result == -1){
                general_error();
                return;
            }

            pid = fork();
            if(pid == 0){
                external_cmd(cmds, path_vec[j]);
            }else{
                if (waitpid(pid, &status, 0) > 0) {

                    if (WIFEXITED(status) && !WEXITSTATUS(status)) {
                        return;
                    }

                    else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        if (WEXITSTATUS(status) == 127){
                        }
                        else{
                            kill(pid, SIGKILL);
                        }
                    }
                    else
                        general_error();
                }
                else {
                    // waitpid() failed
                    kill(pid, SIGKILL);
                }
            }
        }
    }
}

//execute external commands
void external_cmd(vector<string> cmds, string path){
    long unsigned int j = 0;
    char **arg_arr;
    //allocate the array into memory
    arg_arr = (char**)malloc(20 * sizeof(char*));
    string arg_str = "";
    //for storing the command combined with path
    char extcmd[20] = "";
    char arg[20] = "";

    arg_str = path + "/" + cmds[0];
    strcpy(extcmd, arg_str.c_str());
    extcmd[arg_str.length()] = '\0';

    j = 0;
    while(j < cmds.size()){
        arg_arr[j] = (char*)malloc(20 * sizeof(char));
        if(cmds[j] == ">"){
            if(j != cmds.size() - 2){
                file_error();
            }
            strcpy(arg, cmds[j + 1].c_str());
            freopen(arg, "w", stdout);
            arg_arr[j] = NULL;
            execv(extcmd, arg_arr);
        }
        strcat(arg, "");
        strcpy(arg, cmds[j].c_str());
        strcpy(arg_arr[j], arg);
        arg_arr[j][cmds[j].size()] = '\0';
        j++;
    }
    arg_arr[j + 1] = NULL;
    execv(extcmd, arg_arr);
    free(arg_arr);
}

//for errors that no need to exit the shell
void general_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
}

//for errors that need to exit with code 0
void file_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(0);
}

//for errors that need to exit with code 1
void ept_error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}

