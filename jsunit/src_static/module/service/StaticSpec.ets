import { StaticSpecIF } from '../../interface';
import { Core } from '../../core';
import { SpecService } from './SpecService';
import DataDriver from '../config/DataDriver';
import { getFunctionArgumentsCount } from '../../util';
import { AnyType } from '../types/common';
class StaticSpec {
  public description: string;
  public fi: int;
  public fn: () => void;
  public asyncFn: (() => Promise<void>) | ((params: AnyType) => Promise<void>) | null;
  public isSkip: boolean;
  public skipReason: string;
  public fail: Error | null;
  public pass: boolean;
  public isExecuted: boolean;
  public error: Error | null;
  public expectMsg: string;
  public duration: number;
  public startTime: number;
  constructor(attrs: StaticSpecIF) {
    this.description = attrs.description;
    this.fi = attrs.fi;
    this.fail = null;
    this.pass = true;
    this.error = null;
    this.duration = 0;
    this.startTime = 0;
    this.isExecuted = false; // 当前用例是否执行
    this.isSkip = false;
    this.skipReason = '';
    this.expectMsg = '';
    const fn = attrs.fn;
    if (fn) {
      this.fn = fn;
    } else {
      this.fn = () => {};
    }
    const asyncFn = attrs.asyncFn;
    if (asyncFn) {
      this.asyncFn = asyncFn;
    } else {
      this.asyncFn = null;
    }
  }

  setResult() {
    if (this.fail) {
      this.pass = false;
    } else {
      this.pass = true;
    }
  }

  run(coreContext: Core) {
    const sService = coreContext.getDefaultService('spec');

    coreContext.fireEvents('spec', 'specStart');
    if (sService !== null) {
      const specService = sService as SpecService;
      specService.setCurrentRunningSpec(this);
      this.isExecuted = true;
      try {
        this.fn();
        this.setResult();
      } catch (e: Error) {
        this.error = e;
        specService.setStatus(true);
      }
    }
    coreContext.fireEvents('spec', 'specDone');
  }

  async asyncRun(coreContext: Core) {
    const dataDriverMap = coreContext.getServices('dataDriver');
    if(dataDriverMap) {
      const dataDriver = dataDriverMap.get('dataDriver') as DataDriver;
      const suiteParams = dataDriver.getSuiteParams();
      const specParams = dataDriver.getSpecParams();
      
      const funcStr = (this.asyncFn as object).toString();
      const argsCount = getFunctionArgumentsCount(funcStr)
      if(argsCount === 1) {
        if(specParams.length === 0) {
          await (this.asyncFn as ((params: AnyType) => Promise<void>))(suiteParams);
        } else {
          for(const paraItem of specParams) {
            // todo
            // const params = {...suiteParams, ...paraItem}
            // await (this.asyncFn as ((params: AnyType) => Promise<void>))(params);
          }
        }
        
      } else {
        await (this.asyncFn as (() => Promise<void>))();
      }
    } else {
      const asyncFn = this.asyncFn;
      const fn = this.fn;
      if (asyncFn !== null) {
        await (asyncFn as (() => Promise<void>))();
      } else if (fn !== null) {
        fn();
      }
    } 

    this.isExecuted = true;
  }

  filterCheck(coreContext: Core) {
    const sService = coreContext.getDefaultService('spec');
    if (sService !== null) {
      const specService = sService as SpecService;
      specService.setCurrentRunningSpec(this);
    }
    return true;
  }
}

export { StaticSpec };
