/* Zach Schickler */

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>

using namespace std;

namespace fs = std::filesystem;

typedef enum {
    unknownsym = 0, clearsym = 1, quitsym = 2, echosym = 3, whereamisym = 4, changedirsym = 5,
    lastcommandssym = 6, runsym = 7, backgroundsym = 8, exterminatesym = 9
} command_type;

// This function takens in a vector of strings and adds the passed in string to the vector.
// It returns the new vector.
vector<string> add_token(vector<string> cur_tokens, string new_token) {
    vector<string>::iterator it = cur_tokens.end();
    cur_tokens.insert(it, new_token);
    return cur_tokens;
}

// This function prints different error messages based on the error code it recieves.
void throw_error(int error_code) {
    switch (error_code) {
        case 1:
            cout << "Error 1: Command not recognized.\n";
            break;
        case 2:
            cout << "Error 2: Invalid number of arguments passed.\n";
            break;
        case 3:
            cout << "Error 3: Directory unknown.\n";
            break;
        case 4:
            cout << "Error 4: Unrecognized flag.\n";
            break;
        case 5:
            cout << "Error 5: Cannot find program.\n";
            break;
        case 6:
            cout << "Error 6: Fork could not be made.\n";
            break;
        case 7:
            cout << "Error 7: Program was unable to start.\n";
            break;
        default:
            cout << "An unknown error has occured.\n";
            break;
    }
}

// This function returns the command number for a given command.
command_type get_command_token(string command) {
    if (command.compare("clear") == 0)
        return clearsym;
    else if (command.compare("quit") == 0)
        return quitsym;
    else if (command.compare("echo") == 0)
        return echosym;
    else if (command.compare("whereami") == 0)
        return whereamisym;
    else if (command.compare("changedir") == 0)
        return changedirsym;
    else if (command.compare("lastcommands") == 0)
        return lastcommandssym;
    else if (command.compare("run") == 0)
        return runsym;
    else if (command.compare("background") == 0)
        return backgroundsym;
    else if (command.compare("exterminate") == 0)
        return exterminatesym;
    else
        return unknownsym;
}

