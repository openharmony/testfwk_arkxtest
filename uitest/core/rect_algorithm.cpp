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

#include <algorithm>
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;

    bool RectAlgorithm::ComputeIntersection(const Rect &ra, const Rect &rb, Rect &result)
    {
        if (ra.left_ >= rb.right_ || ra.right_ <= rb.left_) {
            return false;
        }
        if (ra.top_ >= rb.bottom_ || ra.bottom_ <= rb.top_) {
            return false;
        }
        array<int32_t, INDEX_FOUR> px = {ra.left_, ra.right_, rb.left_, rb.right_};
        array<int32_t, INDEX_FOUR> py = {ra.top_, ra.bottom_, rb.top_, rb.bottom_};
        sort(px.begin(), px.end());
        sort(py.begin(), py.end());
        result = {px[INDEX_ONE], px[INDEX_TWO], py[INDEX_ONE], py[INDEX_TWO]};
        return true;
    }

    bool RectAlgorithm::ComputeMaxVisibleRegion(const Rect &origRect, const vector<Rect> &overlays, Rect &out)
    {
        vector<Rect> result;
        vector<Rect> current = {origRect};
        for (auto obstacle : overlays) {
            vector<Rect> next;
            for (auto rect : current) {
                Rect intersection(0, 0, 0, 0) ;
                if (!RectAlgorithm::ComputeIntersection(rect, obstacle, intersection)) {
                    next.push_back(rect);
                    continue;
                }
                    if (intersection.bottom_ < rect.bottom_) {
                        next.push_back(Rect(rect.left_, rect.right_, intersection.bottom_, rect.bottom_));
                    }
                    if (rect.top_ < intersection.top_) {
                        next.push_back(Rect(rect.left_, rect.right_, rect.top_, intersection.top_));
                    }
                    if (rect.left_ < intersection.left_) {
                        next.push_back(Rect(rect.left_, intersection.left_, rect.top_, rect.bottom_));
                    }
                    if (intersection.right_ < rect.right_) {
                        next.push_back(Rect(intersection.right_, rect.right_, rect.top_, rect.bottom_));
                }
            }
            current = next;
        }
        for (auto rect : current) {
            if (rect.IsValid()) {
                result.push_back(rect);
            }
        }
        Rect maxRect(0, 0, 0, 0);
        for (auto rect : result) {
            if (rect.GetArea() > maxRect.GetArea()) {
                maxRect = rect;
            }
        }
        out = maxRect;
        return out.GetArea() > 0;
    }
} // namespace OHOS::uitest
