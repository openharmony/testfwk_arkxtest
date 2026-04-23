/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <functional>
#include "common_utils.h"
#include "testhelper_core.h"

namespace OHOS::testhelper {
    // Constants
    constexpr const char* VERSION = "1.0.0";
    constexpr const char* HELP_MSG = R"(
Usage:
  testhelper <command> [options]

Commands:
  get-time           Get current system time
  set-time <time>    Set system time (format: YYYY-MM-DD HH:MM:SS)
  get-timezone       Get current system timezone
  set-timezone <timezone>  Set system timezone (e.g., Asia/Shanghai)
  get-pastedata      Get pasteboard text content
  set-pastedata <text>     Set pasteboard text content
  clear-pastedata    Clear pasteboard content
  hide-keyboard      Hide input method keyboard
  get-fontname <font-path> Get font name from font file (/data/local/* files only)
  install-font <font-path>  Install font from font file (/data/local/* files only)
  uninstall-font <font-name>  Uninstall font by font name
  set-viewmode <dark|light>  Set view mode (dark or light)
  help               Show this help message
  --version          Show version information
)";

    // Argument count validation map: command -> (min args, max args, usage)
    const std::map<std::string, std::tuple<int32_t, int32_t, std::string>> g_argCountMap = {
        {"get-time", std::make_tuple(2, 2, "testhelper get-time")},
        {"set-time", std::make_tuple(3, 4, "testhelper set-time <YYYY-MM-DD HH:MM:SS>")},
        {"get-timezone", std::make_tuple(2, 2, "testhelper get-timezone")},
        {"set-timezone", std::make_tuple(3, 3, "testhelper set-timezone <timezone>")},
        {"get-pastedata", std::make_tuple(2, 2, "testhelper get-pastedata")},
        {"set-pastedata", std::make_tuple(3, 3, "testhelper set-pastedata <text>")},
        {"clear-pastedata", std::make_tuple(2, 2, "testhelper clear-pastedata")},
        {"hide-keyboard", std::make_tuple(2, 2, "testhelper hide-keyboard")},
        {"get-fontname", std::make_tuple(3, 3, "testhelper get-fontname <font-path> (/data/local/* files only)")},
        {"install-font", std::make_tuple(3, 3, "testhelper install-font <font-path> (/data/local/* files only)")},
        {"uninstall-font", std::make_tuple(3, 3, "testhelper uninstall-font <font-name>")},
        {"set-viewmode", std::make_tuple(3, 3, "testhelper set-viewmode <dark|light>")},
        {"--version", std::make_tuple(2, 2, "testhelper --version")},
        {"help", std::make_tuple(2, 2, "testhelper help")},
    };

    // Validate argument count
    void ValidateArgCount(const std::string& command, int32_t argc)
    {
        auto it = g_argCountMap.find(command);
        if (it == g_argCountMap.end()) {
            PrintToConsole("Error: Invalid arguments.");
            _Exit(EXIT_FAILURE);
        }
        auto [minArgs, maxArgs, usage] = it->second;
        if (argc < minArgs || argc > maxArgs) {
            PrintToConsole("Error: Invalid arguments. Usage: " + usage);
            _Exit(EXIT_FAILURE);
        }
    }

    // Command handler type
    using CommandHandler = std::function<int32_t(TestHelperCore&, int32_t, char**)>;

    // Command handler registry
    const std::map<std::string, CommandHandler> g_commandHandlers = {
        {"get-time", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("get-time", argc);
            return core.HandleGetTime();
        }},
        {"set-time", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("set-time", argc);
            std::string timeStr;
            for (int i = TWO; i < argc; i++) {
                if (i > TWO) {
                    timeStr += " ";
                }
                timeStr += argv[i];
            }
            return core.HandleSetTime(timeStr);
        }},
        {"get-timezone", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("get-timezone", argc);
            return core.HandleGetTimezone();
        }},
        {"set-timezone", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("set-timezone", argc);
            std::string timezone(argv[TWO]);
            return core.HandleSetTimezone(timezone);
        }},
        {"get-pastedata", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("get-pastedata", argc);
            return core.HandleGetPasteData();
        }},
        {"set-pastedata", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("set-pastedata", argc);
            std::string text(argv[TWO]);
            return core.HandleSetPasteData(text);
        }},
        {"clear-pastedata", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("clear-pastedata", argc);
            return core.HandleClearPasteData();
        }},
        {"hide-keyboard", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("hide-keyboard", argc);
            return core.HandleHideKeyboard();
        }},
        {"get-fontname", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("get-fontname", argc);
            std::string fontPath(argv[TWO]);
            return core.HandleGetFontname(fontPath);
        }},
        {"install-font", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("install-font", argc);
            std::string fontPath(argv[TWO]);
            return core.HandleInstallFont(fontPath);
        }},
        {"uninstall-font", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("uninstall-font", argc);
            std::string fontName(argv[TWO]);
            return core.HandleUninstallFont(fontName);
        }},
        {"set-viewmode", [](TestHelperCore& core, int32_t argc, char** argv) -> int32_t {
            ValidateArgCount("set-viewmode", argc);
            std::string mode(argv[TWO]);
            return core.HandleSetViewMode(mode);
        }},
    };

    // Main entry point
    extern "C" int main(int32_t argc, char *argv[])
    {
        if (argc < TWO) {
            PrintToConsole("Missing argument");
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_FAILURE);
        }
        std::string command(argv[1]);
        if (command == "--version") {
            ValidateArgCount("--version", argc);
            PrintToConsole(VERSION);
            _Exit(EXIT_SUCCESS);
        } else if (command == "help") {
            ValidateArgCount("help", argc);
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_SUCCESS);
        }
        auto it = g_commandHandlers.find(command);
        if (it != g_commandHandlers.end()) {
            TestHelperCore core;
            _Exit(it->second(core, argc, argv));
        } else {
            PrintToConsole("Illegal argument: " + command);
            PrintToConsole(HELP_MSG);
            _Exit(EXIT_FAILURE);
        }
    }
}
