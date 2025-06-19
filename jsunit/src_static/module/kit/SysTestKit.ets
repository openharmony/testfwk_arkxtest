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
import systemDateTime from '@ohos.systemDateTime';
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import { BusinessError } from '@ohos.base';
import { TAG } from '../../Constant';
import { Core } from '../../core';
import { SuiteService } from '../service/SuiteService';
import { SpecService } from '../service/SpecService';

class SysTestKit {
  public id: string;
  public index: short;
  public static delegator: abilityDelegatorRegistry.AbilityDelegator | null = null;
  public static systemTime: number;
  // public static workerPort: worker.ThreadWorker;
  constructor() {
    this.id = 'sysTestKit';
    this.index = 0;
  }

  static getDescribeName(): string {
    if (Core.getInstance()) {
      const core = Core.getInstance() as Core; //Core.getInstance()可能返回一个uniontype
      const suiteService = core.getDefaultService('suite');
      if (suiteService !== null) {
        if (suiteService instanceof SuiteService) {
          const deaultSuite = suiteService as SuiteService;
          if (deaultSuite.getCurrentRunningSuite()) {
            return deaultSuite.getCurrentRunningSuite().description;
          }
        }
      }
    }
    return '';
  }

  static getItName(): string {
    if (Core.getInstance()) {
      const core = Core.getInstance() as Core; //Core.getInstance()可能返回一个uniontype
      const specService = core.getDefaultService('spec');
      if (specService !== null) {
        if (specService instanceof SpecService) {
          const deaultSpec = specService as SpecService;
          const curSpec = deaultSpec.getCurrentRunningSpec();
          if (curSpec) {
            return curSpec.description;
          }
        }
      }
    }
    return '';
  }

  static getItAttribute(): long {
    if (Core.getInstance()) {
      const core = Core.getInstance() as Core; //Core.getInstance()可能返回一个uniontype
      const specService = core.getDefaultService('spec');
      if (specService !== null) {
        if (specService instanceof SpecService) {
          const deaultSpec = specService as SpecService;
          const curSpec = deaultSpec.getCurrentRunningSpec();
          if (curSpec) {
            return curSpec.fi;
          }
        }
      }
    }
    return 0;
  }

  static actionStart(tag: string): void {
    console.info(`${TAG}${JSON.stringify(tag)}`);
    const message = '\n' + 'OHOS_REPORT_ACTIONSTART: ' + JSON.stringify(tag) + '\n';
    SysTestKit.print(message);
    console.info(`${TAG}${JSON.stringify(tag)} actionStart print success`);
  }

  static actionEnd(tag: string): void {
    console.info(`${TAG}${JSON.stringify(tag)}`);
    const message = '\n' + 'OHOS_REPORT_ACTIONEND: ' + JSON.stringify(tag) + '\n';
    SysTestKit.print(message);
    console.info(`${TAG}${JSON.stringify(tag)}  actionEnd print success`);
  }
  static async print(message: string) {
    const delegator = SysTestKit.delegator;
    if (delegator === null) {
      console.info(`delegator is null`);
    } else {
      delegator.printSync(message);
    }
  }

  static getRealTime(): number {
    let current = new Date().getTime();
    try {
      current = systemDateTime.getUptime(systemDateTime.TimeType.STARTUP, false);
      console.info(`${TAG} systemDateTime.getUptime success`);
    } catch (err: Error) {
      console.error(`${TAG} failed to systemDateTime.getUptime because ` + err.message);
    }
    return 0;
  }

  static async existKeyword(keyword: string, timeout?: number): Promise<boolean> {
    let reg = new RegExp('^[a-zA-Z0-9]{1,}$');
    if (!reg.test(keyword)) {
      throw new Error('keyword must contain more than one string, and only letters and numbers are supported.');
    }
    let time = 4.0;
    if (timeout !== undefined) {
      time = timeout;
    }

    let searchResult = false;
    let cmd = "hilog -x | grep -i '" + keyword + "' | wc -l";
    try {
      const data = await executePromise(cmd, time);
      searchResult = data;
    } catch (err: Error) {
      throw err;
    }
    return searchResult;
  }
}

function executePromise(cmd: string, timeout: number) {
  return new Promise<boolean>((resolve: (value: boolean) => void, reject: (value: Error) => void) => {
    const delegator = SysTestKit.delegator;
    if (!delegator) {
      reject(new Error('delegator is null'));
    }
    (delegator as abilityDelegatorRegistry.AbilityDelegator).executeShellCommand(
      cmd,
      timeout,
      (error: BusinessError | null, data: abilityDelegatorRegistry.ShellCmdResult | undefined) => {
        if (error) {
          reject(new Error('executeShellCommand Error: ' + error.message));
          console.info(`${TAG}existKeyword CallBack: err : ${error.message}`);
        } else if(data) {
          console.info(`${TAG}existKeyword CallBack: data : ${JSON.stringify(data)}`);
          resolve(parseInt(data.stdResult) > 3 ? true : false);
        }
      }
    );
  });
}

export { SysTestKit };
