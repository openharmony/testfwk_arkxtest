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
import fs from '@ohos.file.fs';

export async function collectCoverageData() {
    if (globalThis.__coverage__ === undefined) {
        return;
    }
    const strJson = JSON.stringify(globalThis.__coverage__);

    let savePath = coverageCollectHelp.getSavePath();
    let readPath = coverageCollectHelp.getReadPath();


    console.info("write coverage data to:", savePath);
    console.info("get coverage data in:", readPath);

    let file = fs.openSync(savePath, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
    let writeLen = fs.writeSync(file.fd, strJson, {encoding:"utf-8"});
    console.info("write coverage data success:" + writeLen);
    fs.closeSync(file);

    const OHOS_REPORT_COVERAGE_PATH = 'OHOS_REPORT_COVERAGE_PATH:';
    await SysTestKit.print(`${OHOS_REPORT_COVERAGE_PATH} ${readPath}`);
    console.info(`${OHOS_REPORT_COVERAGE_PATH} ${readPath}`);
}

class CoverageCollectHelp {
    constructor() {
        this.savePath = "";
        this.readPath = "";
    }

    setSavePath(savePath) {
        this.savePath = savePath;
    }

    getSavePath() {
        return this.savePath;
    }

    setReadPath(readPath) {
        this.readPath = readPath;
    }

    getReadPath() {
        return this.readPath;
    }
}

let coverageCollectHelp = new CoverageCollectHelp();
export default coverageCollectHelp;