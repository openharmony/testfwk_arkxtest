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

#include "mock_permission.h"

#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS::testserver {
    void TestServerMockPermission::MockPermission()
    {
        static const char *permissionParams[] = {
            "ohos.permission.DISTRIBUTED_DATASYNC"};
        uint64_t tokenId;
        NativeTokenInfoParams infoInstance = {
            .dcapsNum = 0,
            .permsNum = 1,
            .aclsNum = 0,
            .dcaps = nullptr,
            .perms = permissionParams,
            .acls = nullptr,
            .processName = "distributedsched",
            .aplStr = "system_core",
        };
        tokenId = GetAccessTokenId(&infoInstance);
        SetSelfTokenID(tokenId);
    }

    void TestServerMockPermission::MockProcess(const char *processName)
    {
        static const char *permissionParams[] = {
            "ohos.permission.DISTRIBUTED_DATASYNC"};
        uint64_t tokenId;
        NativeTokenInfoParams infoInstance = {
            .dcapsNum = 0,
            .permsNum = 1,
            .aclsNum = 0,
            .dcaps = nullptr,
            .perms = permissionParams,
            .acls = nullptr,
            .processName = processName,
            .aplStr = "system_core",
        };
        tokenId = GetAccessTokenId(&infoInstance);
        SetSelfTokenID(tokenId);
    }
} // namespace OHOS::testserver