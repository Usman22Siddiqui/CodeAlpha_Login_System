#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <limits>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool hasSpace(const string& s) {
    return any_of(s.begin(), s.end(), [](unsigned char c){ return isspace(c); });
}

bool isValidUsername(const string& u) {
    if (u.size() < 3 || u.size() > 20) return false;
    if (hasSpace(u)) return false;
    for (unsigned char c : u) {
        if (!(isalnum(c) || c == '_' || c == '.')) return false;
    }
    return true;
}

bool isStrongPassword(const string& p) {
    if (p.size() < 6) return false;
    if (hasSpace(p)) return false;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (unsigned char c : p) {
        if (isupper(c)) hasUpper = true;
        else if (islower(c)) hasLower = true;
        else if (isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string fnv1a64(const string& s) {
    const unsigned long long FNV_OFFSET = 14695981039346656037ULL;
    const unsigned long long FNV_PRIME  = 1099511628211ULL;
    unsigned long long hash = FNV_OFFSET;
    for (unsigned char c : s) {
        hash ^= (unsigned long long)c;
        hash *= FNV_PRIME;
    }
    ostringstream oss;
    oss << hex << hash;
    return oss.str();
}

bool userExists(const string& username) {
    ifstream in("users.db");
    if (!in) return false;
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        size_t pos = line.find('|');
        if (pos == string::npos) continue;
        string u = line.substr(0, pos);
        if (u == username) return true;
    }
    return false;
}

bool saveUser(const string& username, const string& passHash) {
    ofstream out("users.db", ios::app);
    if (!out) return false;
    out << username << "|" << passHash << "\n";
    return true;
}

bool verifyUser(const string& username, const string& passHash) {
    ifstream in("users.db");
    if (!in) return false;
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        size_t pos = line.find('|');
        if (pos == string::npos) continue;
        string u = line.substr(0, pos);
        string h = line.substr(pos + 1);
        if (u == username && h == passHash) return true;
    }
    return false;
}

string readPasswordMasked(const string& prompt) {
    cout << prompt;
    string password;

#ifdef _WIN32
    while (true) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            cout << "\n";
            break;
        }
        if (ch == 8) {
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        } else if (ch == 0 || ch == 224) {
            _getch();
        } else {
            if (isprint(ch)) {
                password.push_back((char)ch);
                cout << '*';
            }
        }
    }
#else
    termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    password.clear();
    while (true) {
        int ch = getchar();
        if (ch == '\n' || ch == '\r') {
            cout << "\n";
            break;
        }
        if (ch == 127 || ch == 8) {
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
                cout.flush();
            }
        } else if (isprint(ch)) {
            password.push_back((char)ch);
            cout << '*';
            cout.flush();
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return password;
}

int main() {
    const string RESET  = "\033[0m";
    const string BOLD   = "\033[1m";
    const string RED    = "\033[31m";
    const string GREEN  = "\033[32m";
    const string YELLOW = "\033[33m";
    const string BLUE   = "\033[34m";
    const string CYAN   = "\033[36m";
    const string MAG    = "\033[35m";

    char again = 'y';

    while (true) {
        cout << "\n" << CYAN << BOLD
             << "============================================\n"
             << "         LOGIN & REGISTRATION SYSTEM        \n"
             << "============================================\n"
             << RESET;

        cout << BLUE << "1) Register\n"
             << "2) Login\n"
             << "3) Exit\n" << RESET;

        int choice;
        while (true) {
            cout << YELLOW << "Choose option (1-3): " << RESET;
            if (cin >> choice && choice >= 1 && choice <= 3) break;
            cout << RED << "Invalid choice. Try again.\n" << RESET;
            clearInput();
        }

        if (choice == 3) {
            cout << GREEN << BOLD << "\nGoodbye!\n" << RESET;
            break;
        }

        string username;
        cout << "\n" << MAG << "Username rules: 3-20 chars, no spaces, allowed: letters/digits/_/.\n" << RESET;
        while (true) {
            cout << YELLOW << "Enter username: " << RESET;
            cin >> username;
            username = trim(username);
            if (!isValidUsername(username)) {
                cout << RED << "Invalid username. Try again.\n" << RESET;
                continue;
            }
            break;
        }

        if (choice == 1) {
            if (userExists(username)) {
                cout << RED << "\nRegistration failed: Username already exists.\n" << RESET;
            } else {
                cout << MAG << "Password rules: min 6 chars, no spaces, must include Upper+Lower+Digit+Special\n" << RESET;

                string password, confirm;
                while (true) {
                    password = readPasswordMasked(YELLOW + string("Enter password: ") + RESET);
                    if (!isStrongPassword(password)) {
                        cout << RED << "Weak password. Try again.\n" << RESET;
                        continue;
                    }

                    confirm = readPasswordMasked(YELLOW + string("Confirm password: ") + RESET);
                    if (password != confirm) {
                        cout << RED << "Passwords do not match. Try again.\n" << RESET;
                        continue;
                    }
                    break;
                }

                string hash = fnv1a64(password);

                if (saveUser(username, hash)) {
                    cout << GREEN << BOLD << "\nRegistration successful!\n" << RESET;
                } else {
                    cout << RED << "\nError: Could not open database file.\n" << RESET;
                }
            }
        } else if (choice == 2) {
            string password = readPasswordMasked(YELLOW + string("Enter password: ") + RESET);
            string hash = fnv1a64(password);

            if (verifyUser(username, hash)) {
                cout << GREEN << BOLD << "\nLogin successful! Welcome, " << username << "!\n" << RESET;
            } else {
                if (!userExists(username)) {
                    cout << RED << "\nLogin failed: Username not found.\n" << RESET;
                } else {
                    cout << RED << "\nLogin failed: Incorrect password.\n" << RESET;
                }
            }
        }

        while (true) {
            cout << "\n" << CYAN << "Use again? (y/n): " << RESET;
            cin >> again;
            again = (char)tolower(again);
            if (again == 'y' || again == 'n') break;
            cout << RED << "Please enter only y or n.\n" << RESET;
        }

        if (again == 'n') {
            cout << GREEN << BOLD << "\nGoodbye! Thanks for using the system.\n" << RESET;
            break;
        }
    }

    return 0;
}
