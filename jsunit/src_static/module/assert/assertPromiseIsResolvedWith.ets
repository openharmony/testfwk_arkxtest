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
import { AnyType } from '../types/common';
export function assertPromiseIsResolvedWith(
  actualPromise: Promise<AnyType>,
  expectedValue: AnyType[]
): Promise<AssertResult> {
  const tips = (passed: boolean) => {
    return 'Expected a promise ' + (passed ? 'not ' : '') + 'to be resolved with ' + JSON.stringify(expectedValue[0]);
  };

  return actualPromise.then<AssertResult, AssertResult>(
    (got: AnyType): AssertResult => {
      if (JSON.stringify(got) == JSON.stringify(expectedValue[0])) {
        return {
          pass: true,
          message: 'actualValue was resolved with ' + JSON.stringify(got) + '.',
        };
      }
      return {
        pass: false,
        message: tips(false) + ' but it was resolved with ' + JSON.stringify(got) + '.',
      };
    },
    (rej: Error): AssertResult => {
      return {
        pass: false,
        message: tips(false) + ' but it was rejected with ' + JSON.stringify(rej) + '.',
      };
    }
  );
}
