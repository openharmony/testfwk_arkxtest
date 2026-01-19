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

import { ServiceAttrIF, AssertMatcherIF, ApiIF } from '../../interface';
import { StaticSpec } from './StaticSpec';
import { StaticSuite } from './StaticSuite';
import { TAG } from '../../Constant';
import { Core } from '../../core';
import { AssertException } from './AssertException';
import { LogExpectError } from '../report/LogExpectError';
import { AnyType, NumberType } from '../types/common';
import { AssertResult } from '../modal/assertModel';
import { SpecService } from './SpecService';
import { SuiteService } from './SuiteService';

class ExpectService {
  public id: string;
  public matchers: AssertMatcherIF | null;
  public customMatchers: Map<string, Function>;
  public coreContext: Core | null;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.matchers = null;
    this.customMatchers = new Map<string, Function>();
    this.coreContext = null;
  }

  expect(actualValue?: AnyType): AssertMatcher {
    const core = this.coreContext;
    let currentRunningSpec = new StaticSpec({ description: '', fi: 0 });
    let currentRunningSuite = new StaticSuite({ description: '' });
    if (core) {
      const specService = core.getDefaultService('spec');
      const suiteService = core.getDefaultService('suite');
      if (specService !== null) {
        const spec = (specService as SpecService).getCurrentRunningSpec();
        if (spec) {
          currentRunningSpec = spec;
        }
      }
      if (suiteService !== null) {
        currentRunningSuite = (suiteService as SuiteService).getCurrentRunningSuite();
      }
    }
    return new AssertMatcher(actualValue, currentRunningSpec, currentRunningSuite, this.matchers, this.customMatchers);
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
  }

  addMatchers(matchers: AssertMatcherIF) {
    this.matchers = matchers;
  }

  addCustomMatchers(customAssert: Function) {
    if (typeof customAssert !== 'function') {
      throw new Error('Custom assertion must be a function');
    }
    const customAssertName = customAssert.name;
    if (!customAssertName || customAssertName.trim() == '') {
      throw new Error('Please use a named function');
    }
    this.customMatchers.set(customAssertName, customAssert);
  }

  removeCustomMatchers(customAssertionName: string) {
    const func = this.customMatchers.get(customAssertionName)
    if(func) {
      this.customMatchers.delete(customAssertionName)
    }
  }

  apis(): ApiIF {
    const _this = this;
    return {
      name: 'ExpectService',
      expect: (actualValue?: AnyType) => {
        return _this.expect(actualValue);
      },
    };
  }
}

export class AssertMatcher {
  public actualValue: AnyType;
  public isNot: Boolean;
  public currentRunningSuite: StaticSuite;
  public currentRunningSpec: StaticSpec;
  public assertClose: (expectValue: NumberType, precision: NumberType) => void;
  public assertContain: (expectValue: AnyType) => void;
  public assertFail: () => void;
  public assertTrue: () => void;
  public assertFalse: () => void;
  public assertInstanceOf: (expectValue: string) => void;
  public assertLarger: (expectValue: NumberType) => void;
  public assertLess: (expectValue: NumberType) => void;
  public assertNull: () => void;
  public assertThrow: (expectValue: string) => void;
  public assertThrowError: (expectValue: string, expect?: string) => void;
  public assertUndefined: () => void;
  public assertLargerOrEqual: (expectValue: NumberType) => void;
  public assertLessOrEqual: (expectValue: NumberType) => void;
  public assertNaN: () => void;
  public assertNegUnlimited: () => void;
  public assertPosUnlimited: () => void;
  public assertDeepEquals: (expectValue: AnyType) => void;
  public assertEqual: (expectValue: AnyType) => void;
  public assertPromiseIsPending: () => Promise<void>;
  public assertPromiseIsRejected: () => Promise<void>;
  public assertPromiseIsRejectedWith: (expectValue: AnyType) => Promise<void>;
  public assertPromiseIsRejectedWithError: (expectValue: string, expectValue2?: string) => Promise<void>;
  public assertPromiseIsResolved: () => Promise<void>;
  public assertPromiseIsResolvedWith: (expectValue: string) => Promise<void>;
  public customAssert: Record<string, Function> = {};

