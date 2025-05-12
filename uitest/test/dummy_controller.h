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
#ifndef DUMMY_CONTROLLER_H
#define DUMMY_CONTROLLER_H

#include "ui_controller.h"

namespace OHOS::uitest {
class DummyEventMonitor {
public:
    static DummyEventMonitor &GetInstance()
    {
        static DummyEventMonitor instance;
        return instance;
    }

    static void OnEvent(string eventInfo)
    {
        UiEventSourceInfo uiEventSourceInfo { "", "", eventInfo };
        for (auto &listener : listeners_) {
            listener->OnEvent(eventInfo, uiEventSourceInfo);
        }
    }

    static void RegisterUiEventListener(shared_ptr<UiEventListener> listerner)
    {
        listeners_.push_back(listerner);
    }

    static uint32_t GetListenerCount()
    {
        return listeners_.size();
    }
private:
    DummyEventMonitor() = default;
    static vector<shared_ptr<UiEventListener>> listeners_;
};

class DummyController : public UiController {
public:
    DummyController() : UiController() {}

    ~DummyController() {}

    bool IsWorkable() const override
    {
        return workable_;
    }

    bool IsWearable() const override
    {
        return false;
    }

    void SetWorkable(bool wb)
    {
        this->workable_ = wb;
    }

    void RegisterUiEventListener(std::shared_ptr<UiEventListener> listener) const override
    {
        DummyEventMonitor::GetInstance().RegisterUiEventListener(listener);
    }

private:
    bool workable_ = false;
};
}

#endif
