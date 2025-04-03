/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef MOCK_CONTROLLER_H
#define MOCK_CONTROLLER_H

#include "ui_controller.h"
#include "ui_model.h"
#include "element_node_iterator.h"

namespace OHOS::uitest {
    class MockController : public UiController {
    public:
        static std::unique_ptr<PointerMatrix> touch_event_records_;
        MockController() : UiController() {}

        ~MockController() = default;

        void GetUiWindows(std::map<int32_t, vector<Window>> &out, int32_t targetDisplay) override
        {
            vector<Window> winInfos;
            for (auto iter = testIn.cbegin(); iter != testIn.cend(); ++iter) {
                winInfos.emplace_back(iter->second);
            }
            out.insert(make_pair(0, move(winInfos)));
        }

        bool GetWidgetsInWindow(const Window &winInfo,
                                std::unique_ptr<ElementNodeIterator> &elementNodeIterator,
                                AamsWorkMode mode) override
        {
            // copy ele
            auto eleCopy = windowNodeMap.at(winInfo.id_);
            elementNodeIterator = std::make_unique<MockElementNodeIterator>(eleCopy);
            return true;
        }

        bool IsWorkable() const override
        {
            return true;
        }

        void InjectTouchEventSequence(const PointerMatrix &events) const override
        {
            touch_event_records_ = std::make_unique<PointerMatrix>(events.GetFingers(), events.GetSteps());
            for (int step = 0; step < events.GetSteps(); step++) {
                for (int finger = 0; finger < events.GetFingers(); finger++) {
                    touch_event_records_->PushAction(events.At(finger, step));
                }
            }
        }

        void AddWindowsAndNode(Window in, std::vector<MockAccessibilityElementInfo> eles)
        {
            testIn.emplace(in.id_, in);
            windowNodeMap.emplace(in.id_, eles);
        }
        void RemoveWindowsAndNode(Window in)
        {
            testIn.erase(in.id_);
            windowNodeMap.erase(in.id_);
        }

    private:
        std::map<int, Window> testIn;
        std::map<int, std::vector<MockAccessibilityElementInfo>> windowNodeMap;
    };
} // namespace OHOS::uitest
#endif