/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#include "ui_controller.h"

namespace OHOS::uitest {
    using namespace std;
    mutex UiController::controllerAccessMutex_;
    list<unique_ptr<UiController>> UiController::controllers_;
    UiControllerProvider UiController::controllerProvider_;
    set<string> UiController::controllerInstalledDevices_;

    UiController::UiController(string_view name, string_view device) : name_(name), targetDevice_(device) {}

    void UiController::RegisterControllerProvider(UiControllerProvider provider)
    {
        controllerProvider_ = move(provider);
    }

    void UiController::RegisterController(unique_ptr<UiController> controller, Priority priority)
    {
        if (controller == nullptr) { return; }
        LOG_I("Add controller '%{public}s', priority=%{public}d", controller->GetName().c_str(), priority);
        controller->SetPriority(priority);
        lock_guard<mutex> guard(controllerAccessMutex_);
        controllers_.push_back(move(controller));
        // sort controllers by priority descending
        controllers_.sort(UiController::Comparator);
    }

    void UiController::InstallForDevice(string_view device)
    {
        string name(device);
        if (!device.empty() && controllerInstalledDevices_.find(name) == controllerInstalledDevices_.end()) {
            // install controller on-demand
            if (controllerProvider_ != nullptr) {
                LOG_I("Begin to install UiControllers for device '%{public}s' with registered provider", name.c_str());
                auto receiver = list<unique_ptr<UiController>>();
                controllerProvider_(device, receiver);
                for (auto &uPtr:receiver) {
                    const auto priority = uPtr->priority_;
                    RegisterController(move(uPtr), priority);
                }
                controllerInstalledDevices_.insert(move(name));
            }
        }
    }

    const UiController *UiController::GetController(string_view targetDevice)
    {
        lock_guard<mutex> guard(controllerAccessMutex_);
        for (auto &ctrl:controllers_) {
            if (ctrl->targetDevice_ == targetDevice && ctrl->IsWorkable()) {
                return ctrl.get();
            }
        }
        return nullptr;
    }

    bool UiController::Comparator(const unique_ptr<UiController> &c1, const unique_ptr<UiController> &c2)
    {
        return c1->priority_ > c2->priority_;
    }

    void UiController::RemoveController(string_view name)
    {
        lock_guard<mutex> guard(controllerAccessMutex_);
        auto iter = controllers_.begin();
        while (iter != controllers_.end()) {
            if ((*iter)->name_ == name) {
                iter = controllers_.erase(iter);
            } else {
                iter++;
            }
        }
    }

    void UiController::RemoveAllControllers()
    {
        lock_guard<mutex> guard(controllerAccessMutex_);
        controllers_.clear();
    }
}
