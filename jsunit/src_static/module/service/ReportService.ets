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

import { ServiceAttrIF } from '../../interface';
import { TAG } from '../../Constant';
import { Core } from '../../core';
import { SpecService } from './SpecService';
import { SuiteService } from './SuiteService';
import { SysTestKit } from '../kit/SysTestKit';
import { ConfigService } from '../config/configService';
class ReportService {
  public id: string;
  public coreContext: Core;
  public specService: SpecService;
  public suiteService: SuiteService;
  public duration: int;
  private index: byte;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.coreContext = new Core();
    this.specService = new SpecService({ id: '' });
    this.suiteService = new SuiteService({ id: '' });
    this.duration = 0;
    this.index = 0;
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
    this.duration = 0;
  }

  taskStart() {
    console.info(`${TAG}[start] start run suites`);
  }

  suiteStart() {
    console.info(`${TAG}[suite start]${this.suiteService.getCurrentRunningSuite().description}`);
  }

  specStart() {
    const spec = this.specService.currentRunningSpec;
    if (spec) {
      console.info(`${TAG}start running case '${spec.description}'`);
      this.index = (this.index + 1) as byte;
      spec.startTime = SysTestKit.getRealTime();
    }
  }

  specDone() {
    const spec = this.specService.currentRunningSpec;
    const suite = this.suiteService.currentRunningSuite;
    if (spec) {
      spec.duration = SysTestKit.getRealTime() - spec.startTime;
      suite.duration += spec.duration;
      const err = spec.error;
      const fail = spec.fail;
      if (err) {
        this.formatPrint('error', spec.description + ' ; consuming ' + spec.duration + 'ms');
        this.formatPrint('errorDetail', err.message);
      } else if (fail) {
        this.formatPrint('fail', spec.description + ' ; consuming ' + spec.duration + 'ms');
        this.formatPrint('failDetail', fail.message);
      } else {
        this.formatPrint('pass', spec.description + ' ; consuming ' + spec.duration + 'ms');
      }
    }
  }

  suiteDone() {
    let suite = this.suiteService.currentRunningSuite;
    let message = suite.hookError ? `, ${suite.hookError?.message}` : '';
    console.info(`[suite end] ${suite.description} consuming ${suite.duration} ms${message}`);
  }

  taskDone() {
    let msg = '';
    let summary = this.suiteService.getSummary();
    msg = 'total cases:' + summary.total + ';failure ' + summary.failure + ',' + 'error ' + summary.error;
    msg += ',pass ' + summary.pass + '; consuming ' + summary.duration + 'ms';
    console.info(`${TAG}${msg}`);
    console.info(`${TAG}[end] run suites end`);
  }

  incorrectFormat() {
    const config = this.coreContext.getDefaultService('config');
    if (config !== null) {
      const configService = config as ConfigService;
      if (configService.filterValid.length !== 0) {
        configService.filterValid.forEach((item: string) => {
          console.info(`${TAG}this param ${item} is invalid`);
        });
      }
    }
  }

  incorrectTestSuiteFormat() {
    const config = this.coreContext.getDefaultService('config');
    if (config !== null) {
      const configService = config as ConfigService;
      if (configService.filterXdescribe.length !== 0) {
        configService.filterXdescribe.forEach((item: string) => {
          console.info(`${TAG}xdescribe: ${item} should not contain it`);
        });
      }
    }
  }

  formatPrint(type: string, msg: string) {
    switch (type) {
      case 'pass':
        console.info(`${TAG}[pass]${msg}`);
        break;
      case 'fail':
        console.info(`${TAG}[fail]${msg}`);
        break;
      case 'failDetail':
        console.info(`${TAG}[failDetail]${msg}`);
        break;
      case 'error':
        console.info(`${TAG}[error]${msg}`);
        break;
      case 'errorDetail':
        console.info(`${TAG}[errorDetail]${msg}`);
        break;
    }
  }

  sleep(numberMillis: int) {
    const exitTime = new Date().getTime() + numberMillis;
    while (new Date().getTime() < exitTime) {}
    return;
  }
}

export { ReportService };
