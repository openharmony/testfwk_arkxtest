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

import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import { worker, MessageEvents, ErrorEvent } from '@kit.ArkTS';

import { Core } from './core';
import { TAG, PrintTag } from './Constant';
import DataDriver from './module/config/DataDriver';
import { ExpectExtend } from './module/assert/ExpectExtend';
import { OhReport } from './module/report/OhReport';
import { SysTestKit } from './module/kit/SysTestKit';
import { TestcaseSummaryIF, ItItemIF, DataDriverData } from './interface';
import { AnyType } from './module/types/common';
import { ConfigService } from './module/config/configService';

export class Hypium {
  public static context = new Map<string, AnyType>();
  static setData(data: DataDriverData) {
    const core = Core.getInstance();
    const dataDriver = new DataDriver({ data });
    if (core) {
      core.addService('dataDriver', dataDriver);
    } else {
      throw Error('core is not created');
    }
  }
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
  static async hypiumInitWorkers(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    scriptURL: string,
    workerNum: int = 8,
    params: object
  ) {
    console.info(`${TAG}, hypiumInitWorkers call,${scriptURL}`);
    let workerPromiseArray = new Array<Promise<string>>();

    // 开始统计时间
    let startTime = await SysTestKit.getRealTime();
    for (let i = 0; i < workerNum; i++) {
      // 创建worker线程
      const workerPromise = Hypium.createWorkerPromise(scriptURL, i, params);
      workerPromiseArray.push(workerPromise);
    }
    const ret = { 
      total: 0, 
      failure: 0, 
      error: 0, 
      pass: 0, 
      ignore: 0, 
      duration: 0 
    };
    Promise.all(workerPromiseArray)
      .then(async (items) => {
        console.info(`${TAG}, all result from workers, ${JSON.stringify(items)}`);
        let allItemList = new Array<ItItemIF>();
        // 统计执行结果
        Hypium.handleWorkerTestResult(ret, allItemList, items);
        console.info(`${TAG}, all it result, ${JSON.stringify(allItemList)}`);
        // 统计用例执行结果
        const retResult = {
          total: 0,
          failure: 0,
          error: 0,
          pass: 0,
          ignore: 0,
          duration: 0,
        };
        // 标记用例执行结果
        Hypium.configWorkerItTestResult(retResult, allItemList);
        // 打印用例结果
        Hypium.printWorkerTestResult(abilityDelegator, allItemList);
        // 用例执行完成统计时间
        let endTime = await SysTestKit.getRealTime();
        const taskConsuming = endTime - startTime;
        const message =
          `\n${PrintTag.OHOS_REPORT_ALL_RESULT}: stream=Test run: runTimes: ${ret.total},total: ${retResult.total}, Failure: ${retResult.failure}, Error: ${retResult.error}, Pass: ${retResult.pass}, Ignore: ${retResult.ignore}` +
          `\n${PrintTag.OHOS_REPORT_ALL_CODE}: ${retResult.failure > 0 || retResult.error > 0 ? -1 : 0}` +
          `\n${PrintTag.OHOS_REPORT_ALL_STATUS}: taskconsuming=${taskConsuming > 0 ? taskConsuming : ret.duration}`;
        abilityDelegator.printSync(message);
        console.info(`${TAG}, [end] you worker test`);
        abilityDelegator.finishTest('you worker test finished!!!', 0, () => {});
      })
      .catch((e) => {
        console.info(`${TAG}, [end] error you worker test, ${JSON.stringify(e)}`);
        abilityDelegator.finishTest('you worker test error finished!!!', 0, () => {});
      })
      .finally(() => {
        console.info(`${TAG}, all promise finally end`);
      });
  }
  // 创建worker线程
  static createWorkerPromise(scriptURL: string, i: int, params: object) {
    console.info(`${TAG}, createWorkerPromiser, ${scriptURL}, ${i}`);
    const workerPromise = new Promise<string>((resolve: (value: string) => void, reject: (value: object) => void) => {
      const workerInstance = new worker.ThreadWorker(scriptURL, { name: `worker_${i}` });
      console.info(`${TAG}, send data to worker`);
      // 发送数据到worker线程中
      workerInstance.postMessage(params);
      workerInstance.onmessage = (e: MessageEvents) => {
        const data = e.data;
        if (data) {
          const currentThreadName = data.currentThreadName;
          console.info(`${TAG}, receview data from ${currentThreadName}, ${JSON.stringify(data)}`);

          resolve(data.summary);
          console.info(`${TAG}, ${currentThreadName} finish`);
          workerInstance.terminate();
        }
      };
      workerInstance.onerror = (e: ErrorEvent) => {
        console.info(`${TAG}, worker error, ${JSON.stringify(e)}`);
        reject(e);
        workerInstance.terminate();
      };
      workerInstance.onmessageerror = (e: MessageEvents) => {
        console.info(`${TAG}, worker message error, ${JSON.stringify(e)}`);
        reject(e);
        workerInstance.terminate();
      };
    });
    return workerPromise;
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

  static hypiumWorkerTest(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs,
    testsuite: () => void,
    workerPort: worker.ThreadWorker
  ) {
    console.info(`${TAG}, hypiumWorkerTest call`);
    SysTestKit.workerPort = workerPort;
    let currentWorkerName = workerPort.name;
    console.info(`${TAG}, hypiumWorkerTest_currentWorkerName: ${currentWorkerName}`);
    Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite);
  }
}
