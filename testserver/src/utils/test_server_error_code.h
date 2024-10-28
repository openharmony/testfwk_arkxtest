/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef TEST_SERVER_ERROR_CODE_H
#define TEST_SERVER_ERROR_CODE_H

namespace OHOS::testserver {
    enum {
        TEST_SERVER_OK = 0,
        TEST_SERVER_GET_INTERFACE_FAILED = -1,
        TEST_SERVER_ADD_DEATH_RECIPIENT_FAILED = 19000001,
        TEST_SERVER_CREATE_PASTE_DATA_FAILED = 19000002,
        TEST_SERVER_SET_PASTE_DATA_FAILED = 19000003,
        TEST_SERVER_PUBLISH_EVENT_FAILED = 19000004,
    };
} //OHOS::testserver

#endif // TEST_SERVER_ERROR_CODE_H