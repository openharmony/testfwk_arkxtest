'use static'
/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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

export { Hypium } from './hypium_static';
export { Assert } from './interface';
export {
  hypiumDescribe as describe,
  hypiumBeforeAll as beforeAll,
  hypiumBeforeEach as beforeEach,
  hypiumAfterEach as afterEach,
  hypiumAfterAll as afterAll,
  hypiumIt as it,
  hypiumExpect as expect,
  hypiumBeforeItSpecified as beforeItSpecified,
  hypiumAfterItSpecified as afterItSpecified,
  hypiumXdescribe as xdescribe,
  hypiumXit as xit,
  hypiumXdescribeReason as xdescribeReason,
  hypiumXitReason as xitReason,
} from './hypiumApi';
export { DEFAULT, TestType, Size, Level, TAG, PrintTag } from './Constant';
export { Core } from './core';
export { ExpectExtend } from './module/assert/ExpectExtend';
export { OhReport } from './module/report/OhReport';
export { SysTestKit } from './module/kit/SysTestKit';
export { DataDriver } from './module/config/DataDriver';
