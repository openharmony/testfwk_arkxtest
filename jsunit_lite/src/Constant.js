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

export const TAG = '[Hypium]';

export const DEFAULT = 0;

export const PrintTag = {
    OHOS_REPORT_WORKER_STATUS: 'OHOS_REPORT_WORKER_STATUS',
    OHOS_REPORT_ALL_RESULT: 'OHOS_REPORT_ALL_RESULT',
    OHOS_REPORT_ALL_CODE: 'OHOS_REPORT_ALL_CODE',
    OHOS_REPORT_ALL_STATUS: 'OHOS_REPORT_ALL_STATUS',
    OHOS_REPORT_RESULT: 'OHOS_REPORT_RESULT',
    OHOS_REPORT_CODE: 'OHOS_REPORT_CODE',
    OHOS_REPORT_STATUS: 'OHOS_REPORT_STATUS',
    OHOS_REPORT_SUM: 'OHOS_REPORT_SUM',
    OHOS_REPORT_STATUS_CODE: 'OHOS_REPORT_STATUS_CODE'
};

export const TestType = {
    FUNCTION: 1,
    PERFORMANCE: 2,
    POWER: 4,
    RELIABILITY: 8,
    SECURITY: 16,
    GLOBAL: 32,
    COMPATIBILITY: 64,
    USER: 128,
    STANDARD: 256,
    SAFETY: 512,
    RESILIENCE: 1024
};

export const Size = {
    SMALLTEST: 65536,
    MEDIUMTEST: 131072,
    LARGETEST: 262144
};

export const Level = {
    LEVEL0: 16777216,
    LEVEL1: 33554432,
    LEVEL2: 67108864,
    LEVEL3: 134217728,
    LEVEL4: 268435456
};
