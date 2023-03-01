/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

import SysTestKit from "../kit/SysTestKit";

export async function collectCoverageData() {
    const errorMsg = 'Coverage data generation failed. Please clean up the project and rerun';
    let strJson = globalThis.__coverage__ === undefined ? errorMsg : JSON.stringify(globalThis.__coverage__);
    const strLen = strJson.length;
    const maxLen = 500;
    const maxCount = Math.floor(strLen / maxLen);

    const OHOS_REPORT_COVERAGE_DATA = 'OHOS_REPORT_COVERAGE_DATA:';
    const OHOS_REPORT_ERROR_MESSAGE = 'OHOS_REPORT_ERROR_MESSAGE:';
    let OHOS_REPORT_COVERAGE_KEY = globalThis.__coverage__ === undefined ? OHOS_REPORT_ERROR_MESSAGE : OHOS_REPORT_COVERAGE_DATA;

    for (let count = 0; count <= maxCount; count++) {
        await SysTestKit.print(`${OHOS_REPORT_COVERAGE_KEY} ${strJson.substring(count * maxLen, (count + 1) * maxLen)}`);
    }
}