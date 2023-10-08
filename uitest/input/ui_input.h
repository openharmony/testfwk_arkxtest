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

#ifndef UI_INPUT_H
#define UI_INPUT_H
#include <regex>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <typeinfo>
#include <vector>
#include <condition_variable>
#include "ui_driver.h"
#include "ui_action.h"
#include "ui_model.h"
#include "frontend_api_handler.h"
#include "common_utilities_hpp.h"
#include <stdio.h>
#include <stdlib.h>

namespace OHOS::uitest {
    int32_t UiActionInput(int32_t argc, char *argv[]);
    void PrintInputMessage();
} // namespace OHOS::uitest
#endif // UI_INPUT_H