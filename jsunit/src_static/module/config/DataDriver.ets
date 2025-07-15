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
import { Core } from '../../core';
import { SuiteService } from '../service/SuiteService';
import { SpecService } from '../service/SpecService';
import { StaticSpec } from '../service/StaticSpec';
import {
  DataDriverData,
  DataDriverAttr,
  DataDriverSuite,
} from '../../interface';
import { AnyType } from '../types/common';

class DataDriver {
  public id: string;
  public coreContext: Core | null;
  public suiteService: SuiteService | null;
  public specService: SpecService | null;
  public data: DataDriverData;
  constructor(attr: DataDriverAttr) {
    this.id = 'dataDriver';
    this.data = attr.data;
    this.coreContext = null;
    this.suiteService = null;
    this.specService = null;
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
    let specParams: Record<string, AnyType>[] = [];
    return specParams;
  }

  getSpecParams() {
    let specParams: Record<string, AnyType>[] = [];
    return specParams;
  }

  getSuiteParams() {
    let suiteParams: Record<string, AnyType> = {};
    const suiteService = this.suiteService;
    if (suiteService) {
      const sService = suiteService as SuiteService;
      let suiteDesc = sService.getCurrentRunningSuite().description;
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
        if (
          suiteDesc &&
          Array.isArray(describeValue) &&
          (describeValue as Array<string>).includes(suiteDesc)
        ) {
          const params = suiteItem.params as Record<string, AnyType>;
          for (const entries of params.entries()) {
            if (!entries) {
              continue;
            }
            const key = entries[0];
            let value = entries[1];
            if (!key) {
              continue;
            }
            suiteParams[key] = value;
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
        let tempStress = specItem.stress;
        return tempStress && Number.isInteger(tempStress as number) && (tempStress as int) >= 1
          ? (tempStress as int)
          : stress;
      }
    }
    return stress;
  }

  getSpecStress(specDesc: string): int {
    let stress = 1;
    const suiteService = this.suiteService;
    if (suiteService) {
      const sService = suiteService as SuiteService;
      let suiteDesc = sService.getCurrentRunningSuite().description;
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
        if (
          suiteDesc &&
          Array.isArray(describeValue) &&
          (describeValue as Array<string>).includes(suiteDesc)
        ) {
          let specs: DataDriverSuite[] = [];
          const tempspecs = suite.items;
          if (tempspecs) {
            specs = tempspecs as DataDriverSuite[];
          }
          return this.getStressNum(specs, specDesc);
        }
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
      if (
        suiteDesc &&
        Array.isArray(describeValue) &&
        (describeValue as Array<string>).includes(suiteDesc)
      ) {
        let tempStress = suite.stress;
        return tempStress && Number.isInteger(tempStress as number) && (tempStress as int) >= 1
          ? (tempStress as int)
          : stress;
      }
    }
    return stress;
  }
}

export default DataDriver;
