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
    return current;
  }
}

export { SysTestKit };