int execute_command(vector<string> arg_tokens, int num_args, command_type command_token, fs::path* path, vector<string>* command_history) {
    fs::path temp_path;
    pid_t pid;
    int status = 0;

    vector<char *> argv(arg_tokens.size() + 1); 

    switch (command_token) {
        case unknownsym:
            throw_error(1);
            break;

        case clearsym:
            if (num_args != 0) { // Check to make sure there aren't too many arguments. Repeats a lot.
                throw_error(2);
                break;
            }
            cout << "\e[1;1H\e[2J"; // easy hack to "clear" console.
            break;

        case quitsym:
            if (num_args != 0) {
                throw_error(2);
                break;
            }
            exit(0); 
            break;

        case echosym:
            for (vector<string>::iterator i=arg_tokens.begin(); i!=arg_tokens.end(); i++) // Iterates through every token after echo, so if someone echos something
                cout << *i << " ";                                                        // with spaces, it's accounted for.
            cout << "\n";
            break;

        case whereamisym:
            if (num_args != 0) {
                throw_error(2);
                break;
            }
            cout << (*path) << "\n"; // path is already saved so we simply print it out.
            break;

        case changedirsym:
            if (num_args != 1) {
                throw_error(2);
                break;
            }
            if (!(fs::exists(*arg_tokens.begin()) && fs::is_directory(*arg_tokens.begin()))) { // ensure that our new directory is actually real and a directory.
                throw_error(3);                                                                // fs is great for handling these things.
                break;
            }
            (*path).clear();
            (*path) = *arg_tokens.begin(); // update our stored path.
            break;
        
        case lastcommandssym:
            if (num_args > 1) {
                throw_error(2);
                break;
            }
            if (num_args == 1) {
                if ((*arg_tokens.begin()).compare("-c") == 0) { // -c is flag to clear history.
                    (*command_history).clear();
                }
                else {
                    throw_error(4);
                    break;
                }
            }
            else {
                for (vector<string>::iterator i=(*command_history).begin(); i!=(*command_history).end(); i++) // simply iterate through command history which is stored.
                    cout << *i << "\n";
            }
            break;

        case runsym:
            if (num_args == 0) {
                throw_error(2);
                break;
            }

            if ((*arg_tokens.begin()).at(0) == '/')
                temp_path = *arg_tokens.begin();
            else {
                temp_path = *path;
                temp_path.append(*arg_tokens.begin());
            }

            if (!(fs::exists(temp_path) && !fs::is_directory(temp_path))) {
                throw_error(5);
                break;
            }

            // Trim the first token off of the arg list, since that was the path.
            arg_tokens.erase(arg_tokens.begin());

            // The following process is done to turn the string vector into
            // a form that can be accepted by execvp.
            for (size_t i = 0; i != arg_tokens.size(); i++)
                argv[i] = &arg_tokens[i][0];

            pid = fork();

            if (pid == 0) {
                if (execv(temp_path.c_str(), argv.data()) == -1)
                    perror("exec");
                break;
            }
            if (pid > 0) {
                wait(0);
            }
            break;
        
        case backgroundsym: // almost identical to runsym except parent process does not wait for child process to finish, it sleeps a bit then resumes.
            if (num_args == 0) {
                throw_error(2);
                break;
            }

            if ((*arg_tokens.begin()).at(0) == '/')
                temp_path = *arg_tokens.begin();
            else {
                temp_path = *path;
                temp_path.append(*arg_tokens.begin());
            }

            if (!(fs::exists(temp_path) && !fs::is_directory(temp_path))) {
                throw_error(5);
                break;
            }

            // Trim the first token off of the arg list, since that was the path.
            arg_tokens.erase(arg_tokens.begin());

            // The following process is done to turn the string vector into
            // a form that can be accepted by execvp.
            for (size_t i = 0; i != arg_tokens.size(); i++)
                argv[i] = &arg_tokens[i][0];

            pid = fork();

            if (pid == 0) {
                cout << "child PID: " << pid << endl;
                if (execv(temp_path.c_str(), argv.data()) == -1)
                    perror("exec");
                break;
            }
            if (pid > 0) {
                sleep(1);
            }
            break;

        case exterminatesym:
            if (num_args == 0) {
                throw_error(2);
                break;
            }

            if(kill(atoi((*arg_tokens.begin()).c_str()), SIGKILL) != 0)
                perror("error");
            else
                cout << "Process has been killed." << endl;
            break;
    }
    return 0;
}

int main() {
    string input; // The raw input
    vector<string> tokenized_input; // The input but tokenized.
    vector<string> input_history; // A list of previous raw inputs.

    command_type cur_command; // ONLY the current first token as a command_type object.
    fs::path cur_path = fs::current_path(); // The current path as a path object.
    int token_count = 0; // A counter for the number of arguments following the command.

    while (1) {

        // Steps to loop:
        // 1. Print input request.
        // 2. Get input.
        // 3. Parse input.
        // 4. Execute commands based on tokens.


        // Printing the input request.
        cout << "# ";

        // Clear the input.
        input = "";
        // Get the input from the keyboard.
        getline(cin, input);
        // Add the input to the input history.
        input_history = add_token(input_history, input);

        // Parse input.
        string cur_word = "";
        for (string::iterator i=input.begin(); i!=input.end(); i++) {
            if (*i == ' ') {
                tokenized_input = add_token(tokenized_input, cur_word);
                token_count++;
                cur_word = "";
            }
            else
                cur_word = cur_word + *i;
        }
        tokenized_input = add_token(tokenized_input, cur_word);

        // Get the current command token based on the first token of the input.
        cur_command = get_command_token(*tokenized_input.begin());

        // Trim the token list to only pass in arguments and execute the given command.
        // Make sure to update the path after any modifications made during execution.
        tokenized_input.erase(tokenized_input.begin());
        int success = execute_command(tokenized_input, token_count, cur_command, &cur_path, &input_history);
        
        // Reset for the next input.
        tokenized_input.clear();
        token_count = 0;

    }
    return 0;
}