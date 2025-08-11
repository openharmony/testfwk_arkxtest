'use static'
/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

/**
 * define the testcase type : TestType, Size , Level
 */
export const TAG: string = '[Hypium]';

export const DEFAULT: byte = 0B0000;

export class PrintTag {
    static OHOS_REPORT_WORKER_STATUS: string = 'OHOS_REPORT_WORKER_STATUS';
    static OHOS_REPORT_ALL_RESULT: string = 'OHOS_REPORT_ALL_RESULT';
    static OHOS_REPORT_ALL_CODE: string = 'OHOS_REPORT_ALL_CODE';
    static OHOS_REPORT_ALL_STATUS: string = 'OHOS_REPORT_ALL_STATUS';
    static OHOS_REPORT_RESULT: string = 'OHOS_REPORT_RESULT';
    static OHOS_REPORT_CODE: string = 'OHOS_REPORT_CODE';
    static OHOS_REPORT_STATUS: string = 'OHOS_REPORT_STATUS';
    static OHOS_REPORT_SUM: string = 'OHOS_REPORT_SUM';
    static OHOS_REPORT_STATUS_CODE: string = 'OHOS_REPORT_STATUS_CODE';
};

export class TestType {
    static FUNCTION: short = 0B1;
    static PERFORMANCE: short = 0B1 << 1;
    static POWER: short = 0B1 << 2;
    static RELIABILITY: short = 0B1 << 3;
    static SECURITY: short = 0B1 << 4;
    static GLOBAL: short = 0B1 << 5;
    static COMPATIBILITY: short = 0B1 << 6;
    static USER: short = 0B1 << 7;
    static STANDARD: short = 0B1 << 8;
    static SAFETY: short = 0B1 << 9;
    static RESILIENCE: short = 0B1 << 10;
};

export class Size {
    static SMALLTEST: int = 0B1 << 16;
    static MEDIUMTEST: int = 0B1 << 17;
    static LARGETEST: int = 0B1 << 18;
};

export class Level {
    static LEVEL0: int = 0B1 << 24;
    static LEVEL1: int = 0B1 << 25;
    static LEVEL2: int = 0B1 << 26;
    static LEVEL3: int = 0B1 << 27;
    static LEVEL4: int = 0B1 << 28;
};

const TESTTYPE: Map<string, int> = new Map<string, int>()
TESTTYPE.set("function", 1)
TESTTYPE.set("performance", 1 << 1)
TESTTYPE.set("power", 1 << 2)
TESTTYPE.set("reliability", 1 << 3)
TESTTYPE.set("security", 1 << 4)
TESTTYPE.set("global", 1 << 5)
TESTTYPE.set("compatibility", 1 << 6)
TESTTYPE.set("user", 1 << 7)
TESTTYPE.set("standard", 1 << 8)
TESTTYPE.set("safety", 1 << 9)
TESTTYPE.set("resilience", 1 << 10)
export TESTTYPE


export const LEVEL: int[] = [1 << 24, 1 << 25, 1 << 26, 1 << 27, 1 << 28]

const SIZE: Map<string, int> = new Map<string, int>()
SIZE.set("small", 1 << 16)
SIZE.set("medium", 1 << 17)
SIZE.set("large", 1 << 18)
export SIZE

export const KEYSET: string[] = [
    '-s class', '-s notClass', '-s suite', '-s itName',
    '-s level', '-s testType', '-s size', '-s timeout',
    '-s dryRun', '-s random', '-s breakOnError', '-s stress',
    '-s coverage', '-s skipMessage', '-s runSkipped',
    'class', 'notClass', 'suite', 'itName',
    'level', 'testType', 'size', 'timeout', 'dryRun', 'random',
    'breakOnError', 'stress', 'coverage', 'skipMessage', 'runSkipped'
];
