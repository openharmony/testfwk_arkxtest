/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <chrono>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include "extern_api.h"
#include "ipc_transactors_impl.h"
#include "system_ui_controller.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
    struct option g_longOptions[] = {
        {"path", required_argument, nullptr, 'p'},
    };
    /* *Print to the console of this shell process. */
    static inline void PrintToConsole(string_view message)
    {
        std::cout << message << std::endl;
    }

    static int32_t GetParam(int32_t argc, char *argv[], string_view optstring, string_view usage,
        map<char, string> &params)
    {
        int opt;
        while ((opt = getopt_long(argc, argv, optstring.data(), g_longOptions, nullptr)) != -1) {
            switch (opt) {
                case '?':
                    PrintToConsole(usage);
                    return EXIT_FAILURE;
                    break;
                default:
                    params.insert(pair<char, string>(opt, optarg));
                    break;
            }
        }
        return EXIT_SUCCESS;
    }

    static int32_t DumpLayout(int32_t argc, char *argv[])
    {
        auto ts = to_string(GetCurrentMicroseconds());
        auto savePath = "/data/local/tmp/layout_" + ts + ".json";
        map<char, string> params;
        static constexpr string_view usage = "USAGE: uitestkit dumpLayout -p <path>";
        if (GetParam(argc, argv, "p:", usage, params) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        auto iter = params.find('p');
        if (iter != params.end()) {
            savePath = iter->second;
        }
        auto controller = SysUiController("sys_ui_controller", "");
        if (!controller.ConnectToSysAbility()) {
            PrintToConsole("Dump layout failed, cannot connect to AAMS");
            return EXIT_FAILURE;
        }
        auto data = nlohmann::json();
        controller.GetCurrentUiDom(data);
        ofstream fout;
        fout.open(savePath, ios::out | ios::binary);
        if (!fout) {
            PrintToConsole("Error path:" + savePath);
            return EXIT_FAILURE;
        }
        PrintToConsole("DumpLayout saved to:" + savePath);
        fout << data.dump();
        fout.close();
        return EXIT_SUCCESS;
    }

    static int32_t ScreenCap(int32_t argc, char *argv[])
    {
        auto ts = to_string(GetCurrentMicroseconds());
        auto savePath = "/data/local/tmp/screenCap_" + ts + ".png";
        map<char, string> params;
        static constexpr string_view usage = "USAGE: uitest screenCap -p <path>";
        if (GetParam(argc, argv, "p:", usage, params) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        auto iter = params.find('p');
        if (iter != params.end()) {
            savePath = iter->second;
        }
        auto controller = SysUiController("sys_ui_controller", "");
        stringstream errorRecv;
        if (!controller.TakeScreenCap(savePath, errorRecv)) {
            PrintToConsole("ScreenCap failed: " + errorRecv.str());
            return EXIT_FAILURE;
        }
        PrintToConsole("ScreenCap saved to " + savePath);
        return EXIT_SUCCESS;
    }

    static int32_t StartDaemon(string_view token)
    {
        if (token.empty()) {
            LOG_E("Empty transaction token");
            return EXIT_FAILURE;
        }
        if (daemon(0, 0) != 0) {
            LOG_E("Failed to daemonize current process");
            return EXIT_FAILURE;
        }
        LOG_I("Server starting up");
        TransactionServerImpl server(token);
        if (!server.Initialize()) {
            LOG_E("Failed to initialize server");
            return EXIT_FAILURE;
        }
        // set actual api handlerFunc provided by uitest_core
        server.SetCallFunction(ApiTransact);
        // set delayed UiController
        auto controllerProvider = [](string_view device, list<unique_ptr<UiController>> &receiver) {
            auto controller = make_unique<SysUiController>("sys_ui_controller", device);
            if (controller->ConnectToSysAbility()) {
                receiver.push_back(move(controller));
            }
        };
        UiController::RegisterControllerProvider(controllerProvider);
        LOG_I("UiTest-daemon running, pid=%{public}d", getpid());
        auto exitCode = server.RunLoop();
        LOG_I("Server exited with code: %{public}d", exitCode);
        server.Finalize();
        return exitCode == EXIT_SUCCESS;
    }

    extern "C" int32_t main(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitestkit <screenCap|dumpLayout>";
        if (argc < INDEX_TWO) {
            PrintToConsole("Missing argument");
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
        string command(argv[1]);
        if (command == "dumpLayout") {
            exit(DumpLayout(argc, argv));
        } else if (command == "start-daemon") {
            string_view token = argc < 3 ? "" : argv[2];
            exit(StartDaemon(token));
        } else if (command == "screenCap") {
            exit(ScreenCap(argc, argv));
        } else {
            PrintToConsole("Illegal argument: " + command);
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
    }
}