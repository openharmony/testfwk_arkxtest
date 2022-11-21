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
#include <dirent.h>
#include <sys/stat.h>
#include <typeinfo>
#include <cstring>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <ctime>
#include <condition_variable>
#include <cmath>
#include <string>
#include <vector>
#include <cmath>
#include "ipc_transactors_impl.h"
#include "system_ui_controller.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "ui_driver.h"
#include "ui_record.h"
using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
    const std::string HELP_MSG =
    "   help,                                            print help messages\n"
    "   screenCap,                                                          \n"
    "   dumpLayout,                                                         \n"
    "   uiRecord -record,    wirte location coordinates of events into files\n"
    "   uiRecord -read,                    print file content to the console\n"
    "   --version,                                print current tool version\n";
    const std::string VERSION = "3.2.4.0";

    struct option g_longoptions[] = {
        {"save file in this path", required_argument, nullptr, 'p'},
        {"dump all UI trees in json array format", no_argument, nullptr, 'I'}
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
        while ((opt = getopt_long(argc, argv, optstring.data(), g_longoptions, nullptr)) != -1) {
            switch (opt) {
                case '?':
                    PrintToConsole(usage);
                    return EXIT_FAILURE;
                case 'i':
                    params.insert(pair<char, string>(opt, "true"));
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
        if (GetParam(argc, argv, "p:i", usage, params) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        auto iter = params.find('p');
        if (iter != params.end()) {
            savePath = iter->second;
        }
        ofstream fout;
        fout.open(savePath, ios::out | ios::binary);
        if (!fout) {
            PrintToConsole("Error path:" + savePath);
            return EXIT_FAILURE;
        }
        auto controller = make_unique<SysUiController>("sys_ui_controller");
        if (!controller->ConnectToSysAbility()) {
            PrintToConsole("Dump layout failed, cannot connect to AAMS");
            fout.close();
            return EXIT_FAILURE;
        }
        if (params.find('i') != params.end()) {
            vector<pair<Window, nlohmann::json>> datas;
            controller->GetUiHierarchy(datas);
            auto array = nlohmann::json::array();
            for (auto& data : datas) {
                array.push_back(data.second);
            }
            fout << array.dump();
        } else {
            UiController::RegisterController(move(controller), Priority::MEDIUM);
            auto data = nlohmann::json();
            auto driver = UiDriver();
            auto error = ApiCallErr(NO_ERROR);
            driver.DumpUiHierarchy(data, error);
            if (error.code_ != NO_ERROR) {
                PrintToConsole("Dump layout failed: " + error.message_);
                fout.close();
                return EXIT_FAILURE;
            }
            fout << data.dump();
        }
        PrintToConsole("DumpLayout saved to:" + savePath);
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
        auto controller = SysUiController("sys_ui_controller");
        stringstream errorRecv;
        if (!controller.TakeScreenCap(savePath, errorRecv)) {
            PrintToConsole("ScreenCap failed: " + errorRecv.str());
            return EXIT_FAILURE;
        }
        PrintToConsole("ScreenCap saved to " + savePath);
        return EXIT_SUCCESS;
    }
    static string TrasnlateAppFilePath(string_view raw)
    {
        constexpr uint32_t uid2AccountDivisor = 200000;
        constexpr string_view XDEVICE_AGENT_TOKEN = "0123456789";
        if (access(raw.data(), F_OK) == 0) {
            return string(raw);
        } else if (raw == XDEVICE_AGENT_TOKEN) {
            return "/data/app/el2/100/base/com.ohos.devicetest/cache/shmf";
        }
        string procName;
        string procPid;
        string procAccount;
        string procArea;
        size_t tokenIndex = 0;
        size_t tokenStart = 0;
        size_t tokenEnd = raw.find_first_of('@');
        if (tokenEnd == string_view::npos) {
            LOG_I("Apply raw token: %{public}s", raw.data());
            return string(raw);
        }
        while (true) {
            if (tokenEnd == string_view::npos) {
                procArea = string("el") + to_string(stoi(raw.substr(tokenStart).data()) + INDEX_ONE);
                break;
            }
            string token = string(raw.data() + tokenStart, tokenEnd - tokenStart);
            if (tokenIndex == INDEX_ZERO) {
                procName = token;
            } else if (tokenIndex == INDEX_ONE) {
                procPid = token;
            } else if (tokenIndex == INDEX_TWO) {
                procAccount = to_string(stoi(token.c_str()) / uid2AccountDivisor);
            }
            tokenIndex++;
            tokenStart = tokenEnd + 1;
            tokenEnd = raw.find_first_of('@', tokenStart);
        }
        stringstream builder;
        builder << "/data/app/" << procArea << "/" << procAccount;
        builder << "/base/" << procName << "/cache/shmf_" << procPid;
        return builder.str();
    }
    static int32_t StartDaemon(string_view token)
    {
        if (token.empty()) {
            LOG_E("Empty transaction token");
            return EXIT_FAILURE;
        }
        auto shmfPath = TrasnlateAppFilePath(token);
        if (daemon(0, 0) != 0) {
            LOG_E("Failed to daemonize current process");
            return EXIT_FAILURE;
        }
        LOG_I("Server starting up");
        TransactionServerImpl server(shmfPath);
        if (!server.Initialize()) {
            LOG_E("Failed to initialize server");
            return EXIT_FAILURE;
        }
        // set actual api handlerFunc provided by uitest_core
        server.SetCallFunction(ApiTransact);
        // set delayed UiController
        auto controllerProvider = [](list<unique_ptr<UiController>> &receiver) {
            auto controller = make_unique<SysUiController>("sys_ui_controller");
            if (controller->ConnectToSysAbility()) {
                receiver.push_back(move(controller));
            }
        };
        UiController::RegisterControllerProvider(controllerProvider);
        LOG_I("UiTest-daemon running, pid=%{public}d", getpid());
        auto exitCode = server.RunLoop();
        LOG_I("Server exiting with code: %{public}d", exitCode);
        server.Finalize();
        _Exit(exitCode);
        return exitCode;
    }

    static void TimerFunc()
    {
        int t = GetMillisTime();
        int diff = t - g_touchtime;
        if (diff >= TIMEINTERVAL) {
            std::cout << "No operation detected for 5 seconds, press ctrl + c to save this file?" << std::endl;
        }
    }

    static int32_t UiRecord(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest uiRecord <read|record>";
        if (!(argc == INDEX_THREE)) {
            PrintToConsole(usage);
            return EXIT_FAILURE;
        }
        std::string opt = argv[2];
        if (opt == "record") {
            Timer timer;
            timer.Start(TIMEINTERVAL, TimerFunc);
            if (!InitEventRecordFile(g_outFile)) {
                return OHOS::ERR_INVALID_VALUE;
            }
            auto callBackPtr = InputEventCallback::GetPtr();
            if (callBackPtr == nullptr) {
                std::cout << "nullptr" << std::endl;
                return OHOS::ERR_INVALID_VALUE;
            }
            int32_t id1 = MMI::InputManager::GetInstance()->AddMonitor(callBackPtr);
            if (id1 == -1) {
                std::cout << "Startup Failed!" << std::endl;
                return OHOS::ERR_INVALID_VALUE;
            }
            std::cout << "Started Recording Successfully..." << std::endl;
            int flag = getc(stdin);
            std::cout << flag << std::endl;
            constexpr int timeToSleep = 3600;
            sleep(timeToSleep);
            return OHOS::ERR_OK;
        } else if (opt == "read") {
            std::ifstream inFile(defaultDir + "/" + "record.csv");
            EventData::ReadEventLine(inFile);
            return OHOS::ERR_OK;
        } else {
            PrintToConsole(usage);
            return EXIT_FAILURE;
        }
    }

    extern "C" int32_t main(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest <help|screenCap|dumpLayout|uiRecord|--version>";
        if ((size_t)argc < INDEX_TWO) {
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
        } else if (command == "uiRecord") {
            exit(UiRecord(argc, argv));
        } else if (command == "--version") {
            PrintToConsole(VERSION);
            exit(EXIT_SUCCESS);
        } else if (command == "help") {
            PrintToConsole(HELP_MSG);
            exit(EXIT_SUCCESS);
        } else {
            PrintToConsole("Illegal argument: " + command);
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
    }
}
