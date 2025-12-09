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
#include <sys/statfs.h>
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
#include <cstdio>
#include "ipc_transactor.h"
#include "system_ui_controller.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "ui_driver.h"
#include "ui_record.h"
#include "ui_input.h"
#include "ui_model.h"
#include "extension_executor.h"
#include "hisysevent.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
    const std::string HELP_MSG =
    "usage: uitest <command> [options]                                                                          \n"
    "help                                                                                    print help messages\n"
    "screenCap                                                                        capture the current screen\n"
    "  -p <savePath>                                                                      specifies the savePath\n"
    "  -d <displayId>                                                                       specifies the screen\n"
    "dumpLayout                                                               get the current layout information\n"
    "  -p <savePath>                                                                      specifies the savePath\n"
    "  -i                                                                     not merge windows and filter nodes\n"
    "  -a                                                                                include font attributes\n"
    "  -b <bundleName>                                             specifies the bundleName of the target window\n"
    "  -w <windowId>                                                specifies the window id of the target window\n"
    "  -m <true/false>          whether merge windows, true means to merge, set it true when not use this option\n"
    "  -d <displayId>                                           specifies the locate screen of the target window\n"
    "start-daemon <token>                                                                 start the test process\n"
    "uiRecord                                                                            recording Ui Operations\n"
    "  record                                                           Write Ui event information into csv file\n"
    "    -W <true/false>                                     whether save widget information, true means to save\n" 
    "                                                                       set it true when not use this option\n"
    "    -l                                             Save the current layout information after each operation\n"
    "    -c <true/false>              whether print the Ui event information to the console, true means to print\n" 
    "                                                                       set it true when not use this option\n"
    "  read                                                                    print file content to the console\n"
    "uiInput                                                                     inject Ui simulation operations\n"
    "  help                                                                                  print uiInput usage\n"
    "  dircFling  <direction> [velocity] [stepLength]      direction ranges from 0,1,2,3 (left, right, up, down)\n"
    "  click/doubleClick/longClick <x> <y>                                       click on the target coordinates\n"
    "  swipe/drag <from_x> <from_y> <to_x> <to_y> [velocity]      velocity ranges from 200 to 40000, default 600\n"
    "  fling <from_x> <from_y> <to_x> <to_y> [velocity] [stepLength]   velocity ranges from 200 to 40000, default 600\n"
    "  keyEvent <keyID/Back/Home/Power>                                                          inject keyEvent\n"
    "  keyEvent <keyID_0> <keyID_1> [keyID_2]                                           keyID_2 default to None \n"
    "  inputText <x> <y> <text>                                         inputText at the target coordinate point\n"
    "  text <text>                                           input text at the location where is already focused\n"
    "--version                                                                        print current tool version\n";

    const std::string VERSION = "6.0.2.2";
    struct option g_longoptions[] = {
        {nullptr, required_argument, nullptr, 'p'},
        {nullptr, required_argument, nullptr, 'd'},
        {nullptr, no_argument, nullptr, 'i'},
        {nullptr, no_argument, nullptr, 'a'},
        {nullptr, required_argument, nullptr, 'b'},
        {nullptr, required_argument, nullptr, 'w'},
        {nullptr, required_argument, nullptr, 'm'},
        {nullptr, 0, nullptr, 0}};
    /* *Print to the console of this shell process. */
    static inline void PrintToConsole(string_view message)
    {
        std::cout << message << std::endl;
    }

    static bool IsUiTestCreatedFile(const string fileName)
    {
        if (fileName.find("layout_") == 0 && fileName.rfind(".json") == fileName.size() - FIVE) {
            return true;
        }
        if (fileName.find("screenCap_") == 0 && fileName.rfind(".png") == fileName.size() - FOUR) {
            return true;
        }
        if (fileName == "record.csv") {
            return true;
        }
        return false;
    }

    static uint64_t GetFileSize(const string filePath)
    {
        struct stat fileStat;
        uint64_t fileSize = stat(filePath.c_str(), &fileStat) ? 0 : static_cast<uint64_t>(fileStat.st_size);
        return fileSize;
    }

    static uint64_t GetUiTestCreatedFileSize(const string newFilePath, string dirName)
    {
        uint64_t createdFileSize = 0;
        DIR* dir = opendir(dirName.c_str());
        if (!dir) {
            LOG_E("Open dir %{public}s failed.", dirName.c_str());
            return createdFileSize;
        }
        if (newFilePath != "") {
            createdFileSize += GetFileSize(newFilePath);
        }
        struct dirent* file;
        while ((file = readdir(dir)) != nullptr) {
            if (file->d_type == DT_REG && IsUiTestCreatedFile(file->d_name)) {
                string filePath = dirName + "/" + file->d_name;
                if (filePath != newFilePath) {
                    createdFileSize += GetFileSize(filePath);
                }
            }
        }
        closedir(dir);
        return createdFileSize;
    }

    static void ReportFileWriteEvent(string newFilePath)
    {
        string partitionName = "/data";
        string dirName = "/data/local/tmp";
        struct statfs partitionStat;
        if (statfs(partitionName.c_str(), &partitionStat) != 0) {
            LOG_E("Get remain partition size failed, partitionName = %{public}s", partitionName.c_str());
            return;
        }
        constexpr double units = 1024.0;
        double remainPartitionSize = (static_cast<double>(partitionStat.f_bfree) / units) *
                                     (static_cast<double>(partitionStat.f_bsize) / units);
        uint64_t createdFileSize = GetUiTestCreatedFileSize(newFilePath, dirName);
        vector<std::string> filePaths = { dirName };
        vector<uint64_t> fileSizes = { createdFileSize };
        HiSysEventWrite(HiviewDFX::HiSysEvent::Domain::FILEMANAGEMENT, "USER_DATA_SIZE",
            HiviewDFX::HiSysEvent::EventType::STATISTIC,
            "COMPONENT_NAME", "arkxtest",
            "PARTITION_NAME", partitionName,
            "REMAIN_PARTITION_SIZE", remainPartitionSize,
            "FILE_OR_FOLDER_PATH", filePaths,
            "FILE_OR_FOLDER_SIZE", fileSizes);
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
                case 'a':
                    params.insert(pair<char, string>(opt, "true"));
                    break;
                case 'l':
                    params.insert(pair<char, string>(opt, "true"));
                    break;
                case 'c':
                case 'W':
                case 'm':
                    if (strcmp(optarg, "true") && strcmp(optarg, "false")) {
                        PrintToConsole("Invalid params");
                        PrintToConsole(usage);
                        return EXIT_FAILURE;
                    }
                    params.insert(pair<char, string>(opt, optarg));
                    break;
                default:
                    params.insert(pair<char, string>(opt, optarg));
                    break;
            }
        }
        return EXIT_SUCCESS;
    }

    static void DumpLayoutImpl(const DumpOption &option, bool initController, ApiCallErr &err)
    {
        ofstream fout;
        fout.open(option.savePath_, ios::out | ios::binary);
        if (!fout) {
            err = ApiCallErr(ERR_INVALID_INPUT, "Error path:" + string(option.savePath_) + strerror(errno));
            return;
        }
        if (initController) {
            UiDriver::RegisterController(make_unique<SysUiController>());
        }
        auto driver = UiDriver();
        auto data = nlohmann::json();
        driver.DumpUiHierarchy(data, option, err);
        if (err.code_ != NO_ERROR) {
            fout.close();
            return;
        }
        string dumpStr = data.dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
        LOG_D("dumpStr size = %{public}zu", dumpStr.size());
        fout << dumpStr;
        if (fout.fail()) {
            LOG_E("Write dumpStr to file failed.");
        } else if (fout.bad()) {
            LOG_E("Error in write dumpStr to file.");
        }
        fout.close();
        return;
    }

    static int32_t DumpLayout(int32_t argc, char *argv[])
    {
        DumpOption option;
        auto ts = to_string(GetCurrentMicroseconds());
        auto savePath = "/data/local/tmp/layout_" + ts + ".json";
        map<char, string> params;
        if (GetParam(argc, argv, "p:w:b:m:d:ia", HELP_MSG, params) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        auto iter = params.find('p');
        option.savePath_ = (iter != params.end()) ? iter->second : savePath;
        auto iter2 = params.find('w');
        option.windowId_ = (iter2 != params.end()) ? iter2->second : "";
        auto iter3 = params.find('b');
        option.bundleName_ = (iter3 != params.end()) ? iter3->second : "";
        option.listWindows_ = params.find('i') != params.end();
        option.addExternAttr_ = params.find('a') != params.end();
        auto iter4 = params.find('m');
        option.notMergeWindow_ = (iter4 != params.end()) ? iter4->second == "false" : false;
        auto iter5 = params.find('d');
        option.displayId_ = (iter5 != params.end()) ? std::atoi(iter5->second.c_str()): 0;
        if (option.listWindows_ && option.addExternAttr_) {
            PrintToConsole("The -a and -i options cannot be used together.");
            return EXIT_FAILURE;
        }
        auto err = ApiCallErr(NO_ERROR);
        DumpLayoutImpl(option, true, err);
        if (err.code_ == NO_ERROR) {
            PrintToConsole("DumpLayout saved to:" + option.savePath_);
            ReportFileWriteEvent(option.savePath_);
            return EXIT_SUCCESS;
        } else if (err.code_ != ERR_INITIALIZE_FAILED) {
            PrintToConsole("DumpLayout failed:" + err.message_);
            return EXIT_FAILURE;
        }
        // Cannot connect to AAMS, broadcast request to running uitest-daemon if any
        err = ApiCallErr(NO_ERROR);
        auto cmd = OHOS::AAFwk::Want();
        cmd.SetParam("savePath", string(option.savePath_));
        cmd.SetParam("listWindows", option.listWindows_);
        cmd.SetParam("addExternAttr", option.addExternAttr_);
        cmd.SetParam("bundleName", string(option.bundleName_));
        cmd.SetParam("windowId", string(option.windowId_));
        cmd.SetParam("mergeWindow", option.notMergeWindow_);
        cmd.SetParam("displayId", to_string(option.displayId_));
        ApiTransactor::SendBroadcastCommand(cmd, err);
        if (err.code_ == NO_ERROR) {
            PrintToConsole("DumpLayout saved to:" + option.savePath_);
        } else {
            PrintToConsole("DumpLayout failed:" + err.message_);
        }
        return err.code_ == NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    static int32_t ScreenCap(int32_t argc, char *argv[])
    {
        auto ts = to_string(GetCurrentMicroseconds());
        auto savePath = "/data/local/tmp/screenCap_" + ts + ".png";
        auto displayId = 0;
        map<char, string> params;
        static constexpr string_view usage = "USAGE: uitest screenCap -p <path>";
        if (GetParam(argc, argv, "p:d:", usage, params) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        auto iter = params.find('p');
        if (iter != params.end()) {
            savePath = iter->second;
        }
        auto iter2 = params.find('d');
        if (iter2 != params.end()) {
            displayId = std::atoi(iter2->second.c_str());
        }
        auto controller = SysUiController();
        stringstream errorRecv;
        int32_t fd = open(savePath.c_str(), O_RDWR | O_CREAT, 0666);
        if (!controller.TakeScreenCap(fd, errorRecv, displayId)) {
            PrintToConsole("ScreenCap failed: " + errorRecv.str());
            return EXIT_FAILURE;
        }
        PrintToConsole("ScreenCap saved to " + savePath);
        ReportFileWriteEvent(savePath);
        return EXIT_SUCCESS;
    }

    static string TranslateToken(string_view raw)
    {
        if (raw.find_first_of('@') != string_view::npos) {
            return string(raw);
        }
        return "default";
    }

    static DumpOption GetOptionForCmd(const OHOS::AAFwk::Want &cmd)
    {
        DumpOption option;
        option.savePath_ = cmd.GetStringParam("savePath");
        option.listWindows_ = cmd.GetBoolParam("listWindows", false);
        option.addExternAttr_ = cmd.GetBoolParam("addExternAttr", false);
        option.bundleName_ = cmd.GetStringParam("bundleName");
        option.windowId_ = cmd.GetStringParam("windowId");
        option.notMergeWindow_ = cmd.GetBoolParam("mergeWindow", true);
        option.displayId_ = atoi(cmd.GetStringParam("displayId").c_str());
        return option;
    }

    static int32_t StartDaemon(string_view token, int32_t argc, char *argv[])
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
        // accept remopte dump request during deamon running (initController=false)
        ApiTransactor::SetBroadcastCommandHandler([] (const OHOS::AAFwk::Want &cmd, ApiCallErr &err) {
            auto option = GetOptionForCmd(cmd);
            DumpLayoutImpl(option, false, err);
        });
        if (token == "singleness") {
            ExecuteExtension(VERSION, argc, argv);
            LOG_I("Server exit");
            ApiTransactor::UnsetBroadcastCommandHandler();
            _Exit(0);
            return 0;
        }
        ApiTransactor apiTransactServer(true);
        auto &apiServer = FrontendApiServer::Get();
        auto apiHandler = std::bind(&FrontendApiServer::Call, &apiServer, placeholders::_1, placeholders::_2);
        auto cbHandler = std::bind(&ApiTransactor::Transact, &apiTransactServer, placeholders::_1, placeholders::_2);
        apiServer.SetCallbackHandler(cbHandler); // used for callback from server to client
        if (!apiTransactServer.InitAndConnectPeer(transalatedToken, apiHandler)) {
            LOG_E("Failed to initialize server");
            ApiTransactor::UnsetBroadcastCommandHandler();
            _Exit(0);
            return EXIT_FAILURE;
        }
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
        ApiTransactor::UnsetBroadcastCommandHandler();
        _Exit(0);
        return 0;
    }

    static int32_t UiRecord(int32_t argc, char *argv[])
    {
        if ((size_t)argc < INDEX_THREE) {
            PrintToConsole("Missing parameter. \n");
            PrintToConsole(HELP_MSG);
            return EXIT_FAILURE;
        }
        std::string opt = argv[TWO];
        RecordOption option;
        if ((size_t)argc >= INDEX_FOUR) {
            if (strcmp(argv[THREE], "point") == 0) {
                option.saveWidget = false;
            }
        }
        if (opt == "record") {
            map<char, string> params;
            if (GetParam(argc, argv, "W:c:l", HELP_MSG, params) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }
            option.saveLayout = params.find('l') != params.end();
            auto iter = params.find('c');
            option.terminalCout = (iter != params.end()) ?iter->second == "true" : true;
            if (option.saveWidget) {
                auto w = params.find('W');
                option.saveWidget = (w != params.end()) ?w->second == "true" : true;
            }
            auto controller = make_unique<SysUiController>();
            ApiCallErr error = ApiCallErr(NO_ERROR);
            if (option.saveWidget || option.saveLayout) {
                if(!controller->ConnectToSysAbility(error)){
                    PrintToConsole(error.message_);
                    return EXIT_FAILURE;
                }
            }
            UiDriver::RegisterController(move(controller));
            ReportFileWriteEvent("");
            return UiDriverRecordStart(option);
        } else if (opt == "read") {
            EventData::ReadEventLine();
            return OHOS::ERR_OK;
        } else {
            PrintToConsole("Illegal argument: " + opt);
            PrintToConsole(HELP_MSG);
            return EXIT_FAILURE;
        }
    }

    static int32_t UiInput(int32_t argc, char *argv[])
    {
        if ((size_t)argc < INDEX_FOUR) {
            std::cout << "Missing parameter. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
        if ((string)argv[THREE] == "help") {
            PrintInputMessage();
            return OHOS::ERR_OK;
        }
        auto controller = make_unique<SysUiController>();
        UiDriver::RegisterController(move(controller));
        return UiActionInput(argc, argv);
    }

    extern "C" int32_t main(int32_t argc, char *argv[])
    {
        if ((size_t)argc < INDEX_TWO) {
            PrintToConsole("Missing argument");
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_FAILURE);
        }
        string command(argv[1]);
        if (command == "dumpLayout") {
            _Exit(DumpLayout(argc, argv));
        } else if (command == "start-daemon") {
            string_view token = argc < 3 ? "" : argv[2];
            _Exit(StartDaemon(token, argc - THREE, argv + THREE));
        } else if (command == "screenCap") {
            _Exit(ScreenCap(argc, argv));
        } else if (command == "uiRecord") {
            _Exit(UiRecord(argc, argv));
        } else if (command == "uiInput") {
            _Exit(UiInput(argc, argv));
        } else if (command == "--version") {
            PrintToConsole(VERSION);
            _Exit(EXIT_SUCCESS);
        } else if (command == "help") {
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_SUCCESS);
        } else {
            PrintToConsole("Illegal argument: " + command);
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_FAILURE);
        }
    }
}
