#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

// Basic XOR encryption for demonstration
void xorEncryptDecrypt(string &data, char key = 'S') {
    for (char &c : data) c ^= key;
}

// Send file list to client
void sendFileList(int clientSocket) {
    DIR *dir = opendir(".");
    struct dirent *entry;
    string fileList;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // regular file
            fileList += entry->d_name;
            fileList += "\n";
        }
    }
    closedir(dir);

    send(clientSocket, fileList.c_str(), fileList.size(), 0);
}

// Send file to client
void sendFile(int clientSocket, const string &filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        string msg = "ERROR: File not found";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        return;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string fileData = buffer.str();
    xorEncryptDecrypt(fileData); // encrypt before sending
    send(clientSocket, fileData.c_str(), fileData.size(), 0);
    file.close();
}

// Receive file upload from client
void receiveFile(int clientSocket, const string &filename) {
    char buffer[BUFFER_SIZE];
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    string data(buffer, bytesRead);
    xorEncryptDecrypt(data); // decrypt
    ofstream out(filename, ios::binary);
    out << data;
    out.close();
    cout << "File '" << filename << "' uploaded successfully." << endl;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    cout << "Server started on port " << PORT << "..." << endl;
    addr_size = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
    cout << "Client connected." << endl;

    // Basic authentication
    char auth[BUFFER_SIZE];
    recv(clientSocket, auth, BUFFER_SIZE, 0);
    string password(auth);
    if (password != "admin123") {
        string msg = "AUTH_FAIL";
        send(clientSocket, msg.c_str(), msg.size(), 0);
        close(clientSocket);
        return 0;
    } else {
        string msg = "AUTH_OK";
        send(clientSocket, msg.c_str(), msg.size(), 0);
    }

    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        string command(buffer);

        if (command == "LIST") {
            sendFileList(clientSocket);
        } else if (command.rfind("GET ", 0) == 0) {
            string filename = command.substr(4);
            sendFile(clientSocket, filename);
        } else if (command.rfind("PUT ", 0) == 0) {
            string filename = command.substr(4);
            receiveFile(clientSocket, filename);
        } else if (command == "EXIT") {
            cout << "Client disconnected.\n";
            break;
        }
    }

    close(clientSocket);
    close(serverSocket);
    return 0;
}
