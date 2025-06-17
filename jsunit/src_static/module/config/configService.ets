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

import { Core } from '../../core';
import { ClassFilter, NotClassFilter, SuiteAndItNameFilter, TestTypesFilter, NestFilter } from './Filter';
import { TAG, TESTTYPE, LEVEL, SIZE, KEYSET } from '../../Constant';
import { ConfigIf, ServiceAttrIF, FilterParamIF } from '../../interface';
import { SuiteService } from '../service/SuiteService';
const STRESS_RULE = new RegExp('^[1-9]\\d*$');

class ConfigService {
  public id: string;
  public supportAsync: boolean;
  public random: boolean;
  public filterValid: Array<string>;
  public filter: int;
  public flag: boolean;
  public suite: string;
  public itName: string;
  public testType: string;
  public level: string;
  public size: string;
  public className: string;
  public notClass: string;
  public timeout: string;
  public breakOnError: boolean | string;
  public stress: string;
  public skipMessage: boolean;
  public runSkipped: string;
  public filterXdescribe: Array<string>;
  public dryRun: string;
  public coverage: string;
  public coreContext: Core;
  private filterParam: FilterParamIF | null;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.supportAsync = true; // 默认异步处理测试用例
    this.random = false;
    this.filterValid = new Array<string>();
    this.filter = 0;
    this.flag = false;
    this.suite = '';
    this.itName = '';
    this.testType = '';
    this.level = '';
    this.size = '';
    this.className = '';
    this.notClass = '';
    this.timeout = '';

    this.breakOnError = false;

    this.stress = '';
    this.dryRun = '';
    this.coverage = '';
    this.skipMessage = false;
    this.runSkipped = '';
    this.filterXdescribe = new Array<string>();
    this.filterParam = null;
    this.coreContext = new Core();
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
  }

  isNormalInteger(str: string) {
    const n = Math.floor(Number(str));
    return n !== Infinity && String(n) === String(str) && n >= 0;
  }
  getStress(): number {
    if (this.stress === undefined || this.stress === '' || this.stress === null) {
      return 1;
    }
    if (!STRESS_RULE.test(this.stress)) {
      return 1;
    } else {
      return Number.parseInt(this.stress);
    }
  }

  basicParamValidCheck(params: ConfigIf) {
    const size = params.size;
    if (size) {
      const sizeArray = new Array<string>('small', 'medium', 'large');
      if (sizeArray.indexOf(size) === -1) {
        this.filterValid.push('size:' + size);
      }
    }
    const level = params.level;
    if (level) {
      const levelArray = new Array<string>('0', '1', '2', '3', '4');
      if (levelArray.indexOf(level) === -1) {
        this.filterValid.push('level:' + level);
      }
    }
    const testType = params.testType;
    if (testType) {
      const testTypeArray = new Array<string>(
        'function',
        'performance',
        'power',
        'reliability',
        'security',
        'global',
        'compatibility',
        'user',
        'standard',
        'safety',
        'resilience'
      );
      if (testTypeArray.indexOf(testType) === -1) {
        this.filterValid.push('testType:' + testType);
      }
    }
  }

  filterParamValidCheck(params: ConfigIf) {
    let timeout = params.timeout;
    if (timeout) {
      if (!this.isNormalInteger(timeout)) {
        this.filterValid.push('timeout:' + timeout);
      }
    }

    if (params.dryRun) {
      if (params.dryRun !== 'true' && params.dryRun !== 'false') {
        this.filterValid.push(`dryRun:${params.dryRun}`);
      }
    }
    if (params.random) {
      if (params.random !== 'true' && params.random !== 'false') {
        this.filterValid.push(`random:${params.random}`);
      }
    }
    if (params.breakOnError) {
      if (params.breakOnError !== 'true' && params.breakOnError !== 'false') {
        this.filterValid.push(`breakOnError:${params.breakOnError}`);
      }
    }
    if (params.coverage) {
      if (params.coverage !== 'true' && params.coverage !== 'false') {
        this.filterValid.push(`coverage:${params.coverage}`);
      }
    }
    if (params.skipMessage) {
      if (params.skipMessage !== 'true' && params.skipMessage !== 'false') {
        this.filterValid.push(`skipMessage:${params.skipMessage}`);
      }
    }
    if (params.stress) {
      if (!STRESS_RULE.test(params.stress as string)) {
        this.filterValid.push('stress:' + params.stress);
      }
    }

    const nameRule = new RegExp('^[A-Za-z]{1}[\\w#,.]*$');
    if (params.className) {
      const classList = (params.className as string).split(',');
      const classArray = new Array<string>();
      for (let value of classList) {
        classArray.push(value);
      }
      classArray.forEach((item: string) => {
        if (!item.match(nameRule)) {
          this.filterValid.push(`className:${params.className}`);
        }
      });
    }
  }

  setConfig(params: ConfigIf) {
    this.basicParamValidCheck(params);
    this.filterParamValidCheck(params);
    try {
      if (params.className) {
        this.className = params.className as string;
      }
      if (params.notClass) {
        this.notClass = params.notClass as string;
      }
      if (params.flag !== undefined) {
        this.flag = params.flag as boolean;
      }
      if (params.suite) {
        this.suite = params.suite as string;
      }
      if (params.itName) {
        this.itName = params.itName as string;
      }
      if (params.filter !== undefined) {
        this.filter = params.filter.toInt();
      }
      if (params.testType) {
        this.testType = params.testType as string;
      }
      if (params.level) {
        this.level = params.level as string;
      }
      if (params.size) {
        this.size = params.size as string;
      }
      if (params.timeout) {
        this.timeout = params.timeout as string;
      }
      if (params.dryRun) {
        this.dryRun = params.dryRun as string;
      }
      if (params.breakOnError) {
        this.breakOnError = params.breakOnError as string;
      }
      if (params.stress) {
        this.stress = params.stress as string;
      }
      if (params.coverage) {
        this.coverage = params.coverage as string;
      }
      if (params.skipMessage) {
        this.skipMessage = Boolean(params.skipMessage);
      }
      if (params.runSkipped) {
        this.runSkipped = params.runSkipped as string;
      }
      this.random = params.random === 'true' ? true : false;
      this.filterParam = {
        testType: TESTTYPE,
        level: LEVEL,
        size: SIZE,
      } as FilterParamIF;
      this.parseParams();
    } catch (err: Error) {
      console.info(`${TAG}setConfig error: ${err.message}`);
    }
  }

  parseParams() {
    if (this.filter !== 0) {
      return;
    }
    let testTypeFilter = 0;
    let sizeFilter = 0;
    let levelFilter = 0;
    if (this.testType !== null) {
      const testTypeList = this.testType.split(',');
      const testTypeArr = new Array<string>();
      for (const testTypeV of testTypeList) {
        if (testTypeV !== undefined) {
          testTypeArr.push(testTypeV);
        }
      }
      const testTypeMapList = testTypeArr.map((item: string) => {
        if (this.filterParam) {
          const p = this.filterParam as FilterParamIF;
          if (p.testType) {
            const type = p.testType as Map<string, int>;
            const res = type.get(item);
            if (res) {
              return res;
            } else {
              return 0;
            }
          }
        }
        return 0;
      });
      const testTypeArray = new Array<int>();
      for (const testTypeV of testTypeMapList) {
        if (testTypeV !== undefined) {
          testTypeArray.push(testTypeV);
        }
      }
      testTypeFilter = testTypeArray.reduce((pre: int, cur: int) => pre | cur, 0);
    }
    if (this.level !== null) {
      const levelList = this.level.split(',');
      const levelArr = new Array<string>();
      for (const levelV of levelList) {
        levelArr.push(levelV);
      }
      const levelMapList = levelArr.map((item: string) => {
        if (item === '') {
          return 0;
        }
        if (this.filterParam) {
          const p = this.filterParam as FilterParamIF;
          if (p.level) {
            const level = p.level as int[];
            const res = level[Number(item).toInt()];
            if (res) {
              return res;
            } else {
              return 0;
            }
          }
        }
        return 0;
      });
      const levelArray = new Array<int>();
      for (const levelV of levelMapList) {
        levelArray.push(levelV);
      }
      levelFilter = levelArray.reduce((pre: int, cur: int) => pre | cur, 0);
    }
    if (this.size !== null) {
      const sizeList = this.size.split(',');
      const sizeArr = new Array<string>();
      for (const sizeV of sizeList) {
        sizeArr.push(sizeV);
      }
      const sizeMapList = sizeArr.map((item: string): int => {
        if (this.filterParam) {
          const p = this.filterParam as FilterParamIF;
          if (p.size) {
            const size = p.size as Map<string, int>;
            const res = size.get(item);
            if (res) {
              return res;
            } else {
              return 0;
            }
          }
        }
        return 0;
      });
      const sizeArray = new Array<int>();
      for (const sizeV of sizeMapList) {
        if (sizeV !== undefined) {
          sizeArray.push(sizeV);
        }
      }
      sizeFilter = sizeArray.reduce((pre: int, cur: int) => pre | cur, 0);
    }
    this.filter = testTypeFilter | sizeFilter | levelFilter;
    console.info(`${TAG}filter params:${this.filter}`);
  }

  isCurrentSuite(description: string) {
    if (this.suite !== undefined && this.suite !== '' && this.suite !== null) {
      const suiteList = this.suite.split(',');
      const suiteArray = new Array<string>();
      for (let suite of suiteList) {
        suiteArray.push(suite);
      }
      return suiteArray.indexOf(description) !== -1;
    }
    return false;
  }

  filterSuite(currentSuiteName: string): boolean {
    const filterArray1 = new Array<SuiteAndItNameFilter>();
    if (this.suite !== undefined && this.suite !== '' && this.suite !== null) {
      filterArray1.push(new SuiteAndItNameFilter(currentSuiteName, '', this.suite));
    }
    const mapArray1 = filterArray1.map((item: SuiteAndItNameFilter) => item.filterSuite());
    const reduce1 = mapArray1.reduce((pre: boolean, cur: boolean) => pre || cur, false);

    const filterArray2 = new Array<ClassFilter>();
    if (this.className !== undefined && this.className !== '' && this.className !== null) {
      filterArray2.push(new ClassFilter(currentSuiteName, '', this.className));
    }
    const mapArray2 = filterArray2.map((item: ClassFilter) => item.filterSuite());
    const reduce2 = mapArray2.reduce((pre: boolean, cur: boolean) => pre || cur, false);

    const filterArray3 = new Array<NotClassFilter>();
    if (this.notClass !== undefined && this.notClass !== '' && this.notClass !== null) {
      filterArray3.push(new NotClassFilter(currentSuiteName, '', this.notClass));
    }
    const mapArray3 = filterArray3.map((item: NotClassFilter) => item.filterSuite());
    const reduce3 = mapArray3.reduce((pre: boolean, cur: boolean) => pre || cur, false);
    return reduce1 || reduce2 || reduce3;
  }

  filterDesc(currentSuiteName: string, desc: string, fi: int): boolean {
    const suiteAndItNameFilterArray = new Array<SuiteAndItNameFilter>();
    const classFilterArray = new Array<ClassFilter>();
    const notClassFilterArray = new Array<NotClassFilter>();
    const testTypesFilterArray = new Array<TestTypesFilter>();
    if (this.itName !== undefined && this.itName !== '' && this.itName !== null) {
      suiteAndItNameFilterArray.push(new SuiteAndItNameFilter(currentSuiteName, desc, this.itName));
    }
    if (this.className !== undefined && this.className !== '' && this.className !== null) {
      classFilterArray.push(new ClassFilter(currentSuiteName, desc, this.className));
    }
    if (this.notClass !== undefined && this.notClass !== '' && this.notClass !== null) {
      notClassFilterArray.push(new NotClassFilter(currentSuiteName, desc, this.notClass));
    }
    if (typeof this.filter !== 'undefined' && this.filter !== 0 && fi !== 0) {
      testTypesFilterArray.push(new TestTypesFilter('', '', fi, this.filter));
    }
    const suiteAndItNameFilterResult = suiteAndItNameFilterArray
      .map((item: SuiteAndItNameFilter) => item.filterIt())
      .reduce((pre: boolean, cur: boolean) => pre || cur, false);
    const classFilterResult = classFilterArray
      .map((item: ClassFilter) => item.filterIt())
      .reduce((pre: boolean, cur: boolean) => pre || cur, false);
    const notClassFilterResult = notClassFilterArray
      .map((item: NotClassFilter) => item.filterIt())
      .reduce((pre: boolean, cur: boolean) => pre || cur, false);
    const testTypesFilterResult = testTypesFilterArray
      .map((item: TestTypesFilter) => item.filterIt())
      .reduce((pre: boolean, cur: boolean) => pre || cur, false);
    return suiteAndItNameFilterResult || classFilterResult || notClassFilterResult || testTypesFilterResult;
  }

  filterWithNest(desc: string, filter: int): boolean {
    let filterArray = new Array<TestTypesFilter>();
    const nestFilter = new NestFilter();
    const core = this.coreContext;
    if (core !== null) {
      const coreContext = core as Core;
      const suite = coreContext.getDefaultService('suite');
      if (suite !== null) {
        const defaultService = suite as SuiteService;
        const targetSuiteArray = defaultService.targetSuiteArray;
        const targetSpecArray = defaultService.targetSpecArray;
        const suiteStack = defaultService.suitesStack;
        let isFilter = nestFilter.filterNestName(targetSuiteArray, targetSpecArray, suiteStack, desc);
        const isFullRun = defaultService.fullRun;
        if (this.filter && filter) {
          filterArray.push(new TestTypesFilter('', '', filter, this.filter));
          return filterArray
            .map((item: TestTypesFilter) => item.filterIt())
            .reduce((pre: boolean, cur: boolean) => pre || cur, false);
        }
        if (isFilter && !isFullRun) {
          return true;
        }
        return nestFilter.filterNotClass(this.notClass, suiteStack, desc);
      }
    }

    return false;
  }

  isRandom() {
    return this.random || false;
  }

  isBreakOnError(): boolean {
    return this.breakOnError !== 'true' ? false : true;
  }

  setSupportAsync(value: boolean) {
    this.supportAsync = value;
  }

  isSupportAsync() {
    return this.supportAsync;
  }

  translateParams(parameters: Record<string, string>): ConfigIf {
    const keySet = new Set<string>(KEYSET);
    const targetParams: ConfigIf = {};
    for (const key of parameters.keys()) {
      if (keySet.has(key)) {
        const newKey = key.replace('-s ', '');
        if (newKey === 'class') {
          targetParams.className = parameters[key];
        } else if (newKey === 'notClass') {
          targetParams.notClass = parameters[key];
        } else if (newKey === 'suite') {
          targetParams.suite = parameters[key];
        } else if (newKey === 'itName') {
          targetParams.itName = parameters[key];
        } else if (newKey === 'level') {
          targetParams.level = parameters[key];
        } else if (newKey === 'testType') {
          targetParams.testType = parameters[key];
        } else if (newKey === 'size') {
          targetParams.size = parameters[key];
        } else if (newKey === 'timeout') {
          targetParams.timeout = parameters[key];
        } else if (newKey === 'dryRun') {
          targetParams.dryRun = parameters[key];
        } else if (newKey === 'random') {
          targetParams.random = parameters[key];
        } else if (newKey === 'breakOnError') {
          targetParams.breakOnError = parameters[key];
        } else if (newKey === 'stress') {
          targetParams.stress = parameters[key];
        } else if (newKey === 'coverage') {
          targetParams.coverage = parameters[key];
        } else if (newKey === 'skipMessage') {
          targetParams.skipMessage = parameters[key];
        } else if (newKey === 'runSkipped') {
          targetParams.runSkipped = parameters[key];
        }
      }
    }
    return targetParams;
  }
  translateParamsToString(parameters: Record<string, string>) {
    const keySet = new Set<string>(KEYSET);
    let targetParams = '';
    for (const key of Object.keys(parameters)) {
      if (keySet.has(key)) {
        targetParams += ' ' + key + ' ' + parameters[key];
      }
    }
    return targetParams.trim();
  }

  execute() {}

  checkIfSuiteInSkipRun(desc: string): boolean {
    const list = this.runSkipped.split(',');
    const arr = new Array<string>();
    for (let v of list) {
      arr.push(v);
    }
    return arr.some((item: string) => {
      return (
        item === desc ||
        item.startsWith(desc + '.') ||
        item.startsWith(desc + '#') ||
        desc.startsWith(item + '.') ||
        this.runSkipped === 'skipped'
      );
    });
  }

  checkIfSpecInSkipRun(desc: string): boolean {
    const list = this.runSkipped.split(',');
    const arr = new Array<string>();
    for (let v of list) {
      arr.push(v);
    }
    return arr.some((item: string) => {
      if (item.includes('#')) {
        return item === desc;
      } else {
        return desc.startsWith(item + '.') || desc.startsWith(item + '#') || this.runSkipped === 'skipped';
      }
    });
  }
}

export { ConfigService };
