/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
import { Core } from '../../core';
import { SuiteService } from '../service/SuiteService';
import { SpecService } from '../service/SpecService';
import { StaticSpec } from '../service/StaticSpec';
import { DataDriverData, DataDriverAttr, DataDriverSuite } from '../../interface';

class DataDriver {
  public id: string;
  public coreContext: Core | null;
  public suiteService: SuiteService;
  public specService: SpecService;
  public data: DataDriverData;
  constructor(attr: DataDriverAttr) {
    this.id = 'dataDriver';
    this.data = attr.data;
    this.coreContext = null;
  }

  init(coreContext: Core) {
    this.coreContext = coreContext;
    const suiteService = coreContext.getDefaultService('suite');
    if (suiteService) {
      this.suiteService = suiteService as SuiteService;
    }
    const specService = coreContext.getDefaultService('spec');
    if (specService) {
      this.specService = specService as SpecService;
    }
  }

  getSpecParamsValue(specs: DataDriverSuite[]) {
    let specParams: object[] = [];
    let specDesc = '';
    const currentRunningSpec = this.specService.getCurrentRunningSpec();
    if (currentRunningSpec) {
      specDesc = (currentRunningSpec as StaticSpec).description;
    }
    if (specs === null || specs === undefined) {
      return specParams;
    }
    for (const specItem of specs) {
      if (specItem.it && specItem.it === specDesc) {
        if (specItem.params && Array.isArray(specItem.params)) {
          return specItem.params as object[];
        } else {
          return specParams;
        }
      }
    }
    return specParams;
  }

  getSpecParams() {
    let specParams: object[] = [];
    let suiteDesc = this.suiteService.getCurrentRunningSuite().description;
    let suites: DataDriverSuite[] = [];
    const tempSuite = this.data.suites;
    if (tempSuite) {
      suites = tempSuite as DataDriverSuite[];
    }
    for (const suiteItem of suites) {
      const suite = suiteItem as DataDriverSuite;
      let describeValue: string[] = [];
      const tempdescribe = suite.describe;
      if (tempdescribe) {
        describeValue = tempdescribe as string[];
      }
      if (suiteDesc && Array.isArray(describeValue) && (describeValue as Array<string>).includes(suiteDesc)) {
        let specs: DataDriverSuite[] = [];
        const tempspecs = suite.items;
        if (tempspecs) {
          specs = tempspecs as DataDriverSuite[];
        }
        return this.getSpecParamsValue(specs);
      }
    }
    return specParams;
  }

  getSuiteParams() {
    let suiteParams = new Map<string, object | undefined | null>();
    let suiteDesc = this.suiteService.getCurrentRunningSuite().description;
    let suites: DataDriverSuite[] = [];
    const tempSuite = this.data.suites;
    if (tempSuite) {
      suites = tempSuite as DataDriverSuite[];
    }
    for (const suiteItem of suites) {
      let describeValue: string[] = [];
      const tempdescribe = suiteItem.describe;
      if (tempdescribe) {
        describeValue = tempdescribe as string[];
      }
      if (suiteDesc && Array.isArray(describeValue) && (describeValue as Array<string>).includes(suiteDesc)) {
        const params = suiteItem.params as object;
        for (const entries of Object.entries(params)) {
          if (!entries) {
            continue;
          }
          const key = entries[0];
          let value = entries[1];
          if (!key) {
            continue;
          }
          if (value === undefined) {
            suiteParams.set(key, value as undefined);
          } else if (value === null) {
            suiteParams.set(key, value as null);
          } else {
            suiteParams.set(key, value as object);
          }
        }
      }
    }
    return suiteParams;
  }

  getStressNum(specs: DataDriverSuite[], specDesc: string): int {
    let stress = 1;
    if (specs === null || specs === undefined) {
      return stress;
    }
    for (const specItem of specs) {
      if (specItem.it && specItem.it === specDesc) {
        let tempStress = Number(specItem.stress);
        return Number.isInteger(tempStress) && tempStress >= 1 ? tempStress : stress;
      }
    }
    return stress;
  }

  getSpecStress(specDesc: string): int {
    let stress = 1;
    let suiteDesc = this.suiteService.getCurrentRunningSuite().description;
    let suites: DataDriverSuite[] = [];
    const tempSuite = this.data.suites;
    if (tempSuite) {
      suites = tempSuite as DataDriverSuite[];
    }
    for (const suiteItem of suites) {
      const suite = suiteItem as DataDriverSuite;
      let describeValue: string[] = [];
      const tempdescribe = suite.describe;
      if (tempdescribe) {
        describeValue = tempdescribe as string[];
      }
      if (suiteDesc && Array.isArray(describeValue) && (describeValue as Array<string>).includes(suiteDesc)) {
        let specs: DataDriverSuite[] = [];
        const tempspecs = suite.items;
        if (tempspecs) {
          specs = tempspecs as DataDriverSuite[];
        }
        return this.getStressNum(specs, specDesc);
      }
    }
    return stress;
  }

  getSuiteStress(suiteDesc: string): int {
    let stress = 1;
    let suites: DataDriverSuite[] = [];
    const tempSuite = this.data.suites;
    if (tempSuite) {
      suites = tempSuite as DataDriverSuite[];
    }
    for (const suiteItem of suites) {
      const suite = suiteItem as DataDriverSuite;
      let describeValue: string[] = [];
      const tempdescribe = suite.describe;
      if (tempdescribe) {
        describeValue = tempdescribe as string[];
      }
      if (suiteDesc && Array.isArray(describeValue) && (describeValue as Array<string>).includes(suiteDesc)) {
        let tempStress = Number(suite.stress);
        return Number.isInteger(tempStress) && tempStress >= 1 ? tempStress : stress;
      }
    }
    return stress;
  }
}

export default DataDriver;
