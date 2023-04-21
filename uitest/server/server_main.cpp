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
#include <fcntl.h>
#include "ipc_transactor.h"
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
    const std::string VERSION = "4.0.1.3";
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

    static void DumpLayoutImpl(string_view path, bool listWindows, bool initController, ApiCallErr &err)
    {
        ofstream fout;
        fout.open(path, ios::out | ios::binary);
        if (!fout) {
            err = ApiCallErr(ERR_INVALID_INPUT, "Error path:" + string(path) + strerror(errno));
            return;
        }
        if (initController) {
            UiDriver::RegisterController(make_unique<SysUiController>());
        }
        auto driver = UiDriver();
        auto data = nlohmann::json();
        driver.DumpUiHierarchy(data, listWindows, err);
        if (err.code_ != NO_ERROR) {
            fout.close();
            return;
        }
        fout << data.dump();
        fout.close();
        return;
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
        const bool listWindows = params.find('i') != params.end();
        auto err = ApiCallErr(NO_ERROR);
        DumpLayoutImpl(savePath, listWindows, true, err);
        if (err.code_ == NO_ERROR) {
            PrintToConsole("DumpLayout saved to:" + savePath);
            return EXIT_SUCCESS;
        } else if (err.code_ != ERR_INITIALIZE_FAILED) {
            PrintToConsole("DumpLayout failed:" + err.message_);
            return EXIT_FAILURE;
        }
        // Cannot connect to AAMS, broadcast request to running uitest-daemon if any
        err = ApiCallErr(NO_ERROR);
        auto cmd = OHOS::AAFwk::Want();
        cmd.SetParam("savePath", string(savePath));
        cmd.SetParam("listWindows", listWindows);
        ApiTransactor::SendBroadcastCommand(cmd, err);
        if (err.code_ == NO_ERROR) {
            PrintToConsole("DumpLayout saved to:" + savePath);
            return EXIT_SUCCESS;
        } else {
            PrintToConsole("DumpLayout failed:" + err.message_);
            return EXIT_FAILURE;
        }
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
        auto controller = SysUiController();
        stringstream errorRecv;
        auto fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
        if (!controller.TakeScreenCap(fd, errorRecv)) {
            PrintToConsole("ScreenCap failed: " + errorRecv.str());
            return EXIT_FAILURE;
        }
        PrintToConsole("ScreenCap saved to " + savePath);
        (void) close(fd);
        return EXIT_SUCCESS;
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
        UiDriver::RegisterController(make_unique<SysUiController>());
        ApiTransactor apiTransactServer(true);
        if (!apiTransactServer.InitAndConnectPeer(transalatedToken, ApiTransact)) {
            LOG_E("Failed to initialize server");
            return EXIT_FAILURE;
        }
        // accept remopte dump request during deamon running (initController=false)
        ApiTransactor::SetBroadcaseCommandHandler([] (const OHOS::AAFwk::Want &cmd, ApiCallErr &err) {
            DumpLayoutImpl(cmd.GetStringParam("savePath"), cmd.GetBoolParam("listWindows", false), false, err);
        });
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        condition_variable condVar;
        apiTransactServer.SetDeathCallback([&condVar]() {
            condVar.notify_one();
        });
        LOG_I("UiTest-daemon running, pid=%{public}d", getpid());
        condVar.wait(lock);
        LOG_I("Server exit");
        apiTransactServer.Finalize();
        ApiTransactor::UnsetBroadcaseCommandHandler();
        _Exit(0);
        return 0;
    }

    static int32_t UiRecord(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest uiRecord <read|record>";
        if (!(argc == INDEX_THREE || argc == INDEX_FOUR)) {
            PrintToConsole(usage);
            return EXIT_FAILURE;
        }
        std::string opt = argv[TWO];
        std::string modeOpt;
        if (argc == INDEX_FOUR) {
            modeOpt = argv[THREE];
        }
        if (opt == "record") {
            if (!InitEventRecordFile()) {
                return OHOS::ERR_INVALID_VALUE;
            }
            auto controller = make_unique<SysUiController>();
            if (!controller->ConnectToSysAbility()) {
                PrintToConsole("Failed, cannot connect to AMMS ");
                return EXIT_FAILURE;
            }
            UiDriver::RegisterController(move(controller));
            RecordInitEnv(modeOpt);
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
            Timer timer;
            timer.Start(TIMEINTERVAL, Timer::TimerFunc);
            std::cout << "Started Recording Successfully..." << std::endl;
            int flag = getc(stdin);
            std::cout << flag << std::endl;
            constexpr int timeToSleep = 3600;
            sleep(timeToSleep);
            return OHOS::ERR_OK;
        } else if (opt == "read") {
            EventData::ReadEventLine();
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
