/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';

import { Core } from './src/core';
import { TAG, PrintTag } from './src/Constant';

import { ExpectExtend } from './src/module/assert/ExpectExtend';
import { OhReport } from './src/module/report/OhReport';
import { SysTestKit } from './src/module/kit/SysTestKit';

import { TestcaseSummaryIF, ItItemIF } from './src/interface';
import { AnyType } from './src/module/types/common';
import { ConfigService } from './src/module/config/configService';

export class Hypium {
  public static context = new Map<string, AnyType>();
  static setTimeConfig(systemTime: number) {
    SysTestKit.systemTime = systemTime;
  }

  static set(key: string, value: AnyType) {
    Hypium.context.set(key, value);
  }

  static get(key: string) {
    return Hypium.context.get(key);
  }

  static hypiumTest(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs,
    testsuite: () => void
  ) {
    const core = Core.getInstance();
    const expectExtend = new ExpectExtend({
      id: 'extend',
    });
    const ohReport = new OhReport({
      delegator: abilityDelegator,
      abilityDelegatorArguments: abilityDelegatorArguments,
    });
    SysTestKit.delegator = abilityDelegator;
    if (core) {
      core.addService('expect', expectExtend);
      core.addService('report', ohReport);
      core.init();
      core.subscribeEvent('spec', ohReport);
      core.subscribeEvent('suite', ohReport);
      core.subscribeEvent('task', ohReport);
      const cService = core.getDefaultService('config');
      if (cService !== null && abilityDelegatorArguments !== null) {
        const configService = cService as ConfigService;
        const testParameters = configService.translateParams(abilityDelegatorArguments.parameters);
        console.info(`${TAG}parameters:${JSON.stringify(testParameters)}`);
        configService.setConfig(testParameters);
      }
      testsuite();
      core.execute(abilityDelegator);
    }
  }
  static handleWorkerTestResult(ret: TestcaseSummaryIF, allItemList: Array<ItItemIF>, items: Array<TestcaseSummaryIF>) {
    console.info(`${TAG}, handleWorkerTestResult, ${JSON.stringify(items)}`);
    for (const item of items) {
      ret.total += item.total;
      ret.failure += item.failure;
      ret.error += item.error;
      ret.pass += item.pass;
      ret.ignore += item.ignore;
      ret.duration += item.duration;
      Hypium.handleItResult(allItemList, item.itItemList);
    }
  }
  static handleItResult(allItemList: Array<ItItemIF>, itItemList: Array<ItItemIF>) {
    for (const itItem of itItemList) {
      const description = itItem.description;
      const result = itItem.result;
      const currentThreadName = itItem.currentThreadName;
      const item = allItemList.find((it: ItItemIF) => it.description === description);
      if (item) {
        if (item.result === 0) {
          item.result = result;
          item.currentThreadName = currentThreadName;
        }
      } else {
        const it: ItItemIF = {
          description: description,
          currentThreadName: currentThreadName,
          result: result,
        };
        allItemList.push(it);
      }
    }
  }
  static configWorkerItTestResult(retResult: TestcaseSummaryIF, allItemList: Array<ItItemIF>) {
    console.info(`${TAG}, configWorkerItTestResult, ${JSON.stringify(allItemList)}`);
    for (const item of allItemList) {
      const description = item.description;
      const result = item.result;
      console.info(`${TAG}, description, ${description}, result,${result}`);
      retResult.total++;
      if (result === 0) {
        retResult.pass++;
      } else if (result === -1) {
        retResult.error++;
      } else if (result === -2) {
        retResult.failure++;
      } else {
        retResult.ignore++;
      }
    }
  }
  static printWorkerTestResult(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    allItemList: Array<ItItemIF>
  ) {
    console.info(`${TAG}, printWorkerTestResult, ${JSON.stringify(allItemList)}`);
    let index = 1;
    for (const itemValue of allItemList) {
      const description = itemValue.description;
      const result = itemValue.result;
      console.info(`${TAG}, description print, ${description}, result,${result}`);
      const itArray = description.split('#');
      let des = 'undefined';
      let itName = 'undefined';
      if (itArray.length > 1) {
        des = itArray[0];
        itName = itArray[1];
      } else if (itArray.length > 1) {
        des = itArray[0];
        itName = itArray[0];
      }

      let msg = `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: class=${des}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: test=${itName}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: current=${index}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: CODE=${result}`;
      abilityDelegator.printSync(msg);
      index++;
    }
  }
}

export { Assert } from './src/interface';
export {
  hypiumDescribe as describe,
  hypiumBeforeAll as beforeAll,
  hypiumBeforeEach as beforeEach,
  hypiumAfterEach as afterEach,
  hypiumAfterAll as afterAll,
  hypiumIt as it,
  hypiumExpect as expect,
  hypiumBeforeItSpecified as beforeItSpecified,
  hypiumAfterItSpecified as afterItSpecified,
  hypiumXdescribe as xdescribe,
  hypiumXit as xit,
} from './src/hypiumApi';
export { DEFAULT, TestType, Size, Level, TAG, PrintTag } from './src/Constant';
export { Core } from './src/core';
export { ExpectExtend } from './src/module/assert/ExpectExtend';
export { OhReport } from './src/module/report/OhReport';
export { SysTestKit } from './src/module/kit/SysTestKit';
