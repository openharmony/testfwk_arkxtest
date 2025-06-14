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

import { AssertResult } from '../modal/assertModel';
export function assertThrow(actual: () => void, args: string[]): AssertResult {
  let message = '';
  let pass = false;
  if (typeof actual !== 'function') {
    message = "toThrow's Actual should be a Function";
  } else {
    let hasThrow = false;
    let throwError = new Error();
    try {
      actual();
    } catch (e: Error) {
      hasThrow = true;
      throwError = e;
    }
    if (!hasThrow) {
      message = 'function did not throw an exception';
    } else if (throwError && throwError.message === args[0]) {
      pass = true;
    } else {
      message = `expect to throw ${args[0]} , actual throw ${throwError.message}`;
    }
  }
  return { pass, message };
}
