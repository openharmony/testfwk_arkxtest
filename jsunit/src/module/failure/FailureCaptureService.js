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

import { TAG } from '../../Constant';
import { Driver } from '@ohos.UiTest';

class FailureCaptureService {
    static async captureOnFailure(suiteName, specName, error) {
        const timestamp = Date.now();
        const savePath1 = `/data/storage/el2/base/${suiteName}_${specName}_${timestamp}.png`;
        try {
            console.info(`${TAG}Capturing screenshot to: ${savePath1}`);
            const driver = Driver.create()
            const success1 = await driver.screenCap(savePath1);
            if (success1) {
                console.info(`${TAG}Screenshot saved successfully: ${savePath1}`);
            } else {
                console.error(`${TAG}Failed to save screenshot: ${savePath1}`);
            }
        } catch (e) {
            console.error(`${TAG}Screenshot capture error: ${e.message}`);
        }

        const savePath2 = `/data/storage/el2/base/${suiteName}_${specName}_${timestamp}.json`;
        try {
            console.info(`${TAG}DumpLayout to: ${savePath2}`);
            const driver = Driver.create()
            const success2 = await driver.dumpLayout(savePath2);
            if (success2) {
                console.info(`${TAG}Dump saved successfully: ${savePath2}`);
            } else {
                console.error(`${TAG}Failed to dumpLayout to: ${savePath2}`);
            }
        } catch (e) {
            console.error(`${TAG}DumpLayout error: ${e.message}`);
        }
    }
}

export default FailureCaptureService;
