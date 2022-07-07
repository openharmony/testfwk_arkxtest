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
#include <typeinfo>
#include <cstring>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>
#include <condition_variable>
#include "ipc_transactors_impl.h"
#include "system_ui_controller.h"
#include "input_manager.h"
#include "i_input_event_consumer.h"
#include "pointer_event.h"
#include "ui_driver.h"

using namespace std;
using namespace std::chrono;

namespace OHOS::uitest {
    const std::string HELP_MSG =
    "   help,          help messages\n"
    "   screenCap,                  \n"
    "   dumpLayout,                 \n"
    "   uiRecord -record,     record\n"
    "   uiRecord -read,     readfile\n";
    int g_touchTime;
    int g_newTime;
    int g_indexTime;
    int g_pressTime;
    int g_thousandMilliseconds = 1000;
    int g_timeInterval = 5000;
    int g_actionInterval = 300;
    int g_pressDuration = 600;
    int g_maxDistance = 16;

    std::ofstream outFile;
    std::string g_operationType[5] = {"click", "longClick", "doubleClick", "swipe", "drag"};

    enum Number {
        ZERO,
        ONE,
        TWO,
        THREE,
        FOUR,
        FIVE
    };

    TouchOp touchop = CLICK;

    vector<MMI::PointerEvent::PointerItem> g_eventsVector;

    vector<int> g_timesVector;

    vector<int> g_mmiTimesVector;

    namespace {
        std::string operationType_[5] = {"click", "longClick", "doubleClick", "swipe", "drag"};
        std::string DEFAULT_DIR  = "/data/local/tmp/layout";
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

        class touchEventInfo {
        public:
            struct eventData {
                int actionType;
                int xPosi;
                int yPosi;
                int x2Posi;
                int y2Posi;
                time_t interval;
            };

            static void WriteEventData(std::ofstream &outFile_, const eventData &data)
            {
                outFile_ << operationType_[data.actionType] << ',';
                outFile_ << data.xPosi << ',';
                outFile_ << data.yPosi << ',';
                outFile_ << data.x2Posi << ',';
                outFile_ << data.y2Posi << ',';
                outFile_ << ((data.interval + g_thousandMilliseconds - 1) / g_thousandMilliseconds) << std::endl;
            }

            static void ReadEventLine(std::ifstream &inFile)
            {
                char buffer[50];
                string Type;
                int xPosi = -1;
                int yPosi = -1;
                int x2Posi = -1;
                int y2Posi = -1;
                int interval = -1;
                while (!inFile.eof()) {
                    inFile >> buffer;
                    std::string delim = ",";
                    auto caseInfo = TestUtils::split(buffer, delim);
                    Type = caseInfo[ZERO];
                    xPosi = std::stoi(caseInfo[ONE]);
                    yPosi = std::stoi(caseInfo[TWO]);
                    x2Posi = std::stoi(caseInfo[THREE]);
                    y2Posi = std::stoi(caseInfo[FOUR]);
                    interval = std::stoi(caseInfo[FIVE]);
                    if (inFile.fail()) {
                        break;
                    } else {
                        std::cout << Type << ";"
                                << xPosi << ";"
                                << yPosi << ";"
                                << x2Posi << ";"
                                << y2Posi << ";"
                                << interval << std::endl;
                    }
                    usleep(interval * g_thousandMilliseconds);
                }
            }
        };

        bool InitReportFolder()
        {
            DIR *rootDir = nullptr;
            if ((rootDir = opendir(DEFAULT_DIR.c_str())) == nullptr) {
                int ret = mkdir(DEFAULT_DIR.c_str(), S_IROTH | S_IRWXU | S_IRWXG);
                if (ret != 0) {
                    std::cerr << "failed to create dir: " << DEFAULT_DIR << std::endl;
                    return false;
                }
            }
            return true;
        }

        bool InitEventRecordFile(std::ofstream &outFile_)
        {
            if (!InitReportFolder()) {
                return false;
            }
            std::string filePath = DEFAULT_DIR + "/" + "record.csv";
            outFile_.open(filePath, std::ios_base::out | std::ios_base::trunc);
            if (!outFile_) {
                std::cerr << "Failed to create csv file at:" << filePath << std::endl;
                return false;
            }
            std::cout << "The result will be written in csv file at location: " << filePath << std::endl;
            return true;
        }
    } // namespace

    struct option g_longOptions[] = {
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
        while ((opt = getopt_long(argc, argv, optstring.data(), g_longOptions, nullptr)) != -1) {
            switch (opt) {
                case '?':
                    PrintToConsole(usage);
                    return EXIT_FAILURE;
                    break;
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
            if (error.code_ !=NO_ERROR) {
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
        Timer(): _expired(true), _try_to_expire(false)
        {}

        Timer(const Timer& timer)
        {
            _expired = timer._expired.load();
            _try_to_expire = timer._try_to_expire.load();
        }

        Timer& operator = (const Timer& timer);
        ~Timer()
        {
            stop();
        }

        void start(int interval, std::function<void()> task)
        {
            if (_expired == false) {
                return;
            }
            _expired = false;
            std::thread([this, interval, task]() {
                while (!_try_to_expire) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                    task();
                }

                {
                    std::lock_guard<std::mutex> locker(_mutex);
                    _expired = true;
                    _expired_cond.notify_one();
                }
            }).detach();
        }

        void stop()
        {
            if (_expired) {
                return;
            }

            if (_try_to_expire) {
                return;
            }

            _try_to_expire = true; // change this bool value to make timer while loop stop
            {
                std::unique_lock<std::mutex> locker(_mutex);
                _expired_cond.wait(locker, [this] {return _expired == true; });

                // reset the timer
                if (_expired == true) {
                        _try_to_expire = false;
                }
            }
        }

    private:
        std::atomic<bool> _expired; // timer stopped status
        std::atomic<bool> _try_to_expire; // timer is in stop process
        std::mutex _mutex;
        std::condition_variable _expired_cond;
    };

    class InputEventCallback : public MMI::IInputEventConsumer {
    public:
        virtual void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override
        {
            std::cout << "keyCode" << keyEvent->GetKeyCode() << std::endl;
        }
        virtual void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override
        {
            MMI::PointerEvent::PointerItem item;
            bool result = pointerEvent->GetPointerItem(pointerEvent->GetPointerId(), item);
            g_touchTime = GetMillisTime();
            touchEventInfo::eventData data {};
            if (g_timesVector.size() > 1) {
                    int alpha = g_timesVector.size();
                    data.interval = g_timesVector.back()-g_timesVector[alpha - TWO];
            } else {
                    data.interval = g_thousandMilliseconds;
            }
            if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_DOWN) {
                    g_newTime = GetMillisTime();
                    g_timesVector.push_back(g_newTime);
            }
            if (!result) {
                std::cout << "GetPointerItem Fail" << std::endl;
            }
            if (pointerEvent->GetPointerAction() != MMI::PointerEvent::POINTER_ACTION_UP) {
                g_eventsVector.push_back(item);
                if (pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_DOWN || pointerEvent->GetPointerAction() == MMI::PointerEvent::POINTER_ACTION_MOVE) {
                    g_mmiTimesVector.push_back(g_touchTime);
                }
            } else {
                g_indexTime = GetMillisTime();
                g_pressTime = g_indexTime - g_newTime;
                if (g_eventsVector.size() > 1 && ((item.GetGlobalX() - g_eventsVector[0].GetGlobalX()) \
                    * (item.GetGlobalX() - g_eventsVector[0].GetGlobalX()) +                           \
                    (item.GetGlobalY()-g_eventsVector[0].GetGlobalY())*(item.GetGlobalY() -            \
                    g_eventsVector[0].GetGlobalY())>g_maxDistance)) {
                    if (g_mmiTimesVector[1] - g_mmiTimesVector[0] > g_actionInterval) {
                        touchop = DRAG;
                    } else {
                        touchop = SWIPE;
                    }
                    g_mmiTimesVector.clear();
                } else {
                    if (data.interval > g_actionInterval && g_pressTime < g_pressDuration) {
                            touchop = CLICK;
                        } else if (data.interval < g_actionInterval && g_pressTime < g_pressDuration) {
                            touchop = DOUBLE_CLICK_P;
                        } else if (data.interval > g_actionInterval && g_pressTime > g_pressDuration) {
                            touchop = LONG_CLICK;
                        }
                }
                MMI::PointerEvent::PointerItem up_event = g_eventsVector.back();
                MMI::PointerEvent::PointerItem down_event = g_eventsVector.front();
                data.actionType = touchop;
                data.xPosi = down_event.GetGlobalX();
                data.yPosi = down_event.GetGlobalY();
                data.x2Posi = up_event.GetGlobalX();
                data.y2Posi = up_event.GetGlobalY();
                touchEventInfo::WriteEventData(outFile, data);
                std::cout << " PointerEvent:" << g_operationType[data.actionType]
                            << " xPosi:" << data.xPosi
                            << " yPosi:" << data.yPosi
                            << " x2Posi:" << data.x2Posi
                            << " y2Posi:" << data.y2Posi
                            << " interval:" << ((data.interval + g_thousandMilliseconds - 1) / g_thousandMilliseconds) << std::endl;

                g_eventsVector.clear();
            }
        }
        virtual void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override {}
        static std::shared_ptr<InputEventCallback> GetPtr();
    };

    std::shared_ptr<InputEventCallback> InputEventCallback::GetPtr()
    {
        return std::make_shared<InputEventCallback>();
    }

    static void timerFunc()
    {
        int t = GetMillisTime();
        int diff = t - g_touchTime;
        if (diff >= g_timeInterval) {
            cout<<"No operation detected for 5 seconds, press ctrl + c to save this file?"<<endl;
        }
    }

    static int32_t UiRecord(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest uiRecord <read|record>";
        if (argc != THREE) {
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
        std::string opt = argv[2];
        if (opt == "record") {
            Timer timer;
            timer.start(g_timeInterval, timerFunc);
            if (!InitEventRecordFile(outFile)) {
            return OHOS::ERR_INVALID_VALUE;
            }
            auto callBackPtr = InputEventCallback::GetPtr();
            if (callBackPtr == nullptr) {
                std::cout << "nullptr" << std::endl;
            }
            int32_t id1 = MMI::InputManager::GetInstance()->AddMonitor(callBackPtr);
            if (id1 == -1) {
                std::cout << "Startup Failed!" << std::endl;
            }
            std::cout << "Started Recording Successfully..." << std::endl;
            int flag = getc(stdin);
            std::cout << flag << std::endl;
            return OHOS::ERR_OK;
        } else if (opt == "read") {
            std::ifstream inFile(DEFAULT_DIR + "/" + "record.csv");
            touchEventInfo::ReadEventLine(inFile);
            return OHOS::ERR_OK;
        } else {
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
    }

    static int32_t Help(int32_t argc, char *argv[])
    {
        return EXIT_SUCCESS;
    }

    extern "C" int32_t main(int32_t argc, char *argv[])
    {
        static constexpr string_view usage = "USAGE: uitest <help|screenCap|dumpLayout|uiRecord>";
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
        } else if (command == "uiRecord") {
            exit(UiRecord(argc, argv));
        } else if (command == "help") {
            PrintToConsole(HELP_MSG);
            exit(Help(argc, argv));
        } else {
            PrintToConsole("Illegal argument: " + command);
            PrintToConsole(usage);
            exit(EXIT_FAILURE);
        }
    }
}
