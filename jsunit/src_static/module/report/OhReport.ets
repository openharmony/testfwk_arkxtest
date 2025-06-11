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
import { SysTestKit } from '../kit/SysTestKit';

import { TAG, PrintTag } from '../../Constant';
import { Core } from '../../core';
import { SuiteService } from '../service/SuiteService';
import { SpecService } from '../service/SpecService';
import { StaticSpec } from '../service/StaticSpec';
import { ReportAttrIF, TestcaseSummaryIF } from '../../interface';
import { ConfigService } from '../config/configService';

class OhReport {
  public delegator: abilityDelegatorRegistry.AbilityDelegator;
  public abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs;
  public id: string;
  public index: int;
  public duration: long;
  public currentThreadName: string;
  public coreContext: Core | null;
  public suiteService: SuiteService | null;
  public specService: SpecService | null;
  private taskDoneTime: number;
  constructor(attr: ReportAttrIF) {
    this.delegator = attr.delegator;
    this.abilityDelegatorArguments = attr.abilityDelegatorArguments;
    this.id = 'report';
    this.index = 0;
    this.duration = 0;
    this.currentThreadName = 'mainThread';
    this.coreContext = null;
    this.suiteService = null;
    this.specService = null;
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
    const suite = coreContext.getDefaultService('suite');
    if (suite !== null) {
      this.suiteService = suite as SuiteService;
    }
    const spec = coreContext.getDefaultService('spec');
    if (spec !== null) {
      this.specService = spec as SpecService;
    }
  }

  taskStart() {}

  async taskDone() {
    const suite = this.suiteService;
    if (suite) {
      const summary = suite.getSummary();
      if (this.abilityDelegatorArguments !== null && summary !== null) {
        this.taskDoneTime = new Date().getTime();
        let message =
          '\n' + `${PrintTag.OHOS_REPORT_RESULT}: stream=Tests run: ` + summary.total + ', Failure: ' + summary.failure;
        message += ', Error: ' + summary.error;
        message += ', Pass: ' + summary.pass;
        message += ', Ignore: ' + summary.ignore;
        const core = this.coreContext;
        if (core) {
          const specService = core.getDefaultService('spec');
          if (specService !== null) {
            const spec = specService as SpecService;
            const num = spec.skipSpecNum;
            if (Number(num) > 0) {
              message += ', SkipSpec: ' + num;
            }
          }
        }

        message += '\n' + `${PrintTag.OHOS_REPORT_CODE}: ` + (summary.failure > 0 ? -1 : 0) + '\n';
        let isHasError = summary.failure > 0 || summary.error > 0;
        if (core) {
          const config = core.getDefaultService('config');
          if (config !== null) {
            const cf = config as ConfigService;
            if (cf.isBreakOnError() && isHasError) {
              message +=
                '\n' +
                `${PrintTag.OHOS_REPORT_RESULT}: breakOnError model, Stopping whole test suite if one specific test case failed or error` +
                '\n';
            }
          }
        }

        message += `${PrintTag.OHOS_REPORT_STATUS}: taskconsuming=` + summary.duration + '\n';
        console.info(`${message}`);
        await SysTestKit.print(message);
      }
      console.info(`${TAG}report print success`);
      this.delegator.finishTest('your test finished!!!', 0);
    }
  }

  incorrectFormat() {
    const core = this.coreContext;
    if (core) {
      const config = core.getDefaultService('config');
      if (config !== null) {
        const configService = config as ConfigService;
        if (configService.filterValid.length !== 0) {
          const value = configService.filterValid;
          const message = 'this param ' + value.join(',') + ' is invalid' + '\n';
          this.delegator.finishTest(message, 0);
        }
      }
    }
  }

  incorrectTestSuiteFormat() {
    const core = this.coreContext;
    if (core) {
      const config = core.getDefaultService('config');
      if (config !== null) {
        const configService = config as ConfigService;
        if (configService.filterXdescribe.length !== 0) {
          let value = configService.filterXdescribe;
          let message = 'xdescribe ' + value.join(',') + ' should not contain it' + '\n';
          this.delegator.finishTest(message, 0);
        }
      }
    }
  }

  async suiteStart() {
    if (this.abilityDelegatorArguments !== null) {
      const suite = this.suiteService;
      if (suite) {
        let specArr = new Array<StaticSpec>();
        suite.getAllChildSuiteNum(suite.getCurrentRunningSuite(), specArr);
        let message = '\n' + `${PrintTag.OHOS_REPORT_SUM}: ` + specArr.length;
        suite.setCurrentRunningSuiteDesc(suite.getRootSuite(), suite.getCurrentRunningSuite(), '');
        message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: class=` + suite.getCurrentRunningSuiteDesc() + '\n';
        if (suite.currentRunningSuite.isSkip) {
          message += `${PrintTag.OHOS_REPORT_STATUS}: skipReason=` + suite.currentRunningSuite.skipReason + '\n';
        }
        console.info(`${message}`);
        await SysTestKit.print(message);
        console.info(`${TAG}${suite.getCurrentRunningSuite().description} suiteStart print success`);
      }
    }
  }

  async suiteDone() {
    if (this.abilityDelegatorArguments !== null) {
      const suite = this.suiteService;
      if (suite) {
        const currentRunningSuite = suite.getCurrentRunningSuite();
        suite.setCurrentRunningSuiteDesc(suite.getRootSuite(), currentRunningSuite, '');
        let message = '\n' + `${PrintTag.OHOS_REPORT_STATUS}: class=` + suite.getCurrentRunningSuiteDesc();
        if (suite.currentRunningSuite.isSkip && suite.currentRunningSuite.skipReason !== '') {
          message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: skipReason=` + suite.currentRunningSuite.skipReason;
        }
        const isPromiseError = currentRunningSuite.isPromiseError;
        if (isPromiseError) {
          message +=
            '\n' + `${PrintTag.OHOS_REPORT_STATUS}: shortMsg=Promise(async, await) in describe is not allowed!`;
        }
        message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: suiteconsuming=` + currentRunningSuite.duration;
        const hookError = currentRunningSuite.hookError;
        if (hookError) {
          message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: ${hookError.message}`;
        }
        message += '\n';
        console.info(`${message}`);
        await SysTestKit.print(message);
        console.info(`${TAG}${currentRunningSuite.description} suiteDone print success`);
      }
    }
  }

  async specStart() {
    if (this.abilityDelegatorArguments !== null) {
      const specService = this.specService;
      const suiteService = this.suiteService;
      if (specService) {
        const curSpec = specService.currentRunningSpec;
        if (curSpec) {
          if (suiteService) {
            const staticSpec = curSpec as StaticSpec;
            let message = '\n' + `${PrintTag.OHOS_REPORT_STATUS}: class=` + suiteService.getCurrentRunningSuiteDesc();
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: current=` + ++this.index;
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: id=JS`;
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: numtests=` + specService.getTestTotal();
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: stream=`;
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: test=` + staticSpec.description;
            message += '\n' + `${PrintTag.OHOS_REPORT_STATUS_CODE}: 1` + '\n';
            if (staticSpec.isSkip) {
              message += `${PrintTag.OHOS_REPORT_STATUS}: skipReason=` + staticSpec.skipReason + '\n';
            }
            console.info(`${message}`);
            await SysTestKit.print(message);
            console.info(`${TAG}${staticSpec.description} specStart start print success`);
          }
        }
      }
    }
  }

  async specDone() {
    if (this.abilityDelegatorArguments !== null) {
      const specService = this.specService;
      const suiteService = this.suiteService;
      if (specService) {
        if (suiteService) {
          let message = '\n' + `${PrintTag.OHOS_REPORT_STATUS}: class=` + suiteService.getCurrentRunningSuiteDesc();
          message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: current=` + this.index;
          message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: id=JS`;
          message += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: numtests=` + specService.getTestTotal();
          let messageStack = '';
          let messageCode = '';
          const currentRunningSpec = specService.currentRunningSpec;
          if (currentRunningSpec) {
            const error = currentRunningSpec.error;
            const fail = currentRunningSpec.fail;
            if (error) {
              let errorStack = '';
              const stack = error.stack;
              if (stack) {
                errorStack = stack.slice(0, -1);
              }
              let errorMsg = '';
              const message = error.message;
              if (message) {
                errorMsg = message;
              }
              messageStack = `${PrintTag.OHOS_REPORT_STATUS}: stack=` + errorStack;
              messageCode += `${PrintTag.OHOS_REPORT_STATUS}: stream=`;
              messageCode +=
                currentRunningSpec.expectMsg !== ''
                  ? `message: ${currentRunningSpec.expectMsg}, Error in ${currentRunningSpec.description}, ${errorMsg}`
                  : `Error in ${currentRunningSpec.description}, ${errorMsg}`;
              messageCode += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: test=` + currentRunningSpec.description;
              messageCode += '\n' + `${PrintTag.OHOS_REPORT_STATUS_CODE}: -1` + '\n';
            } else if (fail) {
              let failStack = '';
              const fStack = fail.stack;
              if (fStack) {
                failStack = fStack.slice(0, -1);
              }
              let failMsg = '';
              if (fail.message) {
                failMsg = fail.message;
              }
              messageStack += `${PrintTag.OHOS_REPORT_STATUS}: stack=` + failStack;
              messageCode += `${PrintTag.OHOS_REPORT_STATUS}: stream=`;
              messageCode +=
                currentRunningSpec.expectMsg !== ''
                  ? `message: ${currentRunningSpec.expectMsg}, Error in ${currentRunningSpec.description}, ${failMsg}`
                  : `Error in ${currentRunningSpec.description}, ${failMsg}`;
              messageCode += '\n' + `${PrintTag.OHOS_REPORT_STATUS}: test=` + currentRunningSpec.description;
              messageCode += '\n' + `${PrintTag.OHOS_REPORT_STATUS_CODE}: -2` + '\n';
            } else {
              messageStack += `${PrintTag.OHOS_REPORT_STATUS}: stream=`;
              messageCode += `${PrintTag.OHOS_REPORT_STATUS}: test=` + currentRunningSpec.description;
              messageCode += '\n' + `${PrintTag.OHOS_REPORT_STATUS_CODE}: 0` + '\n';
              messageCode += currentRunningSpec.isSkip
                ? `${PrintTag.OHOS_REPORT_STATUS}: skipReason=` + currentRunningSpec.skipReason + '\n'
                : '';
            }
            messageCode += `${PrintTag.OHOS_REPORT_STATUS}: consuming=` + currentRunningSpec.duration + '\n';
          } else {
            messageCode += '\n';
          }
          await SysTestKit.print(message);
          await SysTestKit.print(messageStack);
          await SysTestKit.print(messageCode);
          if (currentRunningSpec) {
            console.info(`${TAG}${currentRunningSpec.description} specDone end print success`);
          }
        }
      }
    }
  }
}

export { OhReport };
