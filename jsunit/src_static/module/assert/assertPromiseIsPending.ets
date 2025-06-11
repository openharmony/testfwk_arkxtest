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

export function assertPromiseIsPending(actualPromise: Promise<AnyType>): Promise<AssertResult> {
  const helper: object = {};
  return Promise.race([actualPromise as object, Promise.resolve(helper)]).then<AssertResult, AssertResult>(
    (got: object): AssertResult => {
      if (helper === got) {
        return { pass: true, message: 'actualValue is isPending' };
      } else {
        return { pass: false, message: 'expect isPending, actualValue is resolve' };
      }
    },
    (): AssertResult => {
      return { pass: false, message: 'expect isPending, actualValue is reject' };
    }
  );
}
