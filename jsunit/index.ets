/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

import Core from './src/main/core';
import {ConfigService} from './src/main/module/config/configService'
// import {DEFAULT, TestType, Size, Level, TAG} from './src/main/Constant';
import DataDriver from './src/main/module/config/DataDriver';
import ExpectExtend from './src/main/module/assert/ExpectExtend';
import OhReport from './src/main/module/report/OhReport';
import SysTestKit from './src/main/module/kit/SysTestKit';
// import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect, beforeItSpecified, afterItSpecified} from './src/main/interface';
// import {MockKit, when} from './src/main/module/mock/MockKit';
// import ArgumentMatchers from './src/main/module/mock/ArgumentMatchers';

class Hypium {
    static setData(data:Object) {
        const core:Core = Core.getInstance();
        const dataDriver:DataDriver = new DataDriver({data});
        core.addService('dataDriver', dataDriver);
    }

    static setTimeConfig(systemTime:Object) {
        SysTestKit.systemTime = systemTime;
    }

    static hypiumTest(abilityDelegator:Object, abilityDelegatorArguments:Object, testsuite:Function) {
        const core:Core = Core.getInstance();
        const expectExtend:ExpectExtend = new ExpectExtend({
            id: 'extend'
        });
        core.addService('expect', expectExtend);
        const ohReport:OhReport = new OhReport({
            delegator: abilityDelegator,
            abilityDelegatorArguments: abilityDelegatorArguments
        });
        SysTestKit.delegator = abilityDelegator;
        core.addService('report', ohReport);
        core.init();
        core.subscribeEvent('spec', ohReport);
        core.subscribeEvent('suite', ohReport);
        core.subscribeEvent('task', ohReport);
        const configService:ConfigService = core.getDefaultService('config');
        if (abilityDelegatorArguments !== null) {
            let testParameters:Object = configService.translateParams((abilityDelegatorArguments as Record<string, Object>).parameters);
            console.info(`${TAG}parameters:${JSON.stringify(testParameters)}`);
            configService.setConfig(testParameters);
        }
        testsuite();
        core.execute(abilityDelegator);
    }
}

export {
    Hypium,
    Core,
    DataDriver,
    ExpectExtend,
    OhReport,
    SysTestKit,
};

type AllExpectType = Object | undefined | null;

export const DEFAULT = 0B0000

export enum TestType {
    FUNCTION = 0B1,
    PERFORMANCE = 0B1 << 1,
    POWER = 0B1 << 2,
    RELIABILITY = 0B1 << 3,
    SECURITY = 0B1 << 4,
    GLOBAL = 0B1 << 5,
    COMPATIBILITY = 0B1 << 6,
    USER = 0B1 << 7,
    STANDARD = 0B1 << 8,
    SAFETY = 0B1 << 9,
    RESILIENCE = 0B1 << 10
}

export enum Size {
    SMALLTEST = 0B1 << 16,
    MEDIUMTEST = 0B1 << 17,
    LARGETEST = 0B1 << 18
}

export enum Level {
    LEVEL0 = 0B1 << 24,
    LEVEL1 = 0B1 << 25,
    LEVEL2 = 0B1 << 26,
    LEVEL3 = 0B1 << 27,
    LEVEL4 = 0B1 << 28
}

export function describe(testSuiteName: string, callback: Function): void

export function beforeItSpecified(testCaseNames: Array<string> | string, callback: Function): void

export function afterItSpecified(testCaseNames: Array<string> | string, callback: Function): void

export function beforeEach(callback: Function): void

export function afterEach(callback: Function): void

export function beforeAll(callback: Function): void

export function afterAll(callback: Function): void

export function it(testCaseName: string, attribute: (TestType | Size | Level), callback: Function)

export interface Assert {
    assertClose(expectValue: number, precision: number): void
    assertContain(expectValue: AllExpectType): void
    assertEqual(expectValue: AllExpectType): void
    assertFail(): void
    assertFalse(): void
    assertTrue(): void
    assertInstanceOf(expectValue: string): void
    assertLarger(expectValue: number): void
    assertLess(expectValue: number): void
    assertNull(): void
    assertThrowError(expectValue: string): void
    assertUndefined(): void
    assertLargerOrEqual(expectValue: number):void
    assertLessOrEqual(expectValue: number):void
    assertNaN():void
    assertNegUnlimited(): void
    assertPosUnlimited(): void
    not(): Assert;
    assertDeepEquals(expectValue: AllExpectType):void
    assertPromiseIsPending(): void
    assertPromiseIsRejected(): void
    assertPromiseIsRejectedWith(expectValue?: AllExpectType): void
    assertPromiseIsRejectedWithError(...expectValue): void
    assertPromiseIsResolved(): void
    assertPromiseIsResolvedWith(expectValue?: AllExpectType): void
}

export function expect(actualValue?: AllExpectType): Assert

export class ArgumentMatchers {
    static any : AllExpectType;
    static anyString : AllExpectType;
    static anyBoolean : AllExpectType;
    static anyNumber : AllExpectType;
    static anyObj : AllExpectType;
    static anyFunction : AllExpectType;
    static matchRegexs(Regex: RegExp): void
}

declare interface When {
    afterReturn(value: AllExpectType): AllExpectType
    afterReturnNothing(): undefined
    afterAction(action: AllExpectType): AllExpectType
    afterThrow(e_msg: string): string
    (argMatchers?: AllExpectType): When;
}

export const when: When;

export interface VerificationMode {
    times(count: Number): void
    never(): void
    once(): void
    atLeast(count: Number): void
    atMost(count: Number): void
}

export class MockKit {
    constructor()
    mockFunc(obj: Object, func: Function): Function
    mockObject(obj: Object): Object
    verify(methodName: String, argsArray: Array<AllExpectType>): VerificationMode
    ignoreMock(obj: Object, func: Function): void
    clear(obj: Object): void
    clearAll(): void
}

// export class SysTestKit {
//     static actionStart(tag: string): void
//     static actionEnd(tag: string): void
//     static existKeyword(keyword: string, timeout?: number): boolean
// }
