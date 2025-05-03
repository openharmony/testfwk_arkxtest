import { StaticSpecIF } from '../../interface';
import { Core } from '../../core';
import { SpecService } from './SpecService';

class StaticSpec {
  public description: string;
  public fi: int;
  public fn: () => void;
  public asyncFn: (() => Promise<void>) | null;
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
        let dataDriver = coreContext.getServices('dataDriver');
        if (typeof dataDriver === 'undefined') {
          this.fn();
        } else {
        }
        this.setResult();
      } catch (e: Error) {
        this.error = e;
        specService.setStatus(true);
      }
    }
    coreContext.fireEvents('spec', 'specDone');
  }

  async asyncRun(coreContext: Core) {
    const dataDriver = coreContext.getServices('dataDriver');
    if (typeof dataDriver === 'undefined') {
      const asyncFn = this.asyncFn;
      const fn = this.fn;
      if (asyncFn !== null) {
        await asyncFn();
      } else if (fn !== null) {
        fn();
      }
    } else {
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
