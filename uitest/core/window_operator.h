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

#ifndef WINDOW_OPERATOR_H
#define WINDOW_OPERATOR_H

#include "ui_driver.h"

namespace OHOS::uitest {
    enum ResizeDirection : uint8_t {
        LEFT,
        RIGHT,
        D_UP,
        D_DOWN,
        LEFT_UP,
        LEFT_DOWN,
        RIGHT_UP,
        RIGHT_DOWN
    };

    class WindowOperator {
    public:
       WindowOperator(UiDriver &driver, const Window &window, UiOpArgs &options);

       /**Focus the window.*/
       void Focus(ApiReplyInfo &out);

       /**Move this Window to the specified points.*/
       void MoveTo(uint32_t endX, uint32_t endY, ApiReplyInfo &out);

       /**Resize this Window to the specified size for the specified direction.*/
       void Resize(int32_t width, int32_t highth, ResizeDirection direction, ApiReplyInfo &out);

       /**Change this Window into split screen mode*/
       void Split(ApiReplyInfo &out);

       /**Maximize this window.*/
       void Maximize(ApiReplyInfo &out);

       /**Resume this window.*/
       void Resume(ApiReplyInfo &out);

       /**Minimize this window.*/
       void Minimize(ApiReplyInfo &out);

       /**Close this window.*/
       void Close(ApiReplyInfo &out);

    private:
        UiDriver &driver_;
        const Window &window_;
        UiOpArgs &options_;

        /**Exhale the decoration bar of the current window.*/
        void CallBar(ApiReplyInfo &out);

        /**Click on the decoration bar of this window.*/
        void BarAction(size_t index, ApiReplyInfo &out);
    };
} // namespace OHOS::uitest

#endif