/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
import { AnyType, ItfnType } from './module/types/common';

const core = Core.getInstance();
const hypiumDescribe = (desc: string, func: (() => void) | (() => Promise<void>)): Promise<undefined> => {
  if (core) {
    return (core as Core).describe(desc, func);
  }
};

const hypiumIt = (desc: string, filter: int, func: ItfnType) => {
  if (core) {
    return (core as Core).it(desc, filter, func);
  }
};
const hypiumBeforeItSpecified = (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).beforeItSpecified(itDescs, func);
  }
};

const hypiumAfterItSpecified = (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).afterItSpecified(itDescs, func);
  }
};
const hypiumBeforeEach = (func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).beforeEach(func);
  }
};
const hypiumAfterEach = (func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).afterEach(func);
  }
};
const hypiumBeforeAll = (func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).beforeAll(func);
  }
};
const hypiumAfterAll = (func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).afterAll(func);
  }
};
const hypiumExpect = (actualValue?: AnyType) => {
  if (core) {
    const expect = (core as Core).expect;
    if (expect) {
      return expect(actualValue);
    }
  }
};

const hypiumXdescribe = (desc: string, func: (() => void) | (() => Promise<void>)) => {
  if (core) {
    return (core as Core).xdescribe(desc, func, '');
  }
};
const hypiumXdescribeReason = (reason: string) => {
  return (desc: string, func: (() => void) | (() => Promise<void>)) => {
    if (core) {
      return (core as Core).xdescribe(desc, func, reason);
    }
  };
};
const hypiumXit = (desc: string, filter: int, func: ItfnType) => {
  if (core) {
    return (core as Core).xit(desc, filter, func, '');
  }
};
const hypiumXitReason = (reason: string) => {
  return (desc: string, filter: int, func: (() => void) | (() => Promise<void>)) => {
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
  hypiumAfterEach,
  hypiumAfterAll,
  hypiumExpect,
  hypiumBeforeItSpecified,
  hypiumAfterItSpecified,
  hypiumXdescribe,
  hypiumXit,
  hypiumXdescribeReason,
  hypiumXitReason,
};
