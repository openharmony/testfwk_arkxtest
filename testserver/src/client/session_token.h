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

#ifndef SESSION_TOKEN_H
#define SESSION_TOKEN_H

#include "ipc_object_stub.h"
#include "parcel.h"
#include "message_parcel.h"

namespace OHOS::testserver {
    using namespace OHOS::HiviewDFX;
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL_SESSION_TOKEN = { LOG_CORE, 0xD003110, "SessionToken"};

    class SessionToken : public Parcelable {
    public:
        SessionToken(sptr<IPCObjectStub> ipcObjectStub = new(std::nothrow) IPCObjectStub(),
                                                                    sptr<IRemoteObject> iRemoteObject = nullptr)
        {
            HiLog::Info(LABEL_SESSION_TOKEN, "%{public}s called. ", __func__);
            ipcObjectStub_ = ipcObjectStub;
            iRemoteObject_ = iRemoteObject;
        }

        ~SessionToken()
        {
            HiLog::Info(LABEL_SESSION_TOKEN, "%{public}s called. ", __func__);
        }

        bool Marshalling(Parcel &out) const override
        {
            HiLog::Info(LABEL_SESSION_TOKEN, "%{public}s called. ", __func__);
            static_cast<MessageParcel *>(&out)->WriteRemoteObject(ipcObjectStub_);
            return true;
        }

        static SessionToken *Unmarshalling(Parcel &in)
        {
            HiLog::Info(LABEL_SESSION_TOKEN, "%{public}s called. ", __func__);
            sptr<IRemoteObject> iRemoteObject = static_cast<MessageParcel *>(&in)->ReadRemoteObject();
            SessionToken *sessionToken = new SessionToken(nullptr, iRemoteObject);
            return sessionToken;
        }

        bool AddDeathRecipient(const sptr<IRemoteObject::DeathRecipient> &recipient) const
        {
            HiLog::Info(LABEL_SESSION_TOKEN, "%{public}s called. ", __func__);
            return iRemoteObject_->AddDeathRecipient(recipient);
        }

    private:
        sptr<IPCObjectStub> ipcObjectStub_;
        sptr<IRemoteObject> iRemoteObject_;
    };
} // namespace OHOS::testserver
#endif // SESSION_TOKEN_H