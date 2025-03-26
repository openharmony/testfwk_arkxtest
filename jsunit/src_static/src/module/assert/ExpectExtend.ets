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
import { ServiceAttrIF, AssertMatcherIF, ApiIF } from '../../interface';
import { Core } from '../../core';
import { assertNull } from './assertNull';
import { assertClose } from './assertClose';
import { assertContain } from './assertContain';
import { assertLess } from './assertLess';
import { assertLarger } from './assertLarger';
import { assertFail } from './assertFail';
import { assertUndefined } from './assertUndefined';
import { assertFalseFn } from './assertFalse';
import { assertTrueFn } from './assertTrue';
import { assertEqual } from './assertEqual';
import { assertInstanceOf } from './assertInstanceOf';
import { assertThrowError } from './assertThrowError';
import { assertLargerOrEqual } from './assertLargerOrEqual';
import { assertLessOrEqual } from './assertLessOrEqual';
import { assertNaN } from './assertNaN';
import { assertNegUnlimited } from './assertNegUnlimited';
import { assertPosUnlimited } from './assertPosUnlimited';
import { assertDeepEquals } from './deepEquals/assertDeepEquals';
import { assertPromiseIsPending } from './assertPromiseIsPending';
import { assertPromiseIsRejected } from './assertPromiseIsRejected';
import { assertPromiseIsRejectedWith } from './assertPromiseIsRejectedWith';
import { assertPromiseIsRejectedWithError } from './assertPromiseIsRejectedWithError';
import { assertPromiseIsResolved } from './assertPromiseIsResolved';
import { assertPromiseIsResolvedWith } from './assertPromiseIsResolvedWith';
import { AnyType } from '../types/common';
import { ExpectService } from '../service/ExpectService';
import { assertThrow } from './assertThrow';

class ExpectExtend {
  public id: string;
  public matchers: AssertMatcherIF | null;
  public coreContext: Core | null;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.matchers = null;
    this.coreContext = null;
  }

  extendsMatchers() {
    const matcher: AssertMatcherIF = {
      assertClose,
      assertFail,
      assertContain,
      assertTrue: assertTrueFn,
      assertFalse: assertFalseFn,
      assertInstanceOf,
      assertLarger,
      assertLess,
      assertNull,
      assertUndefined,
      assertLargerOrEqual,
      assertLessOrEqual,
      assertNaN,
      assertNegUnlimited,
      assertPosUnlimited,
      assertEqual,
      assertDeepEquals,
      assertThrow,
      assertThrowError,
      assertPromiseIsPending,
      assertPromiseIsRejected,
      assertPromiseIsRejectedWith,
      assertPromiseIsRejectedWithError,
      assertPromiseIsResolved,
      assertPromiseIsResolvedWith,
    };
    this.matchers = matcher;
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
    this.extendsMatchers();
    const expectService = coreContext.getDefaultService('expect');
    if (expectService !== null) {
      const matchers = this.matchers;
      if (matchers) {
        (expectService as ExpectService).addMatchers(matchers);
      }
    }
  }

  apis(): ApiIF {
    return {
      name: 'ExpoctExtend',
      expect: (actualValue: AnyType) => {
        const core = this.coreContext;
        if (core) {
          const coreContext = core as Core;
          const expect = coreContext.getDefaultService('expect');
          if (expect) {
            return (expect as ExpectService).expect(actualValue);
          }
        }
      },
    };
  }
}
export { ExpectExtend };
