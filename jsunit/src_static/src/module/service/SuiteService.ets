import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import { ServiceAttrIF, TestcaseSummaryIF, ItItemIF, ConfigIf, DryrunResultIF, ApiIF } from '../../interface';
import { StaticSuite } from './StaticSuite';
import { StaticSpec } from './StaticSpec';
import { TAG } from '../../Constant';
import { Core } from '../../core';
import { processFunc, checkIsAsyncFunction } from '../../util';
import { SysTestKit } from '../kit/SysTestKit';
import { AssertException } from './AssertException';
import { ConfigService } from '../config/configService';
import { SpecService } from './SpecService';

class SuiteService {
  public id: string;
  public rootSuite: StaticSuite;
  public currentRunningSuite: StaticSuite;
  public suitesStack: Array<StaticSuite>;
  public suiteSkipReason: string;
  public isSkipSuite: boolean;
  public coreContext: Core | null;
  public currentRunningSuiteDesc: string;
  public fullRun: boolean;
  public targetSuiteArray: Array<string>;
  public targetSpecArray: Array<string>;
  private dryrunResult: DryrunResultIF;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.rootSuite = new StaticSuite({
      description: '',
    });
    this.currentRunningSuite = this.rootSuite;
    this.suitesStack = new Array<StaticSuite>(this.rootSuite);
    this.targetSuiteArray = new Array<string>();
    this.targetSpecArray = new Array<string>();
    this.currentRunningSuiteDesc = '';
    this.fullRun = false;
    this.isSkipSuite = false;
    this.suiteSkipReason = '';
    this.coreContext = null;
    const suites = new Array<Map<string, Array<ConfigIf>>>();
    const skipSuites = new Array<Map<string, Array<ConfigIf>>>();
    this.dryrunResult = {
      suites,
      skipSuites,
    };
  }

  async describe(desc: string, func: (() => void) | (() => Promise<void>)): Promise<undefined> {
    const core = this.coreContext;
    if (core) {
      const cc = core as Core;
      const cService = cc.getDefaultService('config');
      if (cService !== null) {
        const configService = cService as ConfigService;
        if (
          this.suitesStack.some((suite: StaticSuite) => {
            return suite.description === desc;
          })
        ) {
          console.error(`${TAG} Loop nesting occurs : ${desc}`);
          this.suiteSkipReason = '';
          this.isSkipSuite = false;
          return Promise.resolve(undefined);
        }
        const isFilter = this.analyzeConfigServiceClass(configService.className, desc);
        if (configService.filterSuite(desc) && isFilter) {
          if (this.currentRunningSuite.description === '' || this.currentRunningSuite.description == null) {
            console.info(`${TAG}filter suite : ${desc}`);
            this.suiteSkipReason = '';
            this.isSkipSuite = false;
            return Promise.resolve(undefined);
          }
        }
        const suite = new StaticSuite({ description: desc });
        if (this.isSkipSuite) {
          suite.isSkip = true;
          suite.skipReason = this.suiteSkipReason;
        }
        this.suiteSkipReason = '';
        this.isSkipSuite = false;
        this.currentRunningSuite.childSuites.push(suite);
        this.currentRunningSuite = suite;
        this.suitesStack.push(suite);
        const funcStr = (func as object).toString();
        const isAsyncFunc = checkIsAsyncFunction(funcStr);
        if (isAsyncFunc) {
          await (func as () => Promise<void>)();
        } else {
          (func as () => void)();
        }
        this.suitesStack.pop();

        const staticService = this.suitesStack.pop();
        if (staticService) {
          this.currentRunningSuite = staticService;
          this.suitesStack.push(staticService);
        }
      }
    }
  }
  async xdescribe(desc: string, func: (() => void) | (() => Promise<void>), reason: string): Promise<void> {
    const core = this.coreContext;
    if (core) {
      const cc = core as Core;
      const cService = cc.getDefaultService('config');
      if (cService !== null) {
        const configService = cService as ConfigService;
        if (!configService.skipMessage && configService.runSkipped !== 'all') {
          if (configService.runSkipped != null && configService.runSkipped !== '') {
            let finalDesc = '';
            this.suitesStack.map((suite: StaticSuite) => {
              finalDesc = finalDesc + '.' + suite.description;
            });
            finalDesc = (finalDesc + '.' + desc).substring(2);
            console.info(`${TAG} finalDesc ${finalDesc}`);
            if (configService.checkIfSuiteInSkipRun(finalDesc)) {
              console.info(`${TAG} runSkipped suite: ${desc}`);
            } else {
              console.info(
                reason == null
                  ? `${TAG} skip suite: ${desc}`
                  : `${TAG} skip suite: ${desc}, and the reason is ${reason}`
              );
              return Promise.resolve();
            }
          } else {
            console.info(
              reason == null ? `${TAG} skip suite: ${desc}` : `${TAG} skip suite: ${desc}, and the reason is ${reason}`
            );
            return Promise.resolve();
          }
        }
        this.isSkipSuite = true;
        this.suiteSkipReason = reason;
        await this.describe(desc, func);
      }
    }
    return Promise.resolve();
  }

  beforeAll(func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core !== null) {
      const fn = processFunc(core as Core, func);
      this.currentRunningSuite.beforeAll.add(fn);
    }
  }

  beforeEach(func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core !== null) {
      this.currentRunningSuite.beforeEach.add(processFunc(core as Core, func));
    }
  }

  beforeItSpecified(itDescs: string, func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core !== null) {
      this.currentRunningSuite.beforeItSpecified.set(itDescs, processFunc(core as Core, func));
    }
  }

  afterItSpecified(itDescs: string, func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core !== null) {
      this.currentRunningSuite.afterItSpecified.set(itDescs, processFunc(core as Core, func));
    }
  }

  afterAll(func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core !== null) {
      this.currentRunningSuite.afterAll.add(processFunc(core as Core, func));
    }
  }

  afterEach(func: (() => void) | (() => Promise<void>)) {
    const core = this.coreContext;
    if (core) {
      this.currentRunningSuite.afterEach.add(processFunc(core as Core, func));
    }
  }

  getCurrentRunningSuite() {
    return this.currentRunningSuite;
  }

  setCurrentRunningSuite(suite: StaticSuite) {
    this.currentRunningSuite = suite;
  }

  getRootSuite() {
    return this.rootSuite;
  }

  getCurrentRunningSuiteDesc() {
    return this.currentRunningSuiteDesc;
  }
  setCurrentRunningSuiteDesc(suite: StaticSuite, currentSuite: StaticSuite, prefix: string) {
    if (suite != null && suite === currentSuite) {
      this.currentRunningSuiteDesc = prefix;
    } else if (suite != null && suite !== currentSuite) {
      suite.childSuites.forEach((it: StaticSuite) => {
        let temp = prefix;
        if (it.description != null || it.description !== '') {
          temp = prefix === '' ? it.description : prefix + '.' + it.description;
        }
        this.setCurrentRunningSuiteDesc(it, currentSuite, temp);
      });
    }
  }
  analyzeConfigServiceClass(configServiceClass: string, desc: string) {
    if (configServiceClass == null || configServiceClass === '') {
      this.fullRun = true;
      return false;
    }
    const targetList = configServiceClass.split(',');
    const targetArray = new Array<string>();
    for (let target of targetList) {
      targetArray.push(target);
    }
    const mapList = targetArray.map((item: string) => item.trim());
    const mapArray = new Array<string>();
    for (let mapValue of mapList) {
      mapArray.push(mapValue);
    }
    const filterList = mapArray.filter((item: string) => item !== '');
    if (this.targetSuiteArray.length === 0) {
      for (let target of filterList) {
        if (target.includes('#')) {
          this.targetSpecArray.push(target);
        } else {
          this.targetSuiteArray.push(target);
        }
      }
    }
    return targetArray.indexOf(desc) === -1;
  }
  traversalResults(suite: StaticSuite, obj: TestcaseSummaryIF, breakOnError: boolean) {
    if (suite.childSuites.length === 0 && suite.specs.length === 0) {
      return;
    }
    if (suite.specs.length > 0 && obj) {
      for (const itItem of suite.specs) {
        obj.total++;
        const itInfo: ItItemIF = {
          currentThreadName: 'mainThread',
          description: suite.description + '#' + itItem.description,
          result: -3,
        };
        obj.itItemList.push(itInfo);
        if (breakOnError && (obj.error > 0 || obj.failure > 0)) {
          // breakOnError模式
          continue;
        }
        if (itItem.error) {
          obj.error++;
          itInfo.result = -1;
        } else if (itItem.fail) {
          obj.failure++;
          itInfo.result = -2;
        } else if (itItem.pass === true) {
          obj.pass++;
          itInfo.result = 0;
        }
      }
    }

    obj.duration += suite.duration;

    if (suite.childSuites.length > 0) {
      for (const suiteItem of suite.childSuites) {
        this.traversalResults(suiteItem, obj, breakOnError);
      }
    }
    return;
  }

  async setSuiteResults(suite: StaticSuite, error: Error, coreContext: Core) {
    if (suite.childSuites.length > 0 && suite.specs.length > 0) {
      if (suite.specs.length > 0) {
        const specService = coreContext.getDefaultService('spec');
        if (specService !== null) {
          const ss = specService as SpecService;
          for (const specItem of suite.specs) {
            ss.setCurrentRunningSpec(specItem);
            if ((error as Error) instanceof AssertException) {
              specItem.fail = error;
            } else {
              specItem.error = error;
            }
            await coreContext.fireEvents('spec', 'specStart');
            await coreContext.fireEvents('spec', 'specDone');
          }
        }
      }
      if (suite.childSuites.length > 0) {
        for (const suiteItem of suite.childSuites) {
          await this.setSuiteResults(suiteItem, error, coreContext);
        }
      }
    }
  }

  getSummary(): TestcaseSummaryIF {
    const core = this.coreContext;
    const obj: TestcaseSummaryIF = {
      total: 0,
      failure: 0,
      error: 0,
      pass: 0,
      ignore: 0,
      duration: 0,
      itItemList: new Array<ItItemIF>(),
    };
    if (core) {
      const suiteService = core.getDefaultService('suite');
      const specService = core.getDefaultService('spec');
      const configService = core.getDefaultService('config');
      if (suiteService !== null && specService !== null && configService !== null) {
        const suiteS = suiteService as SuiteService;
        const specS = specService as SpecService;
        const configS = configService as ConfigService;
        const rootSuite = suiteS.rootSuite;
        const breakOnError = configS.isBreakOnError();
        const isError = specS.getStatus();
        const isBreaKOnError = breakOnError && isError;

        const childrenSuite = rootSuite.childSuites as Array<StaticSuite>;
        for (const suiteItem of childrenSuite) {
          this.traversalResults(suiteItem, obj, isBreaKOnError);
        }
        obj.ignore = obj.total - obj.pass - obj.failure - obj.error;
      }
    }
    return obj;
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
  }

  async dryRun(abilityDelegator: abilityDelegatorRegistry.AbilityDelegator) {
    console.info(`${TAG} rootSuite : ` + JSON.stringify(this.rootSuite));
    const suiteArray = new Array<Map<string, Array<ConfigIf>>>();
    const skipSuiteArray = new Array<Map<string, Array<ConfigIf>>>();
    this.analyzeSuitesArray(new Array<string>(), suiteArray, skipSuiteArray, this.rootSuite);
    const core = this.coreContext;
    if (core) {
      const configService = core.getDefaultService('config');
      if (configService !== null) {
        const configS = configService as ConfigService;
        if (configS.skipMessage) {
          this.dryrunResult = {
            suites: suiteArray,
            skipSuites: skipSuiteArray,
          };
        } else {
          this.dryrunResult = {
            suites: suiteArray,
          };
        }
        const strJson = JSON.stringify(this.dryrunResult);
        const strLen = strJson.length;
        const maxLen = 500;
        const maxCount = Math.floor(strLen / maxLen);
        for (let count = 0; count <= maxCount; count++) {
          await SysTestKit.print(strJson.substring(count * maxLen, (count + 1) * maxLen));
        }
        console.info(`${TAG}dryRun print success`);
        abilityDelegator.finishTest('dry run finished!!!', 0);
      }
    }
  }
  analyzeSuitesArray(
    prefixStack: Array<string>,
    suiteArray: Array<Map<string, Array<ConfigIf>>>,
    skipSuiteArray: Array<Map<string, Array<ConfigIf>>>,
    rootSuite: StaticSuite
  ) {
    rootSuite.childSuites.map((suite: StaticSuite) => {
      if (suite.description != null && suite.description !== '') {
        let prefix = '';
        if (prefixStack.length > 0) {
          prefix = prefixStack.join('.') + '.' + suite.description;
        } else {
          prefix = suite.description;
        }
        prefixStack.push(suite.description);
        const temp = new Map<string, Array<ConfigIf>>();
        temp.set(prefix, new Array<ConfigIf>());
        const skipTemp = new Map<string, Array<ConfigIf>>();
        skipTemp.set(prefix, new Array<ConfigIf>());
        suite.specs.map((spec: StaticSpec) => {
          const it: ConfigIf = { itName: spec.description };
          const skipArr = skipTemp.get(prefix);
          const tempArr = temp.get(prefix);
          if (spec.isSkip) {
            if (skipArr) {
              skipArr.push(it);
            }
          } else if (tempArr) {
            tempArr.push(it);
          }
        });
        suiteArray.push(temp);
        skipSuiteArray.push(skipTemp);
      }
      this.analyzeSuitesArray(prefixStack, suiteArray, skipSuiteArray, suite);
      prefixStack.pop();
    });
  }

  getAllChildSuiteNum(suite: StaticSuite, specArray: Array<StaticSpec>) {
    if (suite.specs != null) {
      suite.specs.forEach((spec: StaticSpec) => {
        specArray.push(spec);
      });
    }
    if (suite.childSuites != null) {
      suite.childSuites.forEach((it: StaticSuite) => {
        this.getAllChildSuiteNum(it, specArray);
      });
    }
  }

  async executeCases(core: Core) {
    await core.fireEvents('task', 'taskStart');
    await this.rootSuite.asyncRun(core);
    await core.fireEvents('task', 'taskDone');
  }

  execute() {
    const cc = this.coreContext;
    if (cc) {
      const core = cc as Core;
      const configS = core.getDefaultService('config');
      if (configS !== null) {
        const configService = configS as ConfigService;
        if (configService.filterValid.length !== 0) {
          core.fireEvents('task', 'incorrectFormat');
          return;
        }
        if (configService.filterXdescribe.length !== 0) {
          core.fireEvents('task', 'incorrectTestSuiteFormat');
          return;
        }
        if (configService.isRandom() && this.rootSuite.childSuites.length > 0) {
          this.rootSuite.childSuites.sort((a: StaticSuite, b: StaticSuite) => {
            return Math.random() > 0.5 ? -1 : 1;
          });
          this.currentRunningSuite = this.rootSuite.childSuites[0];
        }
        if (configService.isSupportAsync()) {
          let asyncExecute = async () => {
            await core.fireEvents('task', 'taskStart');
            await this.rootSuite.asyncRun(core);
          };
          asyncExecute().then(async () => {
            await core.fireEvents('task', 'taskDone');
          });
        } else {
          core.fireEvents('task', 'taskStart');
          this.rootSuite.run(core);
          core.fireEvents('task', 'taskDone');
        }
      }
    }
  }

  apis(): ApiIF {
    const _this = this;
    return {
      name: 'SuiteService',
      describe: (desc: string, func: (() => void) | (() => Promise<void>)) => {
        const c = _this.describe(desc, func);
        return c;
      },
      xdescribe: (desc: string, func: (() => void) | (() => Promise<void>), reason: string) => {
        return _this.xdescribe(desc, func, reason);
      },
      beforeItSpecified: (itDescs: string, func: (() => void) | (() => Promise<void>)) => {
        return _this.beforeItSpecified(itDescs, func);
      },
      afterItSpecified: (itDescs: string, func: (() => void) | (() => Promise<void>)) => {
        return _this.afterItSpecified(itDescs, func);
      },
      beforeAll: (func: (() => void) | (() => Promise<void>)) => {
        return _this.beforeAll(func);
      },
      beforeEach: (func: (() => void) | (() => Promise<void>)) => {
        return _this.beforeEach(func);
      },
      afterAll: (func: (() => void) | (() => Promise<void>)) => {
        return _this.afterAll(func);
      },
      afterEach: (func: (() => void) | (() => Promise<void>)) => {
        return _this.afterEach(func);
      },
    };
  }
}

export { SuiteService };
