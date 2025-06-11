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

import { StaticSpec } from './StaticSpec';
import { StaticServiceIF } from '../../interface';
import { TAG } from '../../Constant';
import { Core } from '../../core';
import { AssertException } from './AssertException';
import { ConfigService } from '../config/configService';
import { SpecService } from './SpecService';
import { SuiteService } from './SuiteService';

let tempSuiteService: SuiteService = new SuiteService({ id: '' });
class StaticSuite {
  public description: string;
  public specs: Array<StaticSpec>;
  public childSuites: Array<StaticSuite>;
  public beforeItSpecified: Map<string | string[], (() => void) | (() => Promise<void>)>;
  public afterItSpecified: Map<string | string[], (() => void) | (() => Promise<void>)>;
  public duration: int;
  public beforeAll: Set<(() => void) | (() => Promise<void>)>;
  public afterAll: Set<(() => void) | (() => Promise<void>)>;
  public beforeEach: Set<(() => void) | (() => Promise<void>)>;
  public afterEach: Set<(() => void) | (() => Promise<void>)>;
  public hookError: Error | null;
  public isSkip: boolean;
  public skipReason: string;
  public isPromiseError: boolean;
  constructor(attrs: StaticServiceIF) {
    this.description = attrs.description;
    this.childSuites = new Array<StaticSuite>();
    this.specs = new Array<StaticSpec>();
    this.beforeAll = new Set<(() => void) | (() => Promise<void>)>();
    this.afterAll = new Set<(() => void) | (() => Promise<void>)>();
    this.beforeEach = new Set<(() => void) | (() => Promise<void>)>();
    this.afterEach = new Set<(() => void) | (() => Promise<void>)>();
    this.beforeItSpecified = new Map<string | string[], (() => void) | (() => Promise<void>)>();
    this.afterItSpecified = new Map<string | string[], (() => void) | (() => Promise<void>)>();
    this.duration = 0;
    this.hookError = null;
    this.isSkip = false;
    this.skipReason = '';
    this.isPromiseError = false;
  }

  pushSpec(spec: StaticSpec) {
    this.specs.push(spec);
  }

  removeSpec(desc: string) {
    this.specs = this.specs.filter((item: StaticSpec) => {
      return item.description !== desc;
    });
  }

  getSpecsNum() {
    return this.specs.length;
  }

  isRun(coreContext: Core): boolean {
    const configService = coreContext.getDefaultService('config');
    const specService = coreContext.getDefaultService('spec');
    if (configService !== null && specService !== null) {
      const cService = configService as ConfigService;
      const sService = specService as SpecService;
      const breakOnError: boolean = cService.isBreakOnError();
      const isError: boolean = sService.getStatus();
      return breakOnError && isError;
    }
    return false;
  }

  run(coreContext: Core) {
    const sService = coreContext.getDefaultService('suite');
    const cService = coreContext.getDefaultService('config');
    if (sService !== null && cService !== null) {
      const suiteService = sService as SuiteService;
      const configService = cService as ConfigService;
      suiteService.setCurrentRunningSuite(this);
      if (this.description !== '') {
        coreContext.fireEvents('suite', 'suiteStart');
      }
      if (this.beforeAll && this.beforeAll.size > 0) {
        this.beforeAll.forEach((func: (() => void) | (() => Promise<void>)) => {
          try {
            (func as () => void)();
          } catch (e: Error) {
            console.error(`${TAG}${e.stack}`);
          }
        });
      }
      const specs = this.specs;
      if (specs.length > 0) {
        if (configService.isRandom()) {
          specs.sort((a: StaticSpec, b: StaticSpec) => {
            return Math.random() > 0.5 ? -1 : 1;
          });
        }
        for (let i = 0; i < specs.length; i++) {
          const spec = specs[i];
          let isBreakOnError = this.isRun(coreContext);
          if (isBreakOnError) {
            break;
          }
          if (this.beforeEach && this.beforeEach.size > 0) {
            this.beforeEach.forEach((func: (() => void) | (() => Promise<void>)) => {
              try {
                (func as () => void)();
              } catch (e: Error) {
                console.error(`${TAG}${e.stack}`);
              }
            });
          }
          spec.run(coreContext);

          if (this.afterEach && this.afterEach.size > 0) {
            this.afterEach.forEach((func: (() => void) | (() => Promise<void>)) => {
              try {
                (func as () => void)();
              } catch (e: Error) {
                console.error(`${TAG}${e.stack}`);
              }
            });
          }
        }
      }
      const clength = this.childSuites.length;
      if (clength > 0) {
        for (let i = 0; i < clength; i++) {
          const suite = this.childSuites[i];
          let isBreakOnError = this.isRun(coreContext);
          if (isBreakOnError) {
            break;
          }
          suite.run(coreContext);
          suiteService.setCurrentRunningSuite(suite);
        }
      }

      if (this.afterAll && this.afterAll.size > 0) {
        this.afterAll.forEach((func: (() => void) | (() => Promise<void>)) => {
          try {
            (func as () => void)();
          } catch (e: Error) {
            console.error(`${TAG}${e.stack}`);
          }
        });
      }
      if (this.description !== '') {
        coreContext.fireEvents('suite', 'suiteDone');
      }
    }
  }

  runBeforeItSpecified(
    beforeItSpecified: Map<string | string[], (() => void) | (() => Promise<void>)>,
    specItem: StaticSpec
  ) {
    for (const itNames of beforeItSpecified.keys()) {
      let flag = false;
      if (typeof itNames === 'string' && (itNames as string) === specItem.description) {
        flag = true;
      } else if (Array.isArray(itNames)) {
        const itNamesArr = itNames as string[];
        for (const name of itNamesArr) {
          if (name === specItem.description) {
            flag = true;
            break;
          }
        }
      }
      if (flag) {
        const hookFunc = beforeItSpecified.get(itNames);
        if (hookFunc) {
          (hookFunc as () => Promise<void>)();
        }
        break;
      }
    }
  }

  runAfterItSpecified(
    afterItSpecified: Map<string | string[], (() => void) | (() => Promise<void>)>,
    specItem: StaticSpec
  ) {
    for (const itNames of afterItSpecified.keys()) {
      let flag = false;
      if (typeof itNames === 'string' && (itNames as string) === specItem.description) {
        flag = true;
      } else if (Array.isArray(itNames)) {
        const itNamesArr = itNames as string[];
        for (const name of itNamesArr) {
          if (name === specItem.description) {
            flag = true;
            break;
          }
        }
      }
      if (flag) {
        const hookFunc = afterItSpecified.get(itNames);
        if (hookFunc) {
          (hookFunc as () => Promise<void>)();
        }
        break;
      }
    }
  }

  async asyncRunSpecs(coreContext: Core) {
    const cService = coreContext.getDefaultService('config');
    const sService = coreContext.getDefaultService('spec');
    if (cService !== null && sService !== null) {
      const configService = cService as ConfigService;
      const specService = sService as SpecService;
      if (configService.isRandom()) {
        this.specs.sort((a, b) => {
          return Math.random() > 0.5 ? -1 : 1;
        });
      }
      for (let specItem of this.specs) {
        if (specItem) {
          specService.setCurrentRunningSpec(specItem);

          let isBreakOnError = this.isRun(coreContext);
          if (isBreakOnError) {
            console.info('break description :' + this.description);
            break;
          }
          await coreContext.fireEvents('spec', 'specStart');
          try {
            this.runBeforeItSpecified(this.beforeItSpecified, specItem);
            for (const hookItem of this.beforeEach) {
              if (hookItem) {
                try {
                  await (hookItem as () => Promise<void>)();
                } catch (error) {
                  if (error) {
                    const err = error as Error;
                    err.message += `, error in beforeEach function`;
                    throw err;
                  }
                }
              }
            }
            await specItem.asyncRun(coreContext);
            this.runAfterItSpecified(this.afterItSpecified, specItem);
            for (const hookItem of this.afterEach) {
              try {
                await (hookItem as () => Promise<void>)();
              } catch (error) {
                if (error) {
                  const err = error as Error;
                  err.message += `, error in afterEach function`;
                  throw err;
                }
              }
            }
          } catch (err) {
            if (err) {
              const e = err as Error;
              console.error(`${TAG}stack:${e.stack}`);
              console.error(`${TAG}stack end`);
              if (err instanceof AssertException) {
                specItem.fail = e;
              } else {
                specItem.error = e;
              }
            }
            specService.setStatus(true);
          }
          specItem.setResult();
          await coreContext.fireEvents('spec', 'specDone');
          specService.setCurrentRunningSpec(null);
        }
      }
    }
  }

  async asyncRunChildSuites(coreContext: Core) {
    for (let i = 0; i < this.childSuites.length; i++) {
      const isBreakOnError = this.isRun(coreContext);
      if (isBreakOnError) {
        console.info(`${TAG}break description : ${this.description}`);
        break;
      }
      await this.childSuites[i].asyncRun(coreContext);
    }
  }

  async asyncRun(coreContext: Core) {
    const sService = coreContext.getDefaultService('suite');
    const spService = coreContext.getDefaultService('spec');
    if (sService !== null && spService !== null) {
      const suiteService = sService as SuiteService;
      const specService = spService as SpecService;

      suiteService.setCurrentRunningSuite(this);
      suiteService.suitesStack.push(this);
      if (this.description !== '') {
        await coreContext.fireEvents('suite', 'suiteStart');
      }

      try {
        for (const hookItem of this.beforeAll) {
          try {
            await (hookItem as () => Promise<void>)();
          } catch (error) {
            if (error) {
              const err = error as Error;
              err.message += `, error in beforeAll function`;
              throw err;
            }
          }
        }
      } catch (error) {
        if (error) {
          const err = error as Error;
          console.error(`${TAG}${err.stack}`);
          this.hookError = err;
        }
      }

      if (this.hookError !== null) {
        const err = this.hookError as Error;
        specService.setStatus(true);
        tempSuiteService = suiteService;
        await tempSuiteService.setSuiteResults(this, err, coreContext);
      }
      if (this.specs.length > 0 && this.hookError === null) {
        await this.asyncRunSpecs(coreContext);
      }

      if (this.childSuites.length > 0 && this.hookError === null) {
        await this.asyncRunChildSuites(coreContext);
      }

      try {
        for (const hookItem of this.afterAll) {
          try {
            await (hookItem as () => Promise<void>)();
          } catch (error) {
            if (error) {
              const err = error as Error;
              err.message += `, error in afterAll function`;
              throw err;
            }
          }
        }
      } catch (error) {
        if (error) {
          console.error(`${TAG}${(error as Error).stack}`);
          this.hookError = error as Error;
          specService.setStatus(true);
        }
      }
      if (this.description !== '') {
        await coreContext.fireEvents('suite', 'suiteDone');
        const currentRunningSuite = suiteService.suitesStack.pop();
        if (currentRunningSuite) {
          const curSuite = currentRunningSuite as StaticSuite;
          suiteService.setCurrentRunningSuite(curSuite);
          suiteService.suitesStack.push(curSuite);
        }
      }
    }
  }
}

export { StaticSuite };