  constructor(
    actualValue: AnyType,
    currentRunningSpec: StaticSpec,
    currentRunningSuite: StaticSuite,
    matchers: AssertMatcherIF | null,
    customAssertion: Map<string, Function>
  ) {
    this.isNot = false;
    this.currentRunningSpec = currentRunningSpec;
    this.currentRunningSuite = currentRunningSuite;
    this.actualValue = actualValue;
    this.assertClose = (expectValue: NumberType, precision: NumberType) => {};
    this.assertContain = (expectValue: AnyType) => {};
    this.assertFail = () => {};
    this.assertTrue = () => {};
    this.assertFalse = () => {};
    this.assertLarger = (expectValue: NumberType) => {};
    this.assertLess = (expectValue: NumberType) => {};
    this.assertLargerOrEqual = (expectValue: NumberType) => {};
    this.assertLessOrEqual = (expectValue: NumberType) => {};
    this.assertUndefined = () => {};
    this.assertNull = () => {};
    this.assertThrow = (expectValue: string) => {};
    this.assertThrowError = (expectValue: string, expect?: string) => {};
    this.assertNaN = () => {};
    this.assertNegUnlimited = () => {};
    this.assertPosUnlimited = () => {};
    this.assertDeepEquals = (expectValue: AnyType) => {};
    this.assertEqual = (expectValue: AnyType) => {};
    this.assertInstanceOf = (expectValue: string) => {};
    this.assertPromiseIsPending = () => {
      return Promise.resolve();
    };
    this.assertPromiseIsRejected = () => {
      return Promise.resolve();
    };
    this.assertPromiseIsResolved = () => {
      return Promise.resolve();
    };
    this.assertPromiseIsRejectedWith = (expectValue: AnyType) => {
      return Promise.resolve();
    };
    this.assertPromiseIsResolvedWith = (expectValue: AnyType) => {
      return Promise.resolve();
    };
    this.assertPromiseIsRejectedWithError = (expectValue: string, expectValue2?: string) => {
      return Promise.resolve();
    };
    if (matchers !== null) {
      this.addAssert(matchers);
    }
    this.addCustomAssert(customAssertion);
  }

  // 翻转方法
  not() {
    this.isNot = true;
    return this;
  }

  message(msg: string) {
    this.currentRunningSpec.expectMsg = msg;
    console.info(`${TAG} msg: ${msg}`);
    return this;
  }

