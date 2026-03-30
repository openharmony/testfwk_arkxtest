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

import { TAG } from '../../Constant';
import { Driver } from '@ohos.UiTest';

class FailureCaptureService {
    static async captureOnFailure(suiteName, specName, error) {
        const timestamp = Date.now();
        const sanitizeName = (name) => name.replace(/[\/\\:*?"<>|]/g, '_');
        const safeSuiteName = sanitizeName(suiteName);
        const safeSpecName = sanitizeName(specName);
        const screenshotPath = `/data/storage/el2/base/${safeSuiteName}_${safeSpecName}_${timestamp}.png`;
        const layoutDumpPath = `/data/storage/el2/base/${safeSuiteName}_${safeSpecName}_${timestamp}.json`;
        const driver = Driver.create()
        try {
            console.info(`${TAG}Capturing screenshot to: ${screenshotPath}`);
            const resultForScreenCap = await driver.screenCap(screenshotPath);
            if (resultForScreenCap) {
                console.info(`${TAG}Screenshot saved successfully: ${screenshotPath}`);
            } else {
                console.error(`${TAG}Failed to save screenshot: ${screenshotPath}`);
            }
        } catch (e) {
            console.error(`${TAG}Screenshot capture error: ${e.message}`);
        }
        try {
            console.info(`${TAG}DumpLayout to: ${layoutDumpPath}`);
            const resultForDump = await driver.dumpLayout(layoutDumpPath);
            if (resultForDump) {
                console.info(`${TAG}Dump saved successfully: ${layoutDumpPath}`);
            } else {
                console.error(`${TAG}Failed to dumpLayout to: ${layoutDumpPath}`);
            }
        } catch (e) {
            console.error(`${TAG}DumpLayout error: ${e.message}`);
        }
    }
}

export default FailureCaptureService;
