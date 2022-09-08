/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include "qgs_server.h"
#include "qgs_log.h"
#include <iostream>
#include <fstream>
#include <linux/vm_sockets.h>

#define QGS_CONFIG_FILE "/etc/qgs.conf"

using namespace std;
using namespace intel::sgx::dcap::qgs;
volatile bool reload = false;
static QgsServer* server = NULL;

void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGTERM:
            if (server)
            {
                reload = false;
                server->shutdown();
            }
            break;
        case SIGHUP:
            if (server)
            {
                reload = true;
                server->shutdown();
            }
            break;
        default:
            break;
    }
}


int main(int argc, const char* argv[])
{
    bool no_daemon = false;
    unsigned long int port = 0;
    char *endptr = NULL;
    if (argc > 3) {
        cout << "Usage: " << argv[0] << "[--no-daemon] [-p=port_number]"
             << endl;
        exit(1);
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--no-daemon") == 0) {
            cout << "--no-daemon option found, will not run as a daemon"
                 << endl;
            no_daemon = true;
            continue;
        } else if (strncmp(argv[i], "-p=", 3 ) == 0) {
            if (strspn(argv[i] + 3, "0123456789") != strlen(argv[i] + 3)) {
                cout << "Please input valid port number" << endl;
                exit(1);
            }
            errno = 0;
            port = strtoul(argv[i] + 3, &endptr, 10);
            if (errno || strlen(endptr) || (port > UINT_MAX) ) {
                cout << "Please input valid port number" << endl;
                exit(1);
            }
            cout << "port number [" << port << "] found in cmdline" << endl;
            continue;
        } else {
            cout << "Usage: " << argv[0] << "[--no-daemon] [-p=port_number]"
                 << endl;
            exit(1);
        }
    }

    // Use the port number in QGS_CONFIG_FILE if no valid port number on
    // command line
    if (port == 0) {
        ifstream config_file(QGS_CONFIG_FILE);
        if (config_file.is_open()) {
            string line;
            while(getline(config_file, line)) {
                line.erase(remove_if(line.begin(), line.end(), ::isspace),
                        line.end());
                if(line.empty() || line[0] == '#' ) {
                    continue;
                }
                auto delimiterPos = line.find("=");
                if (delimiterPos == std::string::npos) {
                    continue;
                }
                auto name = line.substr(0, delimiterPos);
                if (name.empty()) {
                    cout << "Wrong config format in " << QGS_CONFIG_FILE
                         << endl;
                    exit(1);
                }
                if( name.compare("port") == 0) {
                    errno = 0;
                    endptr = NULL;
                    port = strtoul(line.substr(delimiterPos + 1).c_str(),
                                   &endptr, 10);
                    if (errno || strlen(endptr) || (port > UINT_MAX) ) {
                        cout << "Please input valid port number in "
                             << QGS_CONFIG_FILE << endl;
                        exit(1);
                    }
                }
                // ignore unrecognized configs.
            }
        } else {
          cout << "Failed to open config file " << QGS_CONFIG_FILE << endl;
        }
    }

    if (port == 0) {
        cout << "Please provide valid port number in cmdline or "
             << QGS_CONFIG_FILE << endl;
        exit(1);
    }

    if(!no_daemon && daemon(0, 0) < 0) {
        exit(1);
    }

    QGS_LOG_INIT_EX(no_daemon);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    QGS_LOG_INFO("Added signal handler\n");

    try {
        do {
            reload = false;
            asio::io_service io_service;
            struct sockaddr_vm vm_addr = {};
            vm_addr.svm_family = AF_VSOCK;
            vm_addr.svm_reserved1 = 0;
            vm_addr.svm_port = port & UINT_MAX;
            vm_addr.svm_cid = VMADDR_CID_ANY;
            asio::generic::stream_protocol::endpoint ep(&vm_addr, sizeof(vm_addr));
            QGS_LOG_INFO("About to create QgsServer\n");
            server = new QgsServer(io_service, ep);
            QGS_LOG_INFO("About to start main loop\n");
            io_service.run();
            QGS_LOG_INFO("Quit main loop\n");
            QgsServer *temp_server = server;
            server = NULL;
            QGS_LOG_INFO("Deleted QgsServer object\n");
            delete temp_server;
        } while (reload == true);
    } catch (std::exception &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    QGS_LOG_FINI();
    return 0;
}
