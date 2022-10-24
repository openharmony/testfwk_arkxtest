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
#include <regex>
#include <unistd.h>
#include <typeinfo>
#include <cstring>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>
#include <condition_variable>
#include <cmath>
#include "ipc_transactors_impl.h"
#include "system_ui_controller.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "ui_driver.h"
#define TWO 2

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
    const std::string VERSION = "3.2.3.2";
    int g_touchtime;
    int g_timeindex = 1000;
    int g_timeinterval = 5000;
    int g_maxdistance = 220;
    int g_length = 20;
    int g_velocity = 600;
    int g_dragMonitor = 2;
    std::ofstream g_outfile;
    std::string g_operationType[6] = {"click", "longClick", "doubleClick", "swipe", "drag", "fling"};
    vector<MMI::PointerEvent::PointerItem> g_eventsvector;
    vector<int> g_timesvector;
    vector<int> g_mmitimesvector;
    enum GTouchop : uint8_t {CLICK_ = 0, LONG_CLICK_, DOUBLE_CLICK_, SWIPE_, DRAG_, FLING_};
    enum GCaseInfo : uint8_t {Type = 0, XPosi, YPosi, X2Posi, Y2Posi, Interval, Length, Velocity };
    GTouchop g_touchop = CLICK_;
    bool g_isClick = false;
    uint g_clickEventCount = 0;
    namespace {
        std::string g_defaultDir = "/data/local/tmp/layout";
        int64_t GetMillisTime()
        {
            auto timeNow = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch());
            return tmp.count();
        }

        class TestUtils {
        public:
            TestUtils() = default;
            virtual ~TestUtils() = default;
            static std::vector<std::string> split(const std::string &in, const std::string &delim)
            {
                std::regex reg(delim);
                std::vector<std::string> res = {
                    std::sregex_token_iterator(in.begin(), in.end(), reg, -1), std::sregex_token_iterator()
                };
                return res;
            };
        };

        class TouchEventInfo {
        public:
            struct EventData {
                int actionType;
                int xPosi;
                int yPosi;
                int x2Posi;
                int y2Posi;
                time_t interval;
            };

            static void WriteEventData(std::ofstream &outFile, const EventData &data)
            {
                outFile << g_operationType[data.actionType] << ',';
                outFile << data.xPosi << ',';
                outFile << data.yPosi << ',';
                outFile << data.x2Posi << ',';
                outFile << data.y2Posi << ',';
                outFile << ((data.interval + g_timeindex - 1) / g_timeindex) << ',';
                outFile << g_length << ',';
                outFile << g_velocity << std::endl;
            }

            static void ReadEventLine(std::ifstream &inFile)
            {
                char buffer[50];
                while (!inFile.eof()) {
                    inFile >> buffer;
                    std::string delim = ",";
                    auto caseInfo = TestUtils::split(buffer, delim);
                    string type = caseInfo[Type];
                    int xPosi = std::stoi(caseInfo[XPosi]);
                    int yPosi = std::stoi(caseInfo[YPosi]);
                    int x2Posi = std::stoi(caseInfo[X2Posi]);
                    int y2Posi = std::stoi(caseInfo[Y2Posi]);
                    int interval = std::stoi(caseInfo[Interval]);
                    int length = std::stoi(caseInfo[Length]);
                    int velocity = std::stoi(caseInfo[Velocity]);
                    if (inFile.fail()) {
                        break;
                    } else {
                        std::cout << type << ";"
                                << xPosi << ";"
                                << yPosi << ";"
                                << x2Posi << ";"
                                << y2Posi << ";"
                                << interval << ";"
                                << length << ";"
                                << velocity << ";" << std::endl;
                    }
                    usleep(interval * g_timeindex);
                }
            }
        };

        bool InitReportFolder()
        {
            if (opendir(g_defaultDir.c_str()) == nullptr) {
                int ret = mkdir(g_defaultDir.c_str(), S_IROTH | S_IRWXU | S_IRWXG);
                if (ret != 0) {
                    std::cerr << "failed to create dir: " << g_defaultDir << std::endl;
                    return false;
                }
            }
            return true;
        }

        bool InitEventRecordFile(std::ofstream &outFile)
        {
            if (!InitReportFolder()) {
                return false;
            }
            std::string filePath = g_defaultDir + "/" + "record.csv";
            outFile.open(filePath, std::ios_base::out | std::ios_base::trunc);
            if (!outFile) {
                std::cerr << "Failed to create csv file at:" << filePath << std::endl;
                return false;
            }
            std::cout << "The result will be written in csv file at location: " << filePath << std::endl;
            return true;
        }
    } // namespace

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
                PrintToConsole("Dump layout failed: "+error.message_);
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
        constexpr uint32_t UID2ACCOUNT_DIVISOR = 200000;
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
                procAccount = to_string(stoi(token.c_str()) / UID2ACCOUNT_DIVISOR);
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

    class Timer {
    public:
        Timer(): expired(true), tryToExpire(false)
        {}
        Timer(const Timer& timer)
        {
            expired = timer.expired.load();
            tryToExpire = timer.tryToExpire.load();
        }
        Timer& operator = (const Timer& timer);
        ~Timer()
        {
            Stop();
        }
        void Start(int interval, std::function<void()> task)
        {
            if (expired == false) {
                return;
            }
            expired = false;
            std::thread([this, interval, task]() {
                while (!tryToExpire) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                    task();
                }

                {
                    std::lock_guard<std::mutex> locker(index);
                    expired = true;
                    expiredCond.notify_one();
                }
            }).detach();
        }
        void Stop()
        {
            if (expired) {
                return;
            }

            if (tryToExpire) {
                return;
            }

            tryToExpire = true; // change this bool value to make timer while loop stop
            {
                std::unique_lock<std::mutex> locker(index);
                expiredCond.wait(locker, [this] {return expired == true; });

                // reset the timer
                if (expired == true) {
                    tryToExpire = false;
                }
            }
        }

    private:
        std::atomic<bool> expired; // timer stopped status
        std::atomic<bool> tryToExpire; // timer is in stop process
        std::mutex index;
        std::condition_variable expiredCond;
    };

    class InputEventCallback : public MMI::IInputEventConsumer {
    public:
        void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override
        {
            std::cout << "keyCode" << keyEvent->GetKeyCode() << std::endl;
        }
        int GetDistance(int i, int j) const
        {
            int distance = pow((g_eventsvector[i].GetDisplayX() - g_eventsvector[j].GetDisplayX()), TWO)            \
                + pow((g_eventsvector[i].GetDisplayY() - g_eventsvector[j].GetDisplayY()), TWO);
            return distance;
        }
        double GetSpeed(int i, int j, bool isClick, int clickEventCount) const
        {
            double speed = 0;
            if (isClick) {
                speed = GetDistance(i,j)/pow((g_mmitimesvector[i+clickEventCount]-g_mmitimesvector.back()), TWO);
            } else {
                speed = GetDistance(i,j)/pow((g_mmitimesvector[i]-g_mmitimesvector[j]), TWO);
            }
            return speed;
        }
        void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override
        {
            MMI::PointerEvent::PointerItem item;
            bool result = pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item);
            g_touchtime = GetMillisTime();
            TouchEventInfo::EventData data {};
            if (g_timesvector.size() > 1) {
                data.interval = g_timesvector.back() - g_timesvector[g_timesvector.size() - INDEX_TWO];
            } else {
                data.interval = g_timeindex;
            }
            if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_DOWN) {
                g_timesvector.push_back(GetMillisTime());
            }
            if (!result) {
                std::cout << "GetPointerItem Fail" << std::endl;
            }
            g_eventsvector.push_back(item);
            g_mmitimesvector.push_back(g_touchtime);
            if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_UP)  {
                int indexTime = GetMillisTime();
                int eventCount = g_eventsvector.size();
                int actionInterval = 300;
                int pressDuration = 600;
                int pressTime = indexTime - g_timesvector.back();
                int distance = GetDistance(0, eventCount-INDEX_ONE);
                double speed = GetSpeed(0, eventCount-INDEX_ONE, g_isClick, g_clickEventCount);
                if (eventCount > TWO && (distance > g_maxdistance)) {
                    double threshold = 0.6;
                    if (eventCount>g_dragMonitor && GetDistance(0, g_dragMonitor)<g_maxdistance) { 
                        g_touchop = DRAG_; 
                    } else if (speed < threshold) {
                        g_touchop = SWIPE_; 
                    } else {
                        g_touchop = FLING_; 
                    }
                }else {
                    if (data.interval > actionInterval && pressTime < pressDuration) {
                        g_touchop = CLICK_;
                    } else if (data.interval < actionInterval && pressTime < pressDuration) {
                        g_touchop = DOUBLE_CLICK_;
                    } else if (data.interval > actionInterval && pressTime > pressDuration) {
                        g_touchop = LONG_CLICK_;
                    }
                }
                if (g_touchop == CLICK_) {
                    g_isClick = true;
                    g_clickEventCount = g_mmitimesvector.size();
                } else {
                    g_isClick = false;
                    g_clickEventCount = 0;
                    g_mmitimesvector.clear();
                }
                MMI::PointerEvent::PointerItem up_event = g_eventsvector.back();
                MMI::PointerEvent::PointerItem down_event = g_eventsvector.front();
                data.actionType = g_touchop;
                data.xPosi = down_event.GetDisplayX();
                data.yPosi = down_event.GetDisplayY();
                data.x2Posi = up_event.GetDisplayX();
                data.y2Posi = up_event.GetDisplayY();
                TouchEventInfo::WriteEventData(g_outfile, data);
                std::cout << " PointerEvent:" << g_operationType[data.actionType]
                            << " xPosi:" << data.xPosi
                            << " yPosi:" << data.yPosi
                            << " x2Posi:" << data.x2Posi
                            << " y2Posi:" << data.y2Posi
                            << " interval:" << ((data.interval + g_timeindex - 1) / g_timeindex) << std::endl;
                g_eventsvector.clear();
            }
        }
        void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override {}
        static std::shared_ptr<InputEventCallback> GetPtr();
    };

    std::shared_ptr<InputEventCallback> InputEventCallback::GetPtr()
    {
        return std::make_shared<InputEventCallback>();
    }

    static void TimerFunc()
    {
        int t = GetMillisTime();
        int diff = t - g_touchtime;
        if (diff >= g_timeinterval) {
            cout << "No operation detected for 5 seconds, press ctrl + c to save this file?" << endl;
        }
    }

    static int32_t UiRecord(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest uiRecord <read|record>";
        if (!(argc == INDEX_THREE || argc == INDEX_FOUR )) {
            PrintToConsole(usage);
            return EXIT_FAILURE;
        }
        std::string opt = argv[2];
        if (opt == "record") {
            Timer timer;
            timer.Start(g_timeinterval, TimerFunc);
            if (!InitEventRecordFile(g_outfile)) {
                return OHOS::ERR_INVALID_VALUE;
            }
            if (argc == INDEX_FOUR) {
                g_dragMonitor = atoi(argv[INDEX_THREE]);
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
            std::ifstream inFile(g_defaultDir + "/" + "record.csv");
            TouchEventInfo::ReadEventLine(inFile);
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
