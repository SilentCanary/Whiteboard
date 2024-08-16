#include <iostream>
#include <winsock2.h>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <sstream>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

mutex client_mutex;
vector<SOCKET> clients;

void initialize_winsock()
{
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0)
    {
        cout << "WSA START UP FAILED !!" << result << endl;
        exit(1);
    }
}

void broadcast_to_clients(string &data)
{
    lock_guard<mutex> lock(client_mutex);
    for (SOCKET client : clients)
    {
        send(client, data.c_str(), static_cast<int>(data.size()), 0);
    }
}

void handle_clients(SOCKET client_socket)
{
    char buffer[1024];
    while (true)
    {
        int bytes_recieved = recv(client_socket, buffer, sizeof(buffer),0);
        if (bytes_recieved > 0)
        {
            string data(buffer, bytes_recieved);
            broadcast_to_clients(data);
        }
        else
        {
            closesocket(client_socket);
            lock_guard<mutex> lock(client_mutex);
            auto it = remove(clients.begin(), clients.end(), client_socket);
            clients.erase(it, clients.end());
            break;
            // clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
        }
    }
}

void server_listen()
{
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        cout << "socket creation failed" << endl;
        return;
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        cout << "Bind failed." << endl;
        return;
    }

    listen(listen_socket, SOMAXCONN);

    cout << "Server listening on port 8080..." << endl;

    while (true)
    {
        SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET)
        {
            cout << "Accept failed." << endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(client_mutex);
            clients.push_back(client_socket);
        }

        thread(handle_clients, client_socket).detach();
    }

    closesocket(listen_socket);
}
int main()
{
   initialize_winsock();
    std::thread(server_listen).detach();

    cout << "Press any key to stop the server..." << endl;
    cin.get();

    WSACleanup();
    return 0;
}