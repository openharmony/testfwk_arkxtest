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

#ifndef TEST_SERVER_MOCK_PERMISSION_H
#define TEST_SERVER_MOCK_PERMISSION_H

#include <algorithm>
#include <cstdint>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "access_token.h"
#include "access_token_error.h"
#include "accesstoken_kit.h"
#include "atm_tools_param_info.h"
#include "perm_setproc.h"
#include "token_setproc.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
inline constexpr uid_t ACCESS_TOKEN_UID = 3020;
inline const std::string MOCK_HAP_BUNDLE_NAME = "com.ohos.permissionmanager";
inline const std::string MOCK_NATIVE_PROCESS_NAME = "testserver";
inline constexpr uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);

inline uid_t MockAccessTokenUid()
{
    uid_t originalEuid = getuid();
    if (getuid() != ACCESS_TOKEN_UID) {
        (void)setuid(ACCESS_TOKEN_UID);
    }
    return originalEuid;
}

inline void RestoreUid(uid_t originalEuid)
{
    if (originalEuid != ACCESS_TOKEN_UID) {
        (void)setuid(originalEuid);
    }
}

class MockToken final {
public:
    explicit MockToken(const std::vector<std::string>& permissionList, bool isHap = true, bool isSystem = false)
    {
        errMsg_.clear();
        selfToken_ = GetSelfTokenID();
        tokenId_ = GetTokenByProcessName(isHap ? MOCK_HAP_BUNDLE_NAME : MOCK_NATIVE_PROCESS_NAME);
        if ((tokenId_ == INVALID_TOKENID) && (selfToken_ != 0)) {
            tokenId_ = static_cast<AccessTokenID>(selfToken_);
        }
        AccessTokenIDEx tokenIdEx = {0};
        tokenIdEx.tokenIDEx = static_cast<uint64_t>(tokenId_);
        uint64_t fullTokenId = tokenIdEx.tokenIDEx;
        if (isHap && isSystem) {
            fullTokenId |= SYSTEM_APP_MASK;
        } else if (isHap) {
            fullTokenId &= ~SYSTEM_APP_MASK;
        }
        (void)SetSelfTokenID(fullTokenId);
        MockPermissions(permissionList);
    }

    ~MockToken()
    {
        uid_t originalEuid = MockAccessTokenUid();
        for (size_t i = 0; i < oriPermissionList_.size(); ++i) {
            uint32_t opCode = 0;
            if (!AccessTokenKit::TransferPermissionToOpcode(oriPermissionList_[i], opCode)) {
                continue;
            }
            (void)SetPermissionToKernel(tokenId_, static_cast<int32_t>(opCode), oriStatusList_[i]);
        }
        RestoreUid(originalEuid);
        (void)SetSelfTokenID(selfToken_);
    }

    MockToken(const MockToken&) = delete;
    MockToken& operator=(const MockToken&) = delete;

    AccessTokenID GetTokenId() const
    {
        return tokenId_;
    }

    std::string GetMockErrorMsg() const
    {
        return errMsg_;
    }

    AccessTokenID TransfterStrToAccesstokenID(const std::string& numStr)
    {
        size_t index = 0;
        while (index < numStr.length()) {
            if (std::isdigit(static_cast<unsigned char>(numStr[index])) == 0) {
                break;
            }
            ++index;
        }
        if ((index != numStr.length()) || numStr.empty()) {
            return INVALID_TOKENID;
        }

        return static_cast<AccessTokenID>(std::stoul(numStr));
    }

    AccessTokenID GetTokenByProcessName(const std::string& processName)
    {
        std::string dumpInfo;
        AtmToolsParamInfo info;
        AccessTokenKit::DumpTokenInfo(info, dumpInfo);
        std::istringstream stream(dumpInfo);
        std::string line;
        while (std::getline(stream, line)) {
            size_t pos = line.find(':');
            if (pos == std::string::npos) {
                continue;
            }
            std::string name = line.substr(pos + 1);
            name.erase(0, name.find_first_not_of(" \t"));
            if (name == processName) {
                return TransfterStrToAccesstokenID(line.substr(0, pos));
            }
        }
        return INVALID_TOKENID;
    }

    void Grant(const std::string& permission)
    {
        errMsg_.clear();
        if (!IsInitializedPermission(permission)) {
            return;
        }
        SetPermission(permission, true);
    }

    void Revoke(const std::string& permission)
    {
        errMsg_.clear();
        if (!IsInitializedPermission(permission)) {
            return;
        }
        SetPermission(permission, false);
    }

private:
    void MockPermissions(const std::vector<std::string>& permissionList)
    {
        errMsg_.clear();
        if (tokenId_ == INVALID_TOKENID) {
            errMsg_ = "token id not exist";
            return;
        }
        const std::string success = "success";
        errMsg_ = success;
        for (size_t i = 0; i < permissionList.size(); ++i) {
            SetPermission(permissionList[i], true);
        }
    }

    bool IsInitializedPermission(const std::string& permission)
    {
        if (std::find(oriPermissionList_.begin(), oriPermissionList_.end(), permission) != oriPermissionList_.end()) {
            return true;
        }
        errMsg_ = "permission is not initialized: " + permission;
        return false;
    }

    void SetPermission(const std::string& permission, bool status)
    {
        uint32_t opCode = 0;
        if (!AccessTokenKit::TransferPermissionToOpcode(permission, opCode)) {
            errMsg_ += "; no perm: " + permission;
            return;
        }
        int32_t code = static_cast<int32_t>(opCode);

        uid_t originalEuid = MockAccessTokenUid();
        if (std::find(oriPermissionList_.begin(), oriPermissionList_.end(), permission) == oriPermissionList_.end()) {
            bool originalStatus = false;
            int32_t ret = GetPermissionFromKernel(tokenId_, code, originalStatus);
            if (ret == RET_SUCCESS) {
                oriPermissionList_.emplace_back(permission);
                oriStatusList_.emplace_back(originalStatus);
            } else {
                errMsg_ += "; get kernel perm failed: " + permission + ", ret=" + std::to_string(ret);
            }
        }

        int32_t ret = SetPermissionToKernel(tokenId_, code, status);
        if (ret != RET_SUCCESS) {
            errMsg_ += "; set kernel perm failed: " + permission + ", ret=" + std::to_string(ret);
        }
        RestoreUid(originalEuid);
    }

    AccessTokenID tokenId_ = INVALID_TOKENID;
    uint64_t selfToken_ = 0;
    std::vector<std::string> oriPermissionList_;
    std::vector<bool> oriStatusList_;
    std::string errMsg_ = "";
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // TEST_SERVER_MOCK_PERMISSION_H
