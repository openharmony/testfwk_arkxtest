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

import { Core } from './core';
import { AnyType, HookFuncType, ItfnType } from './module/types/common';
import { AssertMatcher } from './module/service/ExpectService';

const hypiumDescribe: (desc: string, func: () => void) => void = (desc: string, func: () => void): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).describe(desc, func);
  }
};

const hypiumIt: (desc: string, filter: int, func: ItfnType, timeout?: number, tag?: string) => void = (desc: string, filter: int, func: ItfnType, timeout?: number, tag?: string): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).it(desc, filter, func, timeout, tag);
  }
};
const hypiumBeforeItSpecified: (itDescs: string | string[], func: HookFuncType) => void = (itDescs: string | string[], func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).beforeItSpecified(itDescs, func);
  }
};

const hypiumAfterItSpecified: (itDescs: string | string[], func: HookFuncType) => void = (itDescs: string | string[], func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).afterItSpecified(itDescs, func);
  }
};
const hypiumBeforeEach: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).beforeEach(func);
  }
};
const hypiumBeforeEachIt: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).beforeEachIt(func);
  }
};
const hypiumAfterEach: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).afterEach(func);
  }
};
const hypiumAfterEachIt: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).afterEachIt(func);
  }
};
const hypiumBeforeAll: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).beforeAll(func);
  }
};
const hypiumAfterAll: (func: HookFuncType) => void = (func: HookFuncType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).afterAll(func);
  }
};
const hypiumExpect: (actualValue?: AnyType) => AssertMatcher = (actualValue?: AnyType): AssertMatcher => {
  const core = Core.getInstance();
  if (core) {
    const expect = (core as Core).expect;
    if (expect) {
      return expect(actualValue);
    }
    throw new Error('[hypium] expect() used before initialization');
  }
  throw new Error('[hypium] core not initialized');
};

const hypiumXdescribe: (desc: string, func: () => void) => void = (desc: string, func: () => void): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).xdescribe(desc, func, '');
  }
};
const hypiumXdescribeReason: (reason: string) => (desc: string, func: () => void) => void = (reason: string): (desc: string, func: () => void) => void => {
  const core = Core.getInstance();
  return (desc: string, func: () => void): void => {
    if (core) {
      return (core as Core).xdescribe(desc, func, reason);
    }
  };
};
const hypiumXit: (desc: string, filter: int, func: ItfnType) => void = (desc: string, filter: int, func: ItfnType): void => {
  const core = Core.getInstance();
  if (core) {
    return (core as Core).xit(desc, filter, func, '');
  }
};
const hypiumXitReason: (reason: string) => (desc: string, filter: int, func: (() => void) | (() => Promise<void>)) => void = (reason: string): (desc: string, filter: int, func: (() => void) | (() => Promise<void>)) => void => {
  const core = Core.getInstance();
  return (desc: string, filter: int, func: (() => void) | (() => Promise<void>)): void => {
    if (core) {
      return (core as Core).xit(desc, filter, func, reason);
    }
  };
};

export {
  hypiumDescribe,
  hypiumIt,
  hypiumBeforeAll,
  hypiumBeforeEach,
  hypiumBeforeEachIt,
  hypiumAfterEach,
  hypiumAfterEachIt,
  hypiumAfterAll,
  hypiumExpect,
  hypiumBeforeItSpecified,
  hypiumAfterItSpecified,
  hypiumXdescribe,
  hypiumXit,
  hypiumXdescribeReason,
  hypiumXitReason,
};
