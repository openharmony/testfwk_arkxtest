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

import hilog from '@ohos.hilog';
import { DeepTypeUtils } from '../assert/deepEquals/DeepTypeUtils'

let domain: int = 0x0000;
let tag: string = 'testTag';

export class MatcherInfo {
  type_: string;
  value: Object;
  needMatch: boolean;

  constructor(type_: string, value: Object, needMatch: boolean) {
    this.type_ = type_;
    this.value = value;
    this.needMatch = needMatch;
  }
}

export class ArgumentMatchers {
  static MATCH_REGEXS = "<match regexs>";
  static CONTAIN_ELEMENT = "<contain element>";
  static CONTAIN_SUBSTRING = "<contain subString>";
  static CONTAIN_SUBARRAY = "<contain subArray>";

  constructor() {
    hilog.info(domain, tag, '%{public}s ', "ArgumentMatchers constructor")
  }

  static any() {
  }

  static anyString() {
  }

  static anyBoolean() {
  }

  static anyNumber() {
  }

  static anyObj() {
  }

  static anyFunction() {
  }

  static notString() {
  }

  static notBoolean() {
  }

  static notNumber() {
  }

  static notObj() {
  }

  static notFunction() {
  }

  static matchRegexs(value: RegExp): MatcherInfo {
    if (ArgumentMatchers.isRegExp(value)) {
      return new MatcherInfo(ArgumentMatchers.MATCH_REGEXS, value, true);
    }
    throw Error('not a regex');
  }

  static notMatchRegexs(value: RegExp): MatcherInfo {
    if (ArgumentMatchers.isRegExp(value)) {
      return new MatcherInfo(ArgumentMatchers.MATCH_REGEXS, value, false);
    }
    throw Error('not a regex');
  }

  static containSubstring(subString: string): MatcherInfo {
    if (typeof subString === "string") {
      return new MatcherInfo(ArgumentMatchers.CONTAIN_SUBSTRING, subString, true);
    }
    throw Error('not a string');
  }

  static notContainSubstring(subString: string): MatcherInfo {
    if (typeof subString === "string") {
      return new MatcherInfo(ArgumentMatchers.CONTAIN_SUBSTRING, subString, false);
    }
    throw Error('not a string');
  }

  static containElement(element: Object): MatcherInfo {
    return new MatcherInfo(ArgumentMatchers.CONTAIN_ELEMENT, element, true);
  }

  static notContainElement(element: Object): MatcherInfo {
    return new MatcherInfo(ArgumentMatchers.CONTAIN_ELEMENT, element, false);
  }

  static containSubArray(subArray: Array<Object>): MatcherInfo {
    if (Array.isArray(subArray)) {
      return new MatcherInfo(ArgumentMatchers.CONTAIN_SUBARRAY, subArray, true);
    }
    throw Error('not an Array');
  }

  static notContainSubArray(subArray: Array<Object>): MatcherInfo {
    if (Array.isArray(subArray)) {
      return new MatcherInfo(ArgumentMatchers.CONTAIN_SUBARRAY, subArray, false);
    }
    throw Error('not an Array');
  }

  static isRegExp<T>(value: T) : boolean {
    let ret  = DeepTypeUtils.getType(value) === 'RegExp';
    hilog.info(domain, tag, 'isRegExp %{public}d ', ret);
    return ret;
  }

  static matcheReturnKey(arg: Object, stubSetKey: Object) {
    if (stubSetKey && stubSetKey === ArgumentMatchers.any && arg) {
      return true;
    } else if (stubSetKey === ArgumentMatchers.anyString && typeof arg === 'string') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.anyBoolean && typeof arg === 'boolean') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.anyNumber && typeof arg === 'number') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.anyObj && typeof arg === 'object') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.anyFunction && typeof arg === 'function') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.notString && typeof arg !== 'string') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.notBoolean && typeof arg !== 'boolean') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.notNumber && typeof arg !== 'number') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.notObj && typeof arg !== 'object') {
      return true;
    } else if (stubSetKey === ArgumentMatchers.notFunction && typeof arg !== 'function') {
      return true;
    } else if (stubSetKey instanceof MatcherInfo && (stubSetKey as MatcherInfo).type_ === ArgumentMatchers.MATCH_REGEXS && typeof arg === 'string') {
      let needMatch = (stubSetKey as MatcherInfo).needMatch !== false;
      let regexValue = (stubSetKey as MatcherInfo).value as RegExp;
      return regexValue.test(arg as string) === needMatch;
    } else if (stubSetKey instanceof MatcherInfo && (stubSetKey as MatcherInfo).type_ === ArgumentMatchers.CONTAIN_SUBSTRING && typeof arg === 'string') {
      console.info("zzzz CONTAIN_SUBSTRING")
      let needMatch = (stubSetKey as MatcherInfo).needMatch !== false;
      let isContained = (arg as string).includes ((stubSetKey as MatcherInfo).value as string);
      return isContained === needMatch
    } else if (stubSetKey instanceof MatcherInfo && (stubSetKey as MatcherInfo).type_ === ArgumentMatchers.CONTAIN_SUBARRAY && Array.isArray(arg)) {
      console.info("zzzz CONTAIN_SUBARRAY")
      let needMatch = (stubSetKey as MatcherInfo).needMatch !== false;
      let isContained = false;
      let containedValue: Array<Object> = (stubSetKey as MatcherInfo).value as Object[]
      let value: Array<Object> = arg as Object[]
      console.info("zzzzzzz type1 " + typeof value[0] + " " + "type2 " + typeof containedValue[0]) // number 跟int
      isContained = containedValue.every((item: Object) => {
        for (let index = 0; index < value.length; index++) {
          if (value[index] === item) {
            return true;
          }
        }
        return false;
      })
      console.info("zzzzzzz isContained" + JSON.stringify(value) + " " +isContained + " " + JSON.stringify(containedValue))
      return isContained === needMatch
    } else if (stubSetKey instanceof MatcherInfo && (stubSetKey as MatcherInfo).type_ === ArgumentMatchers.CONTAIN_ELEMENT && Array.isArray(arg)) {
      console.info("zzzz CONTAIN_ELEMENT")
      let needMatch = (stubSetKey as MatcherInfo).needMatch !== false;
      let isContained = false;
      let arr = arg as Object[]
      let value = (stubSetKey as MatcherInfo).value as Object;
      for (let index = 0; index < arr.length; index++) {
        if (arr[index] === value) {
          isContained = true;
          break;
        }
      }
      console.info("zzzzzzz isContained" + JSON.stringify(arg) + " " +isContained + " " + (stubSetKey as MatcherInfo).value)
      return isContained === needMatch
    }
    return false;
  }
}
