/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <getopt.h>
#include <mutex>
#include <condition_variable>
#include "ipc_transactor.h"
#include "api_caller_client.h"
#include "api_caller_server.h"
#include "frontend_api_handler.h"

using namespace std;

namespace OHOS::perftest {
    const std::string HELP_MSG =
        "usage: perftest <command> [options]                                                                        \n"
        "help                                                                                    print help messages\n"
        "start-daemon <token>                                                             start the perftest process\n";

    static inline void PrintToConsole(string_view message)
    {
        std::cout << message << std::endl;
    }

    static string TranslateToken(string_view raw)
    {
        if (raw.find_first_of('@') != string_view::npos) {
            return string(raw);
        }
        return "default";
    }

    static int32_t StartDaemon(string_view token)
    {
        if (token.empty()) {
            LOG_E("Empty transaction token");
            return EXIT_FAILURE;
        }
        auto transalatedToken = TranslateToken(token);
        if (daemon(0, 0) != 0) {
            LOG_E("Failed to daemonize current process");
            return EXIT_FAILURE;
        }
        LOG_I("Server starting up");
        ApiCallerServer apiCallerServer;
        auto &apiServer = FrontendApiServer::Get();
        auto apiHandler = std::bind(&FrontendApiServer::Call, &apiServer, placeholders::_1, placeholders::_2);
        auto cbHandler = std::bind(&ApiCallerServer::Transact, &apiCallerServer, placeholders::_1, placeholders::_2);
        apiServer.SetCallbackHandler(cbHandler); // used for callback from server to client
        if (!apiCallerServer.InitAndConnectPeer(transalatedToken, apiHandler)) {
            LOG_E("Failed to initialize server");
            _Exit(0);
            return EXIT_FAILURE;
        }
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        condition_variable condVar;
        apiCallerServer.SetDeathCallback([&condVar]() {
            condVar.notify_one();
        });
        LOG_I("PerfTest-daemon running, pid=%{public}d", getpid());
        condVar.wait(lock);
        LOG_I("Server exit");
        apiCallerServer.Finalize();
        _Exit(0);
        return 0;
    }

    extern "C" int32_t main(int32_t argc, char *argv[])
    {
        LOG_I("Server command start");
        if ((size_t)argc < TWO) {
            PrintToConsole("Missing argument");
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_FAILURE);
        }
        string command(argv[INDEX_ONE]);
        if (command == "start-daemon") {
            string_view token = argc < THREE ? "" : argv[INDEX_TWO];
            if (StartDaemon(token) == EXIT_FAILURE) {
                PrintToConsole("Start daemon failed");
                _Exit(EXIT_FAILURE);
            }
            _Exit(EXIT_SUCCESS);
        } else if (command == "help") {
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_SUCCESS);
        }
        PrintToConsole("Illegal argument: " + command);
        PrintToConsole(HELP_MSG);
        _Exit(EXIT_FAILURE);
    }
} // namespace OHOS::perftest