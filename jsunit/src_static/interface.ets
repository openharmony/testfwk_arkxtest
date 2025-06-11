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
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import { ConfigService } from './module/config/configService';
import { SuiteService } from './module/service/SuiteService';
import { SpecService } from './module/service/SpecService';
import { ExpectService, AssertMatcher } from './module/service/ExpectService';
import { ReportService } from './module/service/ReportService';
import { OhReport } from './module/report/OhReport';
import { ExpectExtend } from './module/assert/ExpectExtend';
import { AnyType, ItfnType, NumberType } from './module/types/common';

import { SpecEvent, TaskEvent, SuiteEvent } from './module/service/event';
import { Core } from './core';
import { AssertResult } from './module/modal/assertModel';

export interface ConfigIf {
  size?: string;
  level?: string;
  testType?: string;
  timeout?: string;
  className?: string;
  notClass?: string;
  flag?: boolean;
  suite?: string;
  itName?: string;
  filter?: int;
  dryRun?: string;
  breakOnError?: string;
  random?: string;
  stress?: string;
  coverage?: string;
  skipMessage?: string;
  runSkipped?: string;
}

export interface StaticServiceIF {
  description: string;
}

export interface StaticSpecIF {
  description: string;
  fi: int;
  fn?: () => void;
  asyncFn?: (() => Promise<void>) | ((params: AnyType) => Promise<void>);
}

export interface ServiceAttrIF {
  id: string;
}

export interface ItItemIF {
  currentThreadName: string;
  description: string;
  result: byte;
}

export interface TestcaseSummaryIF {
  total: int;
  failure: int;
  error: int;
  pass: int;
  ignore: int;
  duration: int;
  itItemList: Array<ItItemIF>;
}

export interface CoreServiceIF {
  suite?: SuiteService;
  spec?: SpecService;
  expect?: ExpectService | ExpectExtend;
  report?: ReportService | OhReport;
  config?: ConfigService;
}

export interface CoreEventIF {
  suite?: SuiteEvent;
  spec?: SpecEvent;
  task?: TaskEvent;
}

export interface EventAttrIF {
  id: string;
  coreContext: Core;
}

export interface SuiteReportIF {
  tag: string;
  name?: string;
  errors?: int;
  tests?: int;
  failures?: int;
  time?: string;
  timestamp?: string;
  children?: Array<SuiteReportIF>;
  type?: string;
  message?: string;
}

export interface CaseReportIF {
  tag: string;
  type: string;
  message: string;
}

export interface ReportAttrIF {
  delegator: abilityDelegatorRegistry.AbilityDelegator;
  abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs;
}

export interface AssertMatcherIF {
  assertClose: (actualValue: number, expected: number[]) => AssertResult;
  assertContain: (actualValue: AnyType, expect: AnyType[]) => AssertResult;
  assertFail: () => AssertResult;
  assertTrue: (actualValue: boolean) => AssertResult;
  assertFalse: (actualValue: boolean) => AssertResult;
  assertInstanceOf: (actualValue: AnyType, expected: string[]) => AssertResult;
  assertLarger: (actualValue: number, expected: number[]) => AssertResult;
  assertLess: (actualValue: number, expected: number[]) => AssertResult;
  assertNull: (actualValue: AnyType) => AssertResult;
  assertThrow: (actualValue: () => void, args: string[]) => AssertResult;
  assertThrowError: (actualValue: () => void, args: string[]) => AssertResult;
  assertUndefined: (actualValue: AnyType) => AssertResult;
  assertLargerOrEqual: (actualValue: number, expected: number[]) => AssertResult;
  assertLessOrEqual: (actualValue: number, expected: number[]) => AssertResult;
  assertNaN: (actualValue: number) => AssertResult;
  assertNegUnlimited: (actualValue: number) => AssertResult;
  assertPosUnlimited: (actualValue: number) => AssertResult;
  assertEqual: (actualValue: AnyType, expect: AnyType[]) => AssertResult;
  assertDeepEquals: (actualValue: AnyType, expected: AnyType[]) => AssertResult;
  assertPromiseIsPending: (actualPromise: Promise<AnyType>) => Promise<AssertResult>;
  assertPromiseIsRejected: (actualPromise: Promise<AnyType>) => Promise<AssertResult>;
  assertPromiseIsRejectedWith: (actualPromise: Promise<AnyType>, expectedValue: AnyType[]) => Promise<AssertResult>;
  assertPromiseIsRejectedWithError: (actualPromise: Promise<AnyType>, expectedValue: string[]) => Promise<AssertResult>;
  assertPromiseIsResolved: (actualPromise: Promise<AnyType>) => Promise<AssertResult>;
  assertPromiseIsResolvedWith: (actualPromise: Promise<AnyType>, expectedValue: AnyType[]) => Promise<AssertResult>;
}

export interface WrappedMatchersIF {
  isNot: boolean;
  // 翻转方法
  not: () => ExpectService;
  message: (msg: string) => ExpectExtend;
}

export interface FilterParamIF {
  size: Map<string, int>;
  level: int[];
  testType: Map<string, int>;
}

export interface ApiIF {
  name: string;
  describe?: (desc: string, func: (() => void) | (() => Promise<void>)) => Promise<undefined>;
  xdescribe?: (desc: string, func: (() => void) | (() => Promise<void>), reason: string) => Promise<void>;
  beforeItSpecified?: (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => void;
  afterItSpecified?: (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => void;
  beforeAll?: (func: (() => void) | (() => Promise<void>)) => void;
  beforeEach?: (func: (() => void) | (() => Promise<void>)) => void;
  afterAll?: (func: (() => void) | (() => Promise<void>)) => void;
  afterEach?: (func: (() => void) | (() => Promise<void>)) => void;
  it?: (desc: string, filter: int, func: ItfnType) => void;
  xit?: (desc: string, filter: int, func: ItfnType, reason: string) => void;
  expect?: (actualValue?: AnyType) => AssertMatcher;
}

export interface DryrunResultIF {
  suites: Array<Map<string, Array<ConfigIf>>>;
  skipSuites?: Array<Map<string, Array<ConfigIf>>>;
}

type allExpectType = Object | undefined | null;
export interface Assert {
  assertClose(expectValue: number, precision: number): void;
  assertContain(expectValue: allExpectType): void;
  assertEqual(expectValue: allExpectType): void;
  assertFail(): void;
  assertFalse(): void;
  assertTrue(): void;
  assertInstanceOf(expectValue: string): void;
  assertLarger(expectValue: number): void;
  assertLess(expectValue: number): void;
  assertNull(): void;
  assertThrow(expectValue: string): void;
  assertThrowError(...expectValue: string[]): void;
  assertUndefined(): void;
  assertLargerOrEqual(expectValue: number): void;
  assertLessOrEqual(expectValue: number): void;
  assertNaN(): void;
  assertNegUnlimited(): void;
  assertPosUnlimited(): void;
  not(): Assert;
  assertDeepEquals(expectValue: allExpectType): void;
  assertPromiseIsPending(): Promise<void>;
  assertPromiseIsRejected(): Promise<void>;
  assertPromiseIsRejectedWith: (expectValue: allExpectType) => Promise<void>;
  assertPromiseIsRejectedWithError(...expectValue: allExpectType[]): Promise<void>;
  assertPromiseIsResolved(): Promise<void>;
  assertPromiseIsResolvedWith(expectValue?: allExpectType): Promise<void>;
  message(msg: string): Assert;
}

export interface whenResult {
  afterReturn: (value: allExpectType) => allExpectType;
  afterReturnNothing: () => undefined;
  afterAction: (action: allExpectType) => allExpectType;
  afterThrow: (e_msg: string) => string;
}

export interface VerificationMode {
  times(count: Number): void;
  never(): void;
  once(): void;
  atLeast(count: Number): void;
  atMost(count: Number): void;
}

export interface ExtendAssert {
  pass: boolean;
  message: string;
  actualValue: AnyType;
  checkFunc: string;
}

export interface DataDriverSuite {
  describe?: string[];
  stress?: NumberType;
  it?: string;
  params: Record<string, AnyType> | Record<string, AnyType>[]
  items?: DataDriverSuite[];
}

export interface DataDriverData {
  suites?: DataDriverSuite[];
}

export interface DataDriverAttr {
  data: DataDriverData;
}
