/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "find_widget.h"
using namespace std;
namespace OHOS::uitest {
    std::unique_ptr<Widget> FindWidget(UiDriver &driver, float x, float y)
    {
        ApiCallErr err(NO_ERROR);
        std::vector<std::unique_ptr<Widget>> rec;
        WidgetSelector selector{};
        selector.SetWantMulti(true);
        driver.FindWidgets(selector, rec, err, true);
        if (err.code_ != NO_ERROR) {
            return nullptr;
        }
        int maxDep = 0;
        int maxIndex = -1;
        for (int index = 0; index < rec.size(); index++) {
            const auto &rect = rec[index]->GetBounds();
            if (!(x <= rect.right_ && x >= rect.left_ && y <= rect.bottom_ && y >= rect.top_)) {
                continue;
            }
            int curDep = rec[index]->GetHierarchy().length();
            if (curDep > maxDep) {
                maxDep = curDep;
                maxIndex = index;
            }
        }
        if (maxIndex > -1) {
            return std::move(rec[maxIndex]);
        } else {
            return nullptr;
        }
    }
} // namespace OHOS::uitest