import { ServiceAttrIF, ApiIF } from '../../interface';
import { NestFilter } from '../config/Filter';
import { Core } from '../../core';
import { StaticSpec } from './StaticSpec';
import { StaticSuite } from './StaticSuite';
import { TAG } from '../../Constant';
import {
  processFunc,
  processAsyncFunc,
  processFuncWithArgOne,
  processAsyncFuncWithArgOne,
  processFuncWithArgTwo,
  processAsyncFuncWithArgTwo,
  getFunctionArgumentsCount,
  checkIsAsyncFunction
} from '../../util';
import { SuiteService } from './SuiteService';
import { ConfigService } from '../config/configService';
import DataDriver from '../config/DataDriver';
import { ItfnType } from '../types/common';
class SpecService {
  public id: string;
  public totalTest: int;
  public hasError: boolean;
  public skipSpecNum: int;
  public isSkipSpec: boolean;
  public specSkipReason: string;
  public coreContext: Core | null;
  public currentRunningSpec: StaticSpec | null;
  constructor(attr: ServiceAttrIF) {
    this.id = attr.id;
    this.totalTest = 0;
    this.hasError = false;
    this.skipSpecNum = 0;
    this.isSkipSpec = false;
    this.specSkipReason = '';
    this.coreContext = null;
    this.currentRunningSpec = new StaticSpec({ description: '', fi: 0, fn: (): void => {} });
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
  }

  setCurrentRunningSpec(spec: StaticSpec | null) {
    this.currentRunningSpec = spec;
  }

  setStatus(hasError: boolean) {
    this.hasError = hasError;
  }

  getStatus(): boolean {
    return this.hasError;
  }

  getTestTotal() {
    return this.totalTest;
  }

  getCurrentRunningSpec() {
    return this.currentRunningSpec;
  }
  getSkipSpecNum() {
    return this.skipSpecNum;
  }

  initSpecService() {
    this.isSkipSpec = false;
    this.specSkipReason = '';
  }

  it(desc: string, filter: int, func: ItfnType): void {
    const coreContext = this.coreContext;
    if (coreContext !== null) {
      const suiteService = coreContext.getDefaultService('suite');
      const configService = coreContext.getDefaultService('config');

      if (suiteService !== null && configService !== null) {
        const sService = suiteService as SuiteService;
        const cService = configService as ConfigService;
        let isFilter = new NestFilter().filterNestName(
          sService.targetSuiteArray,
          sService.targetSpecArray,
          sService.suitesStack,
          desc
        );

        if (cService.filterWithNest(desc, filter)) {
          this.initSpecService();
          return;
        }
        const curSuite = sService.getCurrentRunningSuite() as StaticSuite;
        if (cService.filterDesc(curSuite.description, desc, filter) && isFilter && !sService.fullRun) {
          this.initSpecService();
        } else {
          const funcStr = (func as object).toString();
          const argsCount = getFunctionArgumentsCount(funcStr);

          const isAsync = checkIsAsyncFunction(funcStr);
          let processedFunc: () => Promise<void> = (): Promise<void> => {
            return Promise.resolve();
          };
          if (argsCount === 1) {
            if (isAsync) {
              const fn = func as (done: () => void) => Promise<void>;
              processedFunc = processAsyncFuncWithArgOne(coreContext, fn);
            } else {
              const fn = func as (done: () => void) => void;
              processedFunc = processFuncWithArgOne(coreContext, fn);
            }
          } else {
            if (isAsync) {
              const fn = func as () => Promise<void>;
              processedFunc = processAsyncFunc(coreContext, fn);
            } else {
              const fn = func as () => void;
              processedFunc = processFunc(coreContext, fn);
            }
          }
          const spec = new StaticSpec({ description: desc, fi: filter, asyncFn: processedFunc });
          if (this.isSkipSpec) {
            spec.isSkip = true;
            spec.skipReason = this.specSkipReason;
          }
          this.initSpecService();
          if (cService.runSkipped === 'skipped' && !spec.isSkip) {
            console.info(`${TAG} runSkipped is skipped , just run xit, don't run it: ${spec.description}`);
            return;
          }

          if (curSuite.isSkip && !spec.isSkip) {
            cService.filterXdescribe.push(curSuite.description);
          }
          if (cService.dryRun !== 'true') {
            const dataDriverMap = coreContext.getServices('dataDriver');
            if (dataDriverMap) {
              const dataDriver = dataDriverMap.get('dataDriver') as DataDriver;
              const specStress = dataDriver.getSpecStress(desc);
              for (let i = 1; i < specStress; i++) {
                  this.totalTest++;
                  curSuite.pushSpec(spec);
              }
            }
            let stress = cService.getStress(); // 命令配置压力测试
            for (let i = 1; i < stress; i++) {
              this.totalTest++;
              curSuite.pushSpec(spec);
            }
          }

          this.totalTest++;
          curSuite.pushSpec(spec);
        }
      }
    }
  }

  xit(desc: string, filter: int, func: ItfnType, reason: string) {
    const coreContext = this.coreContext;
    if (coreContext) {
      const suiteService = coreContext.getDefaultService('suite');
      const configService = coreContext.getDefaultService('config');
      if (suiteService !== null && configService !== null) {
        const sService = suiteService as SuiteService;
        const cService = configService as ConfigService;
        if (!cService.skipMessage && cService.runSkipped !== 'all') {
          if (cService.runSkipped != null && cService.runSkipped !== '') {
            let finalDesc = '';
            sService.suitesStack.map((suite: StaticSuite) => {
              finalDesc = finalDesc + '.' + suite.description;
            });
            finalDesc = (finalDesc + '#' + desc).substring(2);
            if (cService.checkIfSpecInSkipRun(finalDesc)) {
              console.info(`${TAG} runSkipped spec: ${desc}`);
            } else {
              console.info(
                reason == null ? `${TAG} skip spec: ${desc}` : `${TAG} skip spec: ${desc}, and the reason is ${reason}`
              );
              return;
            }
          } else {
            console.info(
              reason == null ? `${TAG} skip spec: ${desc}` : `${TAG} skip spec: ${desc}, and the reason is ${reason}`
            );
            return;
          }
        }
        this.skipSpecNum++;
        this.isSkipSpec = true;
        this.specSkipReason = reason;
        this.it(desc, filter, func);
      }
    }
  }

  apis(): ApiIF {
    const _this = this;
    return {
      name: 'SpecService',
      it: (desc: string, filter: int, func: ItfnType) => {
        return _this.it(desc, filter, func);
      },
      xit: (desc: string, filter: int, func: ItfnType, reason: string) => {
        return _this.xit(desc, filter, func, reason);
      },
    };
  }
}

export { SpecService };