  addCustomAssert(customMatcher: Map<string, Function>) {
    for (const key of customMatcher.keys()) {
      const customAssertFunc = customMatcher.get(key)
      if (customAssertFunc && typeof customAssertFunc === 'function') {
        const customFn = (matcherName: string, testFn: (actualValue: Object | undefined | null,
          ...expected: Array<Object | undefined | null>) => AssertResult) => {
          return (...args: Array<Object | undefined | null>) => {
            const result = testFn(this.actualValue, ...args);
            if (this.isNot) {
              result.pass = !result.pass;
              result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, args[0], result.message);
            }
            if (!result.pass) {
              const assertError = new AssertException(result.message);
              const currentRunningSpec = this.currentRunningSpec;
              const currentRunningSuite = this.currentRunningSuite;
              if (currentRunningSpec) {
                currentRunningSpec.fail = assertError;
              } else {
                currentRunningSuite.hookError = assertError;
              }
              throw assertError;
            }
          }
        }
        this.customAssert.set(key, customFn(key, customAssertFunc as (actualValue: Object | undefined | null,
          ...expected: Array<Object | undefined | null>) => AssertResult))
      }
    }
  }


  addCloseAssert(matchers: AssertMatcherIF) {
    const numFn = (matcherName: string, assertFn: (actualValue: number, expected: number[]) => AssertResult) => {
      return (arg1: NumberType, arg2: NumberType) => {
        const result = assertFn((this.actualValue as NumberType).toDouble(), [arg1.toDouble(), arg2.toDouble()]);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg1, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertClose = matchers.assertClose;

    if (assertClose) {
      this.assertClose = numFn('assertClose', assertClose);
    }
  }

  addNumAssert(matchers: AssertMatcherIF) {
    const numFn = (matcherName: string, assertFn: (actualValue: number, expected: number[]) => AssertResult) => {
      return (arg1: NumberType): void => {
        const result = assertFn((this.actualValue as NumberType).toDouble(), [arg1.toDouble()]);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg1, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };

    const assertLarger = matchers.assertLarger;
    if (assertLarger) {
      this.assertLarger = numFn('assertLarger', assertLarger);
    }

    const assertLess = matchers.assertLess;
    if (assertLess) {
      this.assertLess = numFn('assertLess', assertLess);
    }

    const assertLargerOrEqual = matchers.assertLargerOrEqual;
    if (assertLargerOrEqual) {
      this.assertLargerOrEqual = numFn('assertLargerOrEqual', assertLargerOrEqual);
    }

    const assertLessOrEqual = matchers.assertLessOrEqual;
    if (assertLessOrEqual) {
      this.assertLessOrEqual = numFn('assertLessOrEqual', assertLessOrEqual);
    }
  }

  addFailAssert(matchers: AssertMatcherIF) {
    const voidFn = (matcherName: string, assertFn: () => AssertResult) => {
      return () => {
        const result = assertFn();
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, 'undefined', result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };

    const assertFail = matchers.assertFail;
    if (assertFail) {
      this.assertFail = voidFn('assertFail', assertFail);
    }
  }

  addNumWithoutExpectAssert(matchers: AssertMatcherIF) {
    const nNumFn = (matcherName: string, assertFn: (actualValue: number) => AssertResult) => {
      return () => {
        const result = assertFn((this.actualValue as NumberType).toDouble());
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, 'undefined', result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertNaN = matchers.assertNaN;
    if (assertNaN) {
      this.assertNaN = nNumFn('assertNaN', assertNaN);
    }

    const assertNegUnlimited = matchers.assertNegUnlimited;
    if (assertNegUnlimited) {
      this.assertNegUnlimited = nNumFn('assertNegUnlimited', assertNegUnlimited);
    }

    const assertPosUnlimited = matchers.assertPosUnlimited;
    if (assertPosUnlimited) {
      this.assertPosUnlimited = nNumFn('assertPosUnlimited', assertPosUnlimited);
    }
  }

  addBooleanAssert(matchers: AssertMatcherIF) {
    const booleanFn = (matcherName: string, assertFn: (actualValue: boolean) => AssertResult) => {
      return () => {
        const result = assertFn(this.actualValue as boolean);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, 'undefined', result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertTrue = matchers.assertTrue;
    if (assertTrue) {
      this.assertTrue = booleanFn('assertTrue', assertTrue);
    }

    const assertFalse = matchers.assertFalse;
    if (assertFalse) {
      this.assertFalse = booleanFn('assertFalse', assertFalse);
    }
  }

  addAnytypeAssert(matchers: AssertMatcherIF) {
    const equalFn = (matcherName: string, assertFn: (actualValue: AnyType, expected: AnyType[]) => AssertResult) => {
      return (arg: AnyType) => {
        const result = assertFn(this.actualValue, [arg]);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertDeepEquals = matchers.assertDeepEquals;
    if (assertDeepEquals) {
      this.assertDeepEquals = equalFn('assertDeepEquals', assertDeepEquals);
    }

    const assertContain = matchers.assertContain;
    if (assertContain) {
      this.assertContain = equalFn('assertContain', assertContain);
    }

    const assertEqual = matchers.assertEqual;
    if (assertEqual) {
      this.assertEqual = equalFn('assertEqual', assertEqual);
    }

  }

  addInstanceOfAssert(matchers: AssertMatcherIF) {
    const equalFn = (matcherName: string, assertFn: (actualValue: AnyType, expected: string[]) => AssertResult) => {
      return (arg: string) => {
        const result = assertFn(this.actualValue, [arg]);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
   
    const assertInstanceOf = matchers.assertInstanceOf;
    if (assertInstanceOf) {
      this.assertInstanceOf = equalFn('assertInstanceOf', assertInstanceOf);
    }
  }

  addUndefinedAssert(matchers: AssertMatcherIF) {
    const undefinedFn = (matcherName: string, assertFn: (actualValue: AnyType) => AssertResult) => {
      return () => {
        const result = assertFn(this.actualValue);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, 'undefined', result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertUndefined = matchers.assertUndefined;
    if (assertUndefined) {
      this.assertUndefined = undefinedFn('assertUndefined', assertUndefined);
    }
    const assertNull = matchers.assertNull;
    if (assertNull) {
      this.assertNull = undefinedFn('assertNull', assertNull);
    }
  }

  addThrowAssert(matchers: AssertMatcherIF) {
    const throwFn = (matcherName: string, assertFn: (actualValue: () => void, args: string[]) => AssertResult) => {
      return (arg1: string, arg2?: string) => {
        let arr = [arg1];
        if (arg2 !== undefined) {
          arr = [arg1, arg2];
        }
        const result = assertFn(this.actualValue as () => void, arr);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg1, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertThrowError = matchers.assertThrowError;
    if (assertThrowError) {
      this.assertThrowError = throwFn('assertThrowError', assertThrowError);
    }
    const assertThrow = matchers.assertThrow;
    if (assertThrow) {
      this.assertThrow = throwFn('assertThrow', assertThrow);
    }
  }

  addPromiseAssert(matchers: AssertMatcherIF) {
    const promiseFn = (matcherName: string, assertFn: (actualPromise: Promise<AnyType>) => Promise<AssertResult>) => {
      return async (): Promise<void> => {
        const result: AssertResult = await assertFn(this.actualValue as Promise<AnyType>);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, 'undefined', result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertPromiseIsPending = matchers.assertPromiseIsPending;
    if (assertPromiseIsPending) {
      this.assertPromiseIsPending = promiseFn('assertPromiseIsPending', assertPromiseIsPending);
    }
    const assertPromiseIsRejected = matchers.assertPromiseIsRejected;
    if (assertPromiseIsRejected) {
      this.assertPromiseIsRejected = promiseFn('assertPromiseIsRejected', assertPromiseIsRejected);
    }
    const assertPromiseIsResolved = matchers.assertPromiseIsResolved;
    if (assertPromiseIsResolved) {
      this.assertPromiseIsResolved = promiseFn('assertPromiseIsResolved', assertPromiseIsResolved);
    }
  }

  addPromiseWithAssert(matchers: AssertMatcherIF) {
    const promiseFn = (
      matcherName: string,
      assertFn: (actualPromise: Promise<AnyType>, expectedValue: AnyType[]) => Promise<AssertResult>
    ) => {
      return async (arg: AnyType): Promise<void> => {
        const result: AssertResult = await assertFn(this.actualValue as Promise<AnyType>, [arg]);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };
    const assertPromiseIsResolvedWith = matchers.assertPromiseIsResolvedWith;
    if (assertPromiseIsResolvedWith) {
      this.assertPromiseIsResolvedWith = promiseFn('assertPromiseIsResolvedWith', assertPromiseIsResolvedWith);
    }
    const assertPromiseIsRejectedWith = matchers.assertPromiseIsRejectedWith;
    if (assertPromiseIsRejectedWith) {
      this.assertPromiseIsRejectedWith = promiseFn('assertPromiseIsRejectedWith', assertPromiseIsRejectedWith);
    }
  }

  addPromiseWithError(matchers: AssertMatcherIF) {
    const promiseFn = (
      matcherName: string,
      assertFn: (actualPromise: Promise<AnyType>, expectedValue: string[]) => Promise<AssertResult>
    ) => {
      return async (arg: string, arg1?: string): Promise<void> => {
        let arr = [arg];
        if (arg1 !== undefined) {
          arr = [arg, arg1];
        }
        const result: AssertResult = await assertFn(this.actualValue as Promise<AnyType>, arr);
        if (this.isNot) {
          result.pass = !result.pass;
          result.message = LogExpectError.getErrorMsg(matcherName, this.actualValue, arg, result.message);
        }
        if (!result.pass) {
          const assertError = new AssertException(result.message);
          const currentRunningSpec = this.currentRunningSpec;
          const currentRunningSuite = this.currentRunningSuite;
          if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
          } else {
            currentRunningSuite.hookError = assertError;
          }
          throw assertError;
        }
      };
    };

    const assertPromiseIsRejectedWithError = matchers.assertPromiseIsRejectedWithError;
    if (assertPromiseIsRejectedWithError) {
      this.assertPromiseIsRejectedWithError = promiseFn(
        'assertPromiseIsRejectedWithError',
        assertPromiseIsRejectedWithError
      );
    }
  }

  addAssert(matchers: AssertMatcherIF) {
    this.addCloseAssert(matchers);
    this.addNumAssert(matchers);
    this.addFailAssert(matchers);
    this.addNumWithoutExpectAssert(matchers);
    this.addBooleanAssert(matchers);
    this.addAnytypeAssert(matchers);
    this.addInstanceOfAssert(matchers)
    this.addThrowAssert(matchers);
    this.addUndefinedAssert(matchers);
    this.addPromiseAssert(matchers);
    this.addPromiseWithAssert(matchers);
    this.addPromiseWithError(matchers);
  }
}

export { ExpectService };
