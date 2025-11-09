#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

void xorEncryptDecrypt(string &data, char key = 'S') {
    for (char &c : data) c ^= key;
}

// Receive file from server
void receiveFile(int sock, const string &filename) {
    char buffer[BUFFER_SIZE];
    int bytesRead = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytesRead <= 0) {
        cout << "Failed to receive file.\n";
        return;
    }
    string data(buffer, bytesRead);
    xorEncryptDecrypt(data); // decrypt
    ofstream out(filename, ios::binary);
    out << data;
    out.close();
    cout << "File '" << filename << "' downloaded successfully.\n";
}

// Upload file to server
void uploadFile(int sock, const string &filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "File not found.\n";
        return;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    string data = buffer.str();
    xorEncryptDecrypt(data);
    send(sock, data.c_str(), data.size(), 0);
    file.close();
}

int main() {
    int sock;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // Authentication
    string password;
    cout << "Enter server password: ";
    cin >> password;
    send(sock, password.c_str(), password.size(), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    if (string(buffer) == "AUTH_FAIL") {
        cout << "Authentication failed.\n";
        close(sock);
        return 0;
    }
    cout << "Authenticated successfully.\n";

    int choice;
    do {
        cout << "\n=== File Sharing Client Menu ===\n";
        cout << "1. List Files on Server\n";
        cout << "2. Download File\n";
        cout << "3. Upload File\n";
        cout << "4. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        string command, filename;
        switch (choice) {
            case 1:
                command = "LIST";
                send(sock, command.c_str(), command.size(), 0);
                memset(buffer, 0, BUFFER_SIZE);
                recv(sock, buffer, BUFFER_SIZE, 0);
                cout << "\nFiles on server:\n" << buffer << endl;
                break;
            case 2:
                cout << "Enter filename to download: ";
                cin >> filename;
                command = "GET " + filename;
                send(sock, command.c_str(), command.size(), 0);
                receiveFile(sock, filename);
                break;
            case 3:
                cout << "Enter filename to upload: ";
                cin >> filename;
                command = "PUT " + filename;
                send(sock, command.c_str(), command.size(), 0);
                uploadFile(sock, filename);
                break;
            case 4:
                command = "EXIT";
                send(sock, command.c_str(), command.size(), 0);
                cout << "Disconnected from server.\n";
                break;
            default:
                cout << "Invalid choice.\n";
                break;
        }
    } while (choice != 4);

    close(sock);
    return 0;
}
