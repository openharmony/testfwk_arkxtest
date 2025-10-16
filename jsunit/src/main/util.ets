'use static'
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
import { Core } from './core';
import { ConfigService } from './module/config/configService';
import { ItfnType, AnyType } from './module/types/common';
import { TAG } from './Constant';

export function getFuncWithArgsZero(func: () => Any, timeout: int, isStressTest: boolean) {
  return new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (value: Error) => void) => {
    let timer = 0;
    if (!isStressTest) {
      timer = setTimeout(() => {
        reject(new Error('execute timeout ' + timeout + 'ms'));
      }, timeout);
    }
    try {
      const res = func()
       if (res instanceof Promise) {
        await res;
      }
    } catch (err) {
      err = err as Error;
      console.info(`${TAG} getFuncWithArgsZero Error: ${err.message}`);
      reject(err)
    }
    if (!!timer) {
      clearTimeout(timer);
    }
    resolve(Promise.resolve());
  });
}

export function getFuncWithArgsOne(func: (params: () => void) => Any, timeout: int, isStressTest: boolean) {
  return new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (reason: Error) => void) => {
    let timer = 0;
    if (!isStressTest) {
      timer = setTimeout(() => {
        reject(new Error('execute timeout ' + timeout + 'ms'));
      }, timeout);
    }

    const done = () => {
      if (!!timer) {
        clearTimeout(timer);
      }
      resolve(Promise.resolve());
    };

    try {
      const res = func(done);
      if (res instanceof Promise) {
        await res;
      }
    } catch (err) {
      if (!!timer) {
        clearTimeout(timer);
      }
      reject(err as Error);
    }
  });
}

export function getFuncWithArgsTwo(
  func: (param1: () => void, param2: AnyType) => Any,
  timeout: int,
  paramItem: AnyType,
  isStressTest: boolean
) {
  return new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (reason: Error) => void) => {
    let timer = 0;
    if (!isStressTest) {
      timer = setTimeout(() => {
        reject(new Error('execute timeout ' + timeout + 'ms'));
      }, timeout);
    }

    const done = () => {
      if (!!timer) {
        clearTimeout(timer);
      }
      resolve(Promise.resolve());
    };

    try {
      const res = func(done, paramItem);
      if (res instanceof Promise) {
        await res;
      }
    } catch (err) {
      if (!!timer) {
        clearTimeout(timer);
      }
      reject(err as Error);
    }
  });
}

export function processFunc(
  coreContext: Core,
  func: () => Any
) {
  const configService = coreContext.getDefaultService('config');
  if (configService !== null) {
    const config = configService as ConfigService;
    config.setSupportAsync(true);
    const timeout = !config.timeout ? 5000 : Number(config.timeout);
    let isStressTest = false;
    if (config.getStress() > 1 || coreContext.getServices('dataDriver')) {
      isStressTest = true;
    } else {
      isStressTest = false;
    }
    return () => {
      return getFuncWithArgsZero(func, timeout.toInt(), isStressTest);
    };
  }
  return () => {
    return getFuncWithArgsZero(func, 0, false);
  };
}

export function processFuncWithArgOne(coreContext: Core, func: (done: () => void) => Any) {
  const configService = coreContext.getDefaultService('config');
  if (configService !== null) {
    const config = configService as ConfigService;
    config.setSupportAsync(true);
    const timeout = !config.timeout ? 5000 : Number(config.timeout);
    const isStressTest = coreContext.getServices('dataDriver') !== undefined || config.getStress() > 1;
    return () => {
      return getFuncWithArgsOne(func, timeout.toInt(), isStressTest);
    };
  }
  return () => {
    return getFuncWithArgsOne(func, 0, false);
  };
}

export function processFuncWithArgTwo(coreContext: Core, func: (param1: () => void, param2: AnyType) => Any) {
  const configService = coreContext.getDefaultService('config');
  if (configService !== null) {
    const config = configService as ConfigService;
    config.setSupportAsync(true);
    const timeout = !config.timeout ? 5000 : Number(config.timeout);
    const isStressTest = coreContext.getServices('dataDriver') !== undefined || config.getStress() > 1;
    return (paramItem: AnyType) => {
      return getFuncWithArgsTwo(func, timeout.toInt(), paramItem, isStressTest);
    };
  }
  return (paramItem: AnyType) => {
    return getFuncWithArgsTwo(func, 0, paramItem, false);
  };
}

export function getFunctionArgumentsCount(func: ItfnType): int {
  if (func instanceof (() => Any)) {
    return 0;
  } else if (func instanceof (((done: Any) => Any))) {
    return 1;
  } else if (func instanceof (((done: Any, arg2: Any) => Any))) {
    return 2;
  }

  return 0
}
