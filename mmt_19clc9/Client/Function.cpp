#include "Header.h"

void ShutDownAndClose(client_type& client) {
    shutdown(client.socket, SD_SEND);
    closesocket(client.socket);
    WSACleanup();
}

void Init(client_type& client) {
    int iResult = 0;
    struct sockaddr_in addrport;
    std::string str;
    std::cout << "Input server ip address: ";
    getline(std::cin, str);
    const char* c = str.c_str();

    WSADATA wsaData;
    int wsOK = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsOK != 0) {
        //std::cout << "Can't initialize winsock. Application is now exiting..." << std::endl;
        return;
    }

    ZeroMemory(&addrport, sizeof(addrport));
    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(5000);
    addrport.sin_addr.s_addr = inet_addr(c);


    /*std::cout << "Setting up server..." << std::endl;
    getaddrinfo(IP_ADDRESS, PORT, &hints, &server);*/


    // Attempt to connect to an address until one succeeds
    while (true) {
        client.socket = socket(PF_INET, SOCK_STREAM, 0);
        iResult = connect(client.socket, (struct sockaddr*)&addrport, sizeof(addrport));
        if (iResult == SOCKET_ERROR) {
            closesocket(client.socket);
            client.socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    if (client.socket == INVALID_SOCKET) {
        ShutDownAndClose(client);
        system("pause");
        return;
    }
}

void Client_Thread(client_type& new_client) {
    while (true) {
        memset(new_client.RecvMsg, NULL, sizeof(new_client.RecvMsg));

        if (new_client.socket != 0) {
            int iResult = recv(new_client.socket, new_client.RecvMsg, DEFAULT_MSG_LENGTH, 0);

            if (iResult != SOCKET_ERROR)
                std::cout << new_client.RecvMsg << std::endl;

            else break;   
            
        }
    }

    if (WSAGetLastError() == WSAECONNRESET)
        std::cout << "The server has disconnected" << std::endl;
}

void Client_Group_Chat(client_type& client) {
    std::string str;
    recv(client.socket, client.RecvMsg, DEFAULT_MSG_LENGTH, 0);

    if (strcmp(client.RecvMsg, "Server is full") != 0)
    {

        std::thread my_thread(Client_Thread, std::ref(client));

        while (true)
        {
            getline(std::cin, str);

            //Encryt message before send
            //sent_message = string_to_hex(sent_message);



            if (str.compare("exit") == 0) break;    // De y dong nay, sau nay se chinh lai neu client muon out group chat

            int iResult = send(client.socket, str.c_str(), strlen(str.c_str()), 0);

            if (iResult <= 0) break;
        }

        client.socket = INVALID_SOCKET;

        my_thread.detach();
    }

    else
        std::cout << client.RecvMsg << std::endl;

    std::cout << "Shutting down socket..." << std::endl;

    ShutDownAndClose(client);
}

void Client_Private_Thread(client_type& new_client) {
    while (true) {
        memset(new_client.RecvMsg, NULL, sizeof(new_client.RecvMsg));

        if (new_client.socket != 0) {
            int iResult = recv(new_client.socket, new_client.RecvMsg, DEFAULT_MSG_LENGTH, 0);

            if (iResult != SOCKET_ERROR) {
                if (strcmp(new_client.RecvMsg, "send file") == 0) {

                    string dest_temp;
                    TCHAR appData[MAX_PATH];
                    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, appData))) {
                        wstring test(&appData[0]);
                        string test2(test.begin(), test.end());
                        dest_temp = test2;
                    }

                    //Receive file name and size
                    iResult = recv(new_client.socket, new_client.RecvMsg, DEFAULT_MSG_LENGTH, 0);
                    if (iResult == SOCKET_ERROR)
                        continue;
                    dest_temp.append("\\").append(new_client.RecvMsg);
                    memset(new_client.RecvMsg, NULL, sizeof(new_client.RecvMsg));

                    iResult = recv(new_client.socket, new_client.RecvMsg, DEFAULT_MSG_LENGTH, 0);
                    if (iResult == SOCKET_ERROR)
                        return;
                    long long int file_size = stoll(std::string(new_client.RecvMsg));
                    memset(new_client.RecvMsg, NULL, sizeof(new_client.RecvMsg));



                    /*cout << "Do you want download file? (Y/N): ";
                    char c = _getch();
                    if (c == 'y' || c == 'Y') {
                        fflush(stdin);
                    }
                    else return false;*/


                    //Receive buffer processing
                    ofstream fs;
                    fs.open(dest_temp, ios::binary | ios::trunc);

                    if (fs.fail()) {
                        continue;
                    }

                    while (true) {
                        char* buffer = new char[DEFAULT_BUFFER_SIZE];
                        iResult = recv(new_client.socket, buffer, DEFAULT_BUFFER_SIZE, 0);
                        if (iResult == SOCKET_ERROR)
                            break;
                        else if (strcmp(buffer, "end") == 0) {
                            fs.close();
                            std::cout << "Completed" << std::endl;
                            break;
                        }
                        else if (iResult < DEFAULT_BUFFER_SIZE) {
                            char* buffer2 = new char[iResult];
                            strncpy(buffer2, buffer, iResult);
                            fs.write(buffer2, iResult);
                            delete[] buffer2;
                        }
                        else 
                            fs.write(buffer, DEFAULT_BUFFER_SIZE);
                        delete[] buffer;
                    }
                }
                else std::cout << new_client.RecvMsg << std::endl;
            }
            else break;
        }
    }

    if (WSAGetLastError() == WSAECONNRESET)
        std::cout << "The server has disconnected" << std::endl;
}

void Client_Private_Chat(client_type& client) {
    std::string str;

    std::thread my_thread(Client_Private_Thread, std::ref(client));

    while (true)
    {
        getline(std::cin, str);

        //Encryt message before send
        //sent_message = string_to_hex(sent_message);

        if (str.compare("exit") == 0) break;    // De y dong nay, sau nay se chinh lai neu client muon out chat

        int iResult = send(client.socket, str.c_str(), strlen(str.c_str()), 0);

        if (iResult <= 0) break;

        if (str.compare("upload file") == 0) {
            std::cout << "File path: ";
            std::getline(std::cin, str);    

            std::string fName = "";

            for (int i = 0; i < str.length(); i++) {
                if (str[i] == '/') str.replace(i, 1, "\\");
            }

            for (int32_t i = str.std::string::length() - 1; i >= 0; i--) {
                if (str[i] == '\\') {
                    fName.std::string::append(str.std::string::substr(i + 1));
                    break;
                }
            }

            std::ifstream fp;
            fp.open(str, ios::binary);

            if (fp.fail()) {
                continue;
            }

            // Send file name and file size
            fp.seekg(0, ios::end);
            long long int size = fp.tellg();
            fp.seekg(0, ios::beg);

            send(client.socket, fName.c_str(), strlen(fName.c_str()), 0);
            sleep_for(nanoseconds(10));
            send(client.socket, std::to_string(size).c_str(), strlen(std::to_string(size).c_str()), 0);
            sleep_for(nanoseconds(10));


            //Sending file processing
            long long int sizetemp = size;
            while (sizetemp > 0) {
                if (sizetemp < DEFAULT_BUFFER_SIZE) {
                    char* buffer = new char[sizetemp];
                    fp.read(buffer, sizetemp);
                    iResult = send(client.socket, buffer, sizetemp, 0);
                    sleep_for(nanoseconds(10));
                    while (iResult == SOCKET_ERROR || iResult != sizetemp) 
                        iResult = send(client.socket, buffer, sizetemp, 0);
                    delete[] buffer;
                    sizetemp = 0;
                }
                else {
                    char* buffer = new char[DEFAULT_BUFFER_SIZE];
                    fp.read(buffer, DEFAULT_BUFFER_SIZE);
                    iResult = send(client.socket, buffer, DEFAULT_BUFFER_SIZE, 0);
                    sleep_for(nanoseconds(10));
                    while (iResult == SOCKET_ERROR || iResult != DEFAULT_BUFFER_SIZE)
                        iResult = send(client.socket, buffer, DEFAULT_BUFFER_SIZE, 0);
                    delete[] buffer;
                    sizetemp -= DEFAULT_BUFFER_SIZE;
                }
            }
            fp.close();
            sleep_for(nanoseconds(10));
            std::cout << "Upload completed" << std::endl;
            iResult = send(client.socket, "end", 4, 0);
        }
    }

    client.socket = INVALID_SOCKET;

    my_thread.detach();

    std::cout << "Shutting down socket..." << std::endl;

    ShutDownAndClose(client);
}


