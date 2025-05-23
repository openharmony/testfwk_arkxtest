/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ui_input.h"

namespace OHOS::uitest {
    using namespace std;
    std::string opt;
    ApiCallErr exception_ = ApiCallErr(NO_ERROR);

    void CheckSwipeVelocityPps(UiOpArgs& args)
    {
        if (args.swipeVelocityPps_ < args.minSwipeVelocityPps_ || args.swipeVelocityPps_ > args.maxSwipeVelocityPps_) {
            std::cout << "The swipe velocity out of range, the default value will be used. \n" << std::endl;
            args.swipeVelocityPps_ = args.defaultSwipeVelocityPps_;
        }
    }
    bool CheckParams(int32_t argc, size_t want)
    {
        if ((size_t)argc < want) {
            std::cout << "Missing parameter. \n" << std::endl;
            PrintInputMessage();
            return false;
        }
        return true;
    }
    void PrintInputMessage()
    {
        const std::string usage =
        "USAGE : \n"
        "help                                                                                  print uiInput usage\n"
        "dircFling [velocity stepLength]                     direction ranges from 0,1,2,3 (left, right, up, down)\n"
        "click/doubleClick/longClick <x> <y>                                       click on the target coordinates\n"
        "swipe/drag <from_x> <from_y> <to_x> <to_y> [velocity]      velocity ranges from 200 to 40000, default 600\n"
        "fling <from_x> <from_y> <to_x> <to_y> [velocity]           velocity ranges from 200 to 40000, default 600\n"
        "keyEvent <keyID/Back/Home/Power>                                                          inject keyEvent\n"
        "keyEvent <keyID_0> <keyID_1> [keyID_2]                                           keyID_2 default to None \n"
        "inputText <x> <y> <text>                                         inputText at the target coordinate point\n"
        "text <text>                                           input text at the location where is already focused\n";
        std::cout << usage << std::endl;
    }
    bool ParameterRedundancy()
    {
        std::cout << " Too many parameters. \n" << std::endl;
        PrintInputMessage();
        return EXIT_FAILURE;
    }
    static bool CreateFlingPoint(Point &to, Point &from, Point screenSize, Direction direction)
    {
        to = Point(screenSize.px_ / INDEX_TWO, screenSize.py_ / INDEX_TWO);
        switch (direction) {
            case TO_LEFT:
                from.px_ = to.px_ - screenSize.px_ / INDEX_FOUR;
                from.py_ = to.py_;
                return true;
            case TO_RIGHT:
                from.px_ = to.px_ + screenSize.px_ / INDEX_FOUR;
                from.py_ = to.py_;
                return true;
            case TO_UP:
                from.px_ = to.px_;
                from.py_ = to.py_ - screenSize.py_ / INDEX_FOUR;
                return true;
            case TO_DOWN:
                from.px_ = to.px_;
                from.py_ = to.py_ + screenSize.py_ / INDEX_FOUR;
                return true;
            default:
                std::cout << "Illegal argument: " + std::to_string(direction) << std::endl;
                PrintInputMessage();
                return false;
        }
    }
    bool GetPoints(Point &to, Point &from, int32_t argc, char *argv[])
    {
        auto from_x = atoi(argv[THREE]);
        auto from_y = atoi(argv[FOUR]);
        auto to_x = atoi(argv[FIVE]);
        auto to_y = atoi(argv[SIX]);
        if (from_x <= 0 || from_y <= 0 || to_x <= 0 || to_y <= 0) {
            std::cout << "Please confirm that the coordinate values are correct. \n" << std::endl;
            PrintInputMessage();
            return false;
        }
        from = Point(from_x, from_y);
        to = Point(to_x, to_y);
        return true;
    }
    bool GetPoint(Point &point, int32_t argc, char *argv[])
    {
        auto from_x = atoi(argv[THREE]);
        auto from_y = atoi(argv[FOUR]);
        if (from_x <= 0 || from_y <= 0) {
            std::cout << "Please confirm that the coordinate values are correct. \n" << std::endl;
            PrintInputMessage();
            return false;
        }
        point = Point(from_x, from_y);
        return true;
    }
    bool CheckStepLength(UiOpArgs &uiOpArgs, Point to, Point from, uint32_t stepLength)
    {
        const int32_t distanceX = to.px_ - from.px_;
        const int32_t distanceY = to.py_ - from.py_;
        const uint32_t distance = sqrt(distanceX * distanceX + distanceY * distanceY);
        if (stepLength <= 0 || stepLength > distance) {
            std::cout << "The stepLen is out of range" << std::endl;
            return EXIT_FAILURE;
        } else {
            uiOpArgs.swipeStepsCounts_ = distance / stepLength;
            return true;
        }
    }
    int32_t FlingActionInput(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        TouchOp op = TouchOp::SWIPE;
        Point screenSize;
        Direction direction;
        Point from;
        Point to;
        if (opt == "dircFling" && CheckParams(argc, INDEX_FOUR)) {
            direction = (Direction)atoi(argv[THREE]);
            screenSize = driver.GetDisplaySize(exception_);
            if (!CreateFlingPoint(to, from, screenSize, direction)) {
                return EXIT_FAILURE;
            }
            if ((size_t)argc >= INDEX_FIVE) {
                uiOpArgs.swipeVelocityPps_ = atoi(argv[FOUR]);
            }
            if ((size_t)argc == INDEX_SIX) {
                uiOpArgs.swipeStepsCounts_ = atoi(argv[FIVE]);
            } else if ((size_t)argc > INDEX_SIX) {
                return ParameterRedundancy();
            }
        } else if (opt == "fling" && CheckParams(argc, INDEX_SEVEN)) {
            if (!GetPoints(to, from, argc, argv)) {
                return EXIT_FAILURE;
            }
            if ((size_t)argc == INDEX_EIGHT) {
                uiOpArgs.swipeVelocityPps_ = (uint32_t)atoi(argv[SEVEN]);
            } else if ((size_t)argc == INDEX_NINE) {
                auto stepLength = (uint32_t)atoi(argv[EIGHT]);
                if (!CheckStepLength(uiOpArgs, to, from, stepLength)) {
                    return EXIT_FAILURE;
                }
            } else if ((size_t)argc > INDEX_NINE) {
                return ParameterRedundancy();
            }
        } else {
            return EXIT_FAILURE;
        }
        CheckSwipeVelocityPps(uiOpArgs);
        auto touch = GenericSwipe(op, from, to);
        driver.PerformTouch(touch, uiOpArgs, exception_);
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t SwipeActionInput(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        opt = argv[TWO];
        TouchOp op = TouchOp::SWIPE;
        if (opt == "drag") {
            op = TouchOp::DRAG;
        }
        Point from;
        Point to;
        if (CheckParams(argc, INDEX_SEVEN)) {
            if (!GetPoints(to, from, argc, argv)) {
                return EXIT_FAILURE;
            }
            if ((size_t)argc == INDEX_EIGHT) {
                uiOpArgs.swipeVelocityPps_ = (uint32_t)atoi(argv[SEVEN]);
            } else if ((size_t)argc > INDEX_EIGHT) {
                return ParameterRedundancy();
            }
        } else {
            return EXIT_FAILURE;
        }
        CheckSwipeVelocityPps(uiOpArgs);
        auto touch = GenericSwipe(op, from, to);
        driver.PerformTouch(touch, uiOpArgs, exception_);
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t KeyEventActionInput(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        std::string key = argv[THREE];
        if (key == "Home" && (size_t)argc == INDEX_FOUR) {
            driver.TriggerKey(Home(), uiOpArgs, exception_);
        } else if (key == "Back" && (size_t)argc == INDEX_FOUR) {
            driver.TriggerKey(Back(), uiOpArgs, exception_);
        } else if (key == "Power" && (size_t)argc == INDEX_FOUR) {
            driver.TriggerKey(Power(), uiOpArgs, exception_);
        } else if (atoi(argv[THREE]) != 0) {
            int32_t codeZero_ = atoi(argv[THREE]);
            int32_t codeOne_;
            int32_t codeTwo_;
            if ((size_t)argc == INDEX_FOUR) {
                auto keyAction_ = AnonymousSingleKey(codeZero_);
                driver.TriggerKey(keyAction_, uiOpArgs, exception_);
            } else if ((size_t)argc == INDEX_FIVE || (size_t)argc == INDEX_SIX) {
                codeOne_ = atoi(argv[FOUR]);
                if ((size_t)argc == INDEX_SIX) {
                    codeTwo_ = atoi(argv[FIVE]);
                } else {
                    codeTwo_ = KEYCODE_NONE;
                }
                auto keyAction_ = CombinedKeys(codeZero_, codeOne_, codeTwo_);
                driver.TriggerKey(keyAction_, uiOpArgs, exception_);
            } else {
                return ParameterRedundancy();
            }
        } else {
            std::cout << "Invalid parameters. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t TextActionInput(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        if ((size_t)argc != INDEX_FOUR) {
            std::cout << "The number of parameters is incorrect. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
        auto text = argv[THREE];
        driver.InputText(text, exception_, uiOpArgs);
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t TextActionInputWithPoint(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        if ((size_t)argc != INDEX_SIX) {
            std::cout << "The number of parameters is incorrect. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
        Point point;
        if (!GetPoint(point, argc, argv)) {
            return EXIT_FAILURE;
        }
        auto text = argv[FIVE];
        auto touch = GenericClick(TouchOp::CLICK, point);
        driver.PerformTouch(touch, uiOpArgs, exception_);
        static constexpr uint32_t focusTimeMs = 500;
        driver.DelayMs(focusTimeMs);
        driver.InputText(text, exception_, uiOpArgs);
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t ClickActionInput(int32_t argc, char *argv[], UiDriver &driver, UiOpArgs uiOpArgs)
    {
        if ((size_t)argc != INDEX_FIVE) {
            std::cout << "The number of parameters is incorrect. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
        TouchOp op = TouchOp::CLICK;
        if (opt == "doubleClick") {
            op = TouchOp::DOUBLE_CLICK_P;
        } else if (opt == "longClick") {
            op = TouchOp::LONG_CLICK;
        }
        Point point;
        if (!GetPoint(point, argc, argv)) {
            return EXIT_FAILURE;
        }
        auto touch = GenericClick(op, point);
        driver.PerformTouch(touch, uiOpArgs, exception_);
        std::cout << exception_.message_ << std::endl;
        return EXIT_SUCCESS;
    }
    int32_t UiActionInput(int32_t argc, char *argv[])
    {
        // 1 uitest 2 uiInput 3 TouchOp 4 5 from_point 6 7 to_point 8 velocity
        auto driver = UiDriver();
        UiOpArgs uiOpArgs;
        opt = argv[TWO];
        if (!CheckParams(argc, INDEX_FOUR)) {
            return EXIT_FAILURE;
        }
        if (opt == "keyEvent") {
            return KeyEventActionInput(argc, argv, driver, uiOpArgs);
        } else if (opt == "fling" || opt == "dircFling") {
            return FlingActionInput(argc, argv, driver, uiOpArgs);
        } else if (opt == "swipe" || opt == "drag") {
            return SwipeActionInput(argc, argv, driver, uiOpArgs);
        } else if (opt == "click" || opt == "longClick" || opt == "doubleClick") {
            return ClickActionInput(argc, argv, driver, uiOpArgs);
        } else if (opt == "inputText") {
            return TextActionInputWithPoint(argc, argv, driver, uiOpArgs);
        } else if (opt == "text") {
            return TextActionInput(argc, argv, driver, uiOpArgs);
        } else {
            std::cout << "Invalid parameters. \n" << std::endl;
            PrintInputMessage();
            return EXIT_FAILURE;
        }
    }
} // namespace OHOS::uitest
