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

import { LEVEL, SIZE, TESTTYPE } from '../../Constant';
import { StaticSuite } from '../service/StaticSuite';

class ClassFilter {
  public suiteName: string;
  public itName: string;
  public params: string;
  constructor(suiteName: string, itName: string, params: string) {
    this.suiteName = suiteName;
    this.itName = itName;
    this.params = params;
  }

  filterSuite(): boolean {
    const paramsArr = this.params.split(',');
    const paramsList = new Array<string>();
    for (const str of paramsArr) {
      paramsList.push(str);
    }
    const mapArr = paramsList.map((item: string) => item.split('#')[0]);
    const mapList = new Array<string>();
    for (const str of mapArr) {
      mapList.push(str);
    }
    const mmapArr = mapList.map((item: string) => item === this.suiteName);
    const mmapList = new Array<boolean>();
    for (const item of mmapArr) {
      mmapList.push(item);
    }
    return !mmapList.reduce((pre: boolean, cur: boolean) => pre || cur, false);
  }

  filterIt(): boolean {
    const classArray = this.params.split(',');
    const classList = new Array<string>();
    for (const str of classArray) {
      classList.push(str);
    }
    const classFilter = classList.filter((item: string) => !item.includes('#'));
    const classFilterList = new Array<string>();
    for (const str of classFilter) {
      classFilterList.push(str);
    }
    const classMap = classFilterList.map((item: string) => item === this.suiteName);
    const classMapList = new Array<boolean>();
    for (const str of classMap) {
      classMapList.push(str);
    }
    const suiteFilterResult = classMapList.reduce((pre: boolean, cur: boolean) => pre || cur, false);

    const filter = classList.filter((item: string) => item.includes('#'));
    const filterList = new Array<string>();
    for (const str of filter) {
      filterList.push(str);
    }
    const filterMap = filterList.map((item: string) => item == this.suiteName + '#' + this.itName);
    const filterMapList = new Array<boolean>();
    for (const str of filterMap) {
      filterMapList.push(str);
    }
    const itFilterResult = filterMapList.reduce((pre: boolean, cur: boolean) => pre || cur, false);
    return !(suiteFilterResult || itFilterResult);
  }
}

class NotClassFilter {
  public suiteName: string;
  public itName: string;
  public params: string;
  constructor(suiteName: string, itName: string, params: string) {
    this.suiteName = suiteName;
    this.itName = itName;
    this.params = params;
  }

  filterSuite(): boolean {
    const splitArr = this.params.split(',');
    const splitList = new Array<string>();
    for (const str of splitArr) {
      splitList.push(str);
    }
    const mapArr = splitList.map((item: string) => item === this.suiteName);
    const mapList = new Array<boolean>();
    for (const item of mapArr) {
      mapList.push(item);
    }
    return mapArr.reduce((pre: boolean, cur: boolean) => pre || cur, false);
  }

  filterIt(): boolean {
    const splitArr = this.params.split(',');
    const splitList = new Array<string>();
    for (const str of splitArr) {
      splitList.push(str);
    }
    return splitList.some((item: string) => item === this.suiteName + '#' + this.itName);
  }
}

class SuiteAndItNameFilter {
  public suiteName: string;
  public itName: string;
  public params: string;
  constructor(suiteName: string, itName: string, params: string) {
    this.suiteName = suiteName;
    this.itName = itName;
    this.params = params;
  }

  filterSuite(): boolean {
    const splitArr = this.params.split(',');
    const splitList = new Array<string>();
    for (const str of splitArr) {
      splitList.push(str);
    }
    const mapArr = splitList.map((item: string) => item === this.suiteName);
    const mapList = new Array<boolean>();
    for (const item of mapArr) {
      mapList.push(item);
    }
    return mapArr.reduce((pre: boolean, cur: boolean) => pre || cur, false);
  }

  filterIt(): boolean {
    const splitArr = this.params.split(',');
    const splitList = new Array<string>();
    for (const str of splitArr) {
      splitList.push(str);
    }
    const mapArr = splitList.map((item: string) => item === this.itName);
    const mapList = new Array<boolean>();
    for (const item of mapArr) {
      mapList.push(item);
    }
    return mapArr.reduce((pre: boolean, cur: boolean) => pre || cur, false);
  }
}

class TestTypesFilter {
  public suiteName: string;
  public itName: string;
  public params: long;
  public fi: long;
  constructor(suiteName: string, itName: string, fi: long, params: long) {
    this.suiteName = suiteName;
    this.itName = itName;
    this.params = params;
    this.fi = fi;
  }

  filterIt(): boolean {
    return !(this.params === (this.fi & this.params) || this.fi === 0);
  }
}

class NestFilter {
  filterNestName(
    targetSuiteArray: Array<string>,
    targetSpecArray: Array<string>,
    suiteStack: Array<StaticSuite>,
    desc: string
  ): boolean {
    let targetSuiteName = '';
    for (let value of suiteStack) {
      if (value) {
        targetSuiteName = targetSuiteName + '.' + value.description;
      }
    }
    targetSuiteName = targetSuiteName.substring(2);
    const targetSpecName = targetSuiteName + '#' + desc;
    if (targetSpecArray.includes(targetSpecName)) {
      return false;
    }
    for (const suiteName of targetSuiteArray) {
      if (targetSuiteName.startsWith(suiteName)) {
        return false;
      }
    }
    return true;
  }

  filterNotClass(notClass: string, suiteStack: Array<StaticSuite>, desc: string): boolean {
    let filterFlag = false;
    if (notClass != null) {
      const notClassList = notClass.split(',');
      const notClassArray = new Array<string>();
      for (let v of notClassList) {
        if (v !== undefined && v !== '') {
          notClassArray.push(v);
        }
      }
      let targetSuiteName = '';

      for (let stackValue of suiteStack) {
        targetSuiteName = targetSuiteName + '.' + stackValue.description;
      }
      targetSuiteName = targetSuiteName.substring(2);
      const targetSpecName = targetSuiteName + '#' + desc;
      if (
        notClassArray.length &&
        (notClassArray.includes(targetSpecName) || notClassArray.some((key: string) => targetSpecName.startsWith(key)))
      ) {
        filterFlag = true;
      }
    }
    return filterFlag;
  }

  filterLevelOrSizeOrTestType(level: int | null, size: string | null, testType: string | null, filter: int) {
    let result = false;
    if (filter === 0) {
      return false;
    }
    if (level === null && size === null && testType === null) {
      return false;
    }
    if (level != null && filter === LEVEL[level]) {
      result = true;
    }
    if (size != null && !result) {
      const ss = SIZE.get(size);
      if (ss !== undefined) {
        result = filter === ss;
      }
    }
    if (testType != null && !result) {
      const tt = TESTTYPE.get(testType);
      if (tt !== undefined) {
        result = filter === tt;
      }
    }
    return !result;
  }
}
export { ClassFilter, NotClassFilter, SuiteAndItNameFilter, TestTypesFilter, NestFilter };
