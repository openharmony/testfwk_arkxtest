<div style="text-align: center;font-size: xxx-large" >Hypium</div>
<div style="text-align: center">A unit test framework for OpenHarmonyOS application</div>

## Hypium是什么?
***
- Hypium是OpenHarmony上的测试框架，提供测试用例编写、执行、结果显示能力，用于OpenHarmony系统应用接口以及应用界面测试。
- Hypium结构化模型：hypium工程主要由List.test.js与TestCase.test.js组成。
```
rootProject                  // Hypium工程根目录
├── moduleA
│   ├── src
│      ├── main                   // 被测试应用目录
│      ├── ohosTest               // 测试用例目录
│         ├── ets
│            └── test
│               └── List.test.ets      // 测试用例加载脚本，ets目录下为.ets后缀
│               └── TestCase.test.ets  // 测试用例脚本，ets目录下为.ets后缀
└── moduleB
    ...
│               └── List.test.ets      // 测试用例加载脚本，ets目录下为.ets后缀
│               └── TestCase.test.ets  // 测试用例脚本，ets目录下为.ets后缀
```

## 安装使用

- 方式一
```javascript
ohpm i @ohos/hypium
```
- 方式二
- 在DevEco Studio内使用Hypium
- 工程级oh-package.json5内配置:
```json
"dependencies": {
    "@ohos/hypium": "1.0.24"
}
```
注：
hypium服务于OpenHarmonyOS应用对外接口测试、系统对外接口测试（SDK中接口），完成HAP自动化测试。

## 引入方式
```javascript
import { describe, it, expect } from '@ohos/hypium';
```


## 功能特性

| No.  | 特性     | 功能说明                                              |
| ---- | -------- |---------------------------------------------------|
| 1    | 基础流程 | 支持编写及异步执行基础用例。                                    |
| 2    | 断言库   | 判断用例实际结果值与预期值是否相符。                                |
| 3    | 异步代码测试   | 等待异步任务完成之后再判断测试是否成功。                                |
| 4    | 公共能力   | 支持获取用例信息的基础能力以及日志打印、清楚等能力。                                |
| 5    | Mock能力 | 支持函数级Mock能力，对定义的函数进行Mock后修改函数的行为，使其返回指定的值或者执行某种动作。 |
| 6    | 数据驱动 | 提供数据驱动能力，支持复用同一个测试脚本，使用不同输入数据驱动执行。                |
| 7    | 专项能力 | 支持测试套与用例筛选、随机执行、压力测试、超时设置、遇错即停模式，跳过，支持测试套嵌套等。     |



### 基础流程

#### describe

describe(testSuiteName: string, func: Function): void

定义一个测试套。

**参数：**

| 参数名 | 类型     | 必填 | 说明                                                       | 
| --- |--------|----|----------------------------------------------------------|
| testSuiteName | string | 是  | 指定测试套的名称                                                 |
| func | Function | 否  | 指定测试套函数，用于注册测试用例。**注意：测试套函数不支持异步函数**                     |

**示例：**
```javascript
  import { describe, it, expect } from '@ohos/hypium';

  export default async function assertCloseTest() {
    describe('assertClose', function () {
      it('assertClose_success', 0, function () {
        let a = 100;
        let b = 0.1;
        expect(a).assertClose(99, b);
      })
    })
  }
```

#### beforeAll

beforeAll(func: Function): void

在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次

**参数：**

| 参数名 | 类型     | 必填    | 说明                                                 | 
|------------| ------ |-------|----------------------------------------------------|
| func       | Function | 否     | 预置动作函数，在一组测试用例（测试套）开始执行之前 |

**示例：**
```javascript
import { describe, beforeAll, it } from '@ohos/hypium';
export default function customAssertTest() {
    describe('customAssertTest', () => {
       beforeAll(async ()=> {
          console.info('beforeAll')
       })
        it('assertClose_success', 0, function () {
            let a = 100;
            let b = 0.1;
            expect(a).assertClose(99, b);
        })
    });
}
```


#### beforeEach

beforeEach(func: Function): void

在测试套内定义一个单元预置条件，在每条测试用例开始前执行,执行次数与it定义的测试用例数一致

**参数：**

| 参数名 | 类型     | 必填    | 说明                                                  | 
|------------| ------ |-------|-----------------------------------------------------|
| func       | Function | 否     | 预置动作函数，在一组测试用例（测试套）开始执行之前  |

**示例：**
```javascript
import { describe, beforeEach, it } from '@ohos/hypium';
let str = "";
export default function test() {
   describe('test0', () => {
       beforeEach(async () => {
           str += "A"
       })
       it('test0000', 0, () => {
           expect(str).assertEqual("BA");
       })
   })
}
```

#### afterEach

afterEach(func: Function): void

在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致

**参数：**

| 参数名 | 类型     | 必填    | 说明                          | 
|------------| ------ |-------|-----------------------------|
| func       | Function | 否     | 清理动作函数，在一组测试用例（测试套）执行完成后运行,用于 释放资源、重置状态、清除数据 |

**示例：**
```javascript
import { describe, afterEach, it } from '@ohos/hypium';
let str = "";
export default function test() {
   describe('test0', () => {
      afterEach(async () => {
           str += "A"
       })
       it('test0000', 0, () => {
           expect(str).assertEqual("BA");
       })
   })
}
```

#### afterAll

afterAll(func: Function): void

在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次

**参数：**

| 参数名 | 类型     | 必填    | 说明                          | 
|------------| ------ |-------|-----------------------------|
| func       | Function | 否     | 清理动作函数，在一组测试用例（测试套）执行完成后运行,用于 释放资源、重置状态、清除数据 |

**示例：**
```javascript
import { describe, afterAll, it } from '@ohos/hypium';
export default function customAssertTest() {
    describe('customAssertTest', () => {
       afterAll(async ()=> {
          console.info('afterAll')
       })
        it('assertClose_success', 0, function () {
            let a = 100;
            let b = 0.1;
            expect(a).assertClose(99, b);
        })
    });
}
```
//// Array<string>\string预览有问题
#### beforeItSpecified

beforeItSpecified(testCaseNames: Array<string> | string, func: Function): void

@since1.0.15在测试套内定义一个单元预置条件，仅在指定测试用例开始前执行。

**参数：**

| 参数名 | 类型                   | 必填 | 说明 | 
|------------|----------------------|----|----|
| testCaseNames   | Array<>string\string | 是  | 单个用例名称或用例名称数组|
|      func      |          Function            | 否  |  预置动作函数，在一组测试用例（测试套）开始执行之前  |


#### afterItSpecified

afterItSpecified(testCaseNames: Array<string> | string, func: Function): void

@since1.0.15在测试套内定义一个单元清理条件，仅在指定测试用例结束后执行

**参数：**

| 参数名 | 类型                   | 必填 | 说明 | 
|------------|----------------------|----|----|
| testCaseNames   | Array<>string\string | 是  | 单个用例名称或用例名称数组|
|      func      |          Function            | 否  |  清理动作函数，在一组测试用例（测试套）执行完成后运行,用于 释放资源、重置状态、清除数据 |


**beforeItSpecified, afterItSpecified示例：**
```javascript
import { describe, it, expect, beforeItSpecified, afterItSpecified } from '@ohos/hypium';
export default function beforeItSpecifiedTest() {
   describe('beforeItSpecifiedTest', () => {
      beforeItSpecified(['String_assertContain_success'], () => {
         const num:number = 1;
         expect(num).assertEqual(1);
      })
      afterItSpecified(['String_assertContain_success'], async (done: Function) => {
         const str:string = 'abc';
         setTimeout(()=>{
            try {
               expect(str).assertContain('b');
            } catch (error) {
               console.error(`error message ${JSON.stringify(error)}`);
            }
            done();
         }, 1000)
      })
      it('String_assertContain_success', 0, () => {
         let a: string = 'abc';
         let b: string = 'b';
         expect(a).assertContain(b);
         expect(a).assertEqual(a);
      })
   })
}
```


//attribute 类型不清楚
#### it

it(testCaseName: string,attribute: TestType | Size | Level,func: Function): void

定义一条测试用例

**参数：**

| 参数名 | 类型                 | 必填 | 说明                                                   | 
|------------|--------------------|----|------------------------------------------------------|
| testSuiteName      | string             | 是  | 指定测试套的名称 |
|      attribute              | | 是  |    过滤参数，提供Level、Size、TestType 对象，对测试用例进行标记，以区分测试用例的级别、粒度、测试类型      |
|         func                    |Function |  否  |        指定测试套函数，用于注册测试用例。                                                            |

**示例：**
```javascript
import { describe, it, expect, TestType, Size, Level } from '@ohos/hypium';

export default function attributeTest() {
   describe('attributeTest', () => {
      it("testAttributeIt", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
         console.info('Hello Test');
      })
   })
}
```

#### xdescribe

xdescribe(testSuiteName: string, func: Function): void

指定跳过一个测试套。

**参数：**

| 参数名 | 类型     | 必填| 说明            | 
| --- | ------ |---|---------------|
| testSuiteName | string | 是 | 指定跳过测试套的名称    |
| func | Function | 否 | 指定测试套函数，用于注册测试用例。**注意：测试套函数不支持异步函数** |

**示例：**
```javascript
import { expect, xdescribe, xit } from '@ohos/hypium';
  
  export default function skip1() {
    xdescribe('skip1', () => {
      xit('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
```

//attribute 类型不清楚
#### xit

xit(testCaseName: string,attribute: TestType | Size | Level,func: Function): void

定义一条测试用例

**参数：**

| 参数名 | 类型                 | 必填 | 说明                                                   | 
|------------|--------------------|----|------------------------------------------------------|
| testSuiteName      | string             | 是  | 指定测试套的名称 |
|      attribute              | | 是  |    过滤参数，提供Level、Size、TestType 对象，对测试用例进行标记，以区分测试用例的级别、粒度、测试类型      |
|         func                    |Function |  否  |        指定测试套函数，用于注册测试用例。                                                            |

**示例：**
```javascript
import { expect, xdescribe, xit } from '@ohos/hypium';

export default function skip1() {
   xdescribe('skip1', () => {
      //注意：在xdescribe中不支持编写it用例
      xit('assertContain1', 0, () => {
         let a = true;
         let b = true;
         expect(a).assertEqual(b);
      })
   })
}
```



| No. | API               | 功能说明                                                                   |
|-----| ----------------- |------------------------------------------------------------------------|
| 1   | describe          | 定义一个测试套，支持两个参数：测试套名称和测试套函数。其中测试套函数不能是异步函数                                   |
| 2   | beforeAll         | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数。                        |
| 3   | beforeEach        | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数。          |
| 4   | afterEach         | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数。          |
| 5   | afterAll          | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数。                        |
| 6   | beforeItSpecified | @since1.0.15在测试套内定义一个单元预置条件，仅在指定测试用例开始前执行，支持两个参数：单个用例名称或用例名称数组、预置动作函数。 |
| 7   | afterItSpecified  | @since1.0.15在测试套内定义一个单元清理条件，仅在指定测试用例结束后执行，支持两个参数：单个用例名称或用例名称数组、清理动作函数  |
| 8   | it                | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数。                                        |
| 9   | expect            | 支持bool类型判断等多种断言方法。                                                     |
| 10  | xdescribe    | @since1.0.17定义一个跳过的测试套，支持两个参数：测试套名称和测试套函数。                             |
| 11  | xit                | @since1.0.17定义一条跳过的测试用例，支持三个参数：用例名称，过滤参数和用例函数。                         |                                                                                 |

#### 断言库


#### expect

expect(actualValue?: any): Assert;

支持bool类型判断等多种断言方法

**参数：**

| 参数名  | 类型          | 必填  | 说明                                                        |
|---|---|---|-----------------------------------------------------------|
| actualValue  | any         |  否 | 期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。                   |


**返回值：**

|类型   |说明   |
|---|---|
| Assert  |断言对象，提供一系列匹配器方法用于执行具体断言操作    |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('Expect Basic Value Tests', function () {
   it('expect_equalTo', function () {
      let result = 2 + 3;
      expect(result).assertEqual(5); // 断言相等
   });

   it('expect_not_equalTo', function () {
      let name = 'Tom';
      expect(name).assertNotEqual('Jerry'); // 断言不相等
   });
});
```

```javascript
  expect(${actualvalue}).assertX(${expectvalue})
```

- 断言功能列表：

| No.  | API                              | 功能说明                                                                                       |
| :--- | :------------------------------- | ---------------------------------------------------------------------------------------------- |
| 1    | assertClose                      | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1)                                  |
| 2    | assertContain                    | 检验actualvalue中是否包含expectvalue                                                           |
| 3    | assertDeepEquals                 | @since1.0.4 检验actualvalue和expectvalue(0)是否是同一个对象                                    |
| 4    | assertEqual                      | 检验actualvalue是否等于expectvalue[0]                                                          |
| 5    | assertFail                       | 抛出一个错误                                                                                   |
| 6    | assertFalse                      | 检验actualvalue是否是false                                                                     |
| 7    | assertTrue                       | 检验actualvalue是否是true                                                                      |
| 8    | assertInstanceOf                 | 检验actualvalue是否是expectvalue类型                                                           |
| 9    | assertLarger                     | 检验actualvalue是否大于expectvalue                                                             |
| 10   | assertLess                       | 检验actualvalue是否小于expectvalue                                                             |
| 11   | assertNaN                        | @since1.0.4 检验actualvalue是否是NaN                                                           |
| 12   | assertNegUnlimited               | @since1.0.4 检验actualvalue是否等于Number.NEGATIVE_INFINITY                                    |
| 13   | assertNull                       | 检验actualvalue是否是null                                                                      |
| 14   | assertPosUnlimited               | @since1.0.4 检验actualvalue是否等于Number.POSITIVE_INFINITY                                    |
| 15   | assertPromiseIsPending           | @since1.0.4 检验actualvalue是否处于Pending状态【actualvalue为promse对象】                      |
| 16   | assertPromiseIsRejected          | @since1.0.4 检验actualvalue是否处于Rejected状态【同15】                                        |
| 17   | assertPromiseIsRejectedWith      | @since1.0.4 检验actualvalue是否处于Rejected状态，并且比较执行的结果值【同15】                  |
| 18   | assertPromiseIsRejectedWithError | @since1.0.4 检验actualvalue是否处于Rejected状态并有异常，同时比较异常的类型和message值【同15】 |
| 19   | assertPromiseIsResolved          | @since1.0.4 检验actualvalue是否处于Resolved状态【同15】                                        |
| 20   | assertPromiseIsResolvedWith      | @since1.0.4 检验actualvalue是否处于Resolved状态，并且比较执行的结果值【同15】                  |
| 21   | assertThrowError                 | 检验actualvalue抛出Error内容是否是expectValue                                                  |
| 22   | assertUndefined                  | 检验actualvalue是否是undefined                                                                 |
| 23   | not                              | @since1.0.4 断言结果取反                                                                       |
| 24   | message                | @since1.0.17自定义断言异常信息 |


#### assertClose

assertClose(expectValue: number, precision: number): void;

检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1)

**参数：**

| 参数名  | 类型          | 必填 | 说明                                                        |
|---|---|----|-----------------------------------------------------------|
| expectValue  | number         | 是  |     期望值（expected value），即预期的正确结果数值。               |
| precision             |      number       | 是   |     精度值（容差范围，delta），表示允许的最大误差。实际值只要落在 [expectValue - precision, expectValue + precision] 区间内即视为通过                |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertCloseTest', 0, () => {
      let a: number = 100;
      let b: number = 0.1;
      expect(a).assertClose(99, b);
   })
});
```

#### assertContain

assertContain(expectValue: any): void;

检验actualvalue中是否包含expectvalue。

**参数：**

| 参数名  | 类型          | 必填 | 说明                                                        |
|---|---|----|-----------------------------------------------------------|
| expectValue  | any         | 是  |    期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。              |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertContain_1', 0, () => {
      let a = "abc";
      expect(a).assertContain('b');
   })
});
```

#### assertEqual

assertEqual(expectValue: any): void;

检验actualvalue是否等于expectvalue[0]。

**参数：**

| 参数名  | 类型          | 必填 | 说明                                                        |
|---|---|----|-----------------------------------------------------------|
| expectValue  | any         | 是  |    期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。              |

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertEqualTest', 0, () => {
      expect(3).assertEqual(3);
   })
});
```

#### assertFail

assertFail(): void;

抛出一个错误。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertFailTest', 0, () => {
      expect().assertFail() // 用例失败;
   })
});
```
#### assertFalse

assertFalse(): void;

检验actualvalue是否是false。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertFalseTest', 0, () => {
      expect(false).assertFalse();
   })
});
```

#### assertTrue

assertFalse(): void;

检验actualvalue是否是true。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertTrueTest', 0, () => {
      expect(false).assertTrue();
   })
});
```

#### assertInstanceOf

assertInstanceOf(expectValue: string): void;

检验actualvalue是否是expectvalue类型，支持基础类型。

**参数：**

| 参数名  | 类型          | 必填 | 说明                       |
|---|---|----|--------------------------|
| expectValue  | string         | 是  | 期望值,参数必须是一个字符串类型（string） |

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertInstanceOfTest', 0, () => {
      let a: string = 'strTest';
      expect(a).assertInstanceOf('String');
   })
});
```

#### assertLarger

assertLarger(expectValue: number): void;

检验actualvalue是否大于expectvalue。

**参数：**

| 参数名  | 类型     | 必填 | 说明                      |
|---|--------|----|-------------------------|
| expectValue  | number | 是  | 期望值,参数必须是一个数字类型（number） |

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertLargerTest', 0, () => {
      expect(3).assertLarger(2);
   })
});
```

#### assertLess

assertLess(expectValue: number): void;

检验actualvalue是否小于expectvalue。

**参数：**

| 参数名  | 类型     | 必填 | 说明   |
|---|--------|----|------|
| expectValue  | number | 是  | 期望值,参数必须是一个数字类型（number） |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertLessTest', 0, () => {
      expect(2).assertLess(3);
   })
});
```

#### assertNull

assertNull(): void;

检验actualvalue是否是null。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertNullTest', 0, () => {
      expect(null).assertNull();
   })
});
```

#### assertThrowError

assertThrowError(expectValue: string | Function): void;


检验actualvalue抛出Error内容是否是expectValue。

**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| expectValue  | string\function | 是  | 期望值,参数必须是一个字符串类型（string）或者指定测试套函数，用于注册测试用例 |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertThrowErrorTest', 0, () => {
      expect(() => {
         throw new Error('test')
      }).assertThrowError('test');
   })
});
```

#### assertUndefined

assertUndefined(): void;

检验actualvalue是否是undefined。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertUndefinedTest', 0, () => {
      expect(undefined).assertUndefined();
   })
});
```

#### assertNaN

assertNaN(): void;

@since1.0.4 检验actualvalue是否是一个NaN。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertNaNTest', 0, () => {
      expect(Number.NaN).assertNaN(); // true
   })
});
```

#### assertNegUnlimited

assertNegUnlimited(): void;

@since1.0.4 检验actualvalue是否等于Number.NEGATIVE_INFINITY。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertNegUnlimitedTest', 0, () => {
      expect(Number.NEGATIVE_INFINITY).assertNegUnlimited(); // true
   })
});
```

#### assertPosUnlimited

assertPosUnlimited(): void;

@since1.0.4 检验actualvalue是否等于Number.POSITIVE_INFINITY。

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPosUnlimitedTest', 0, () => {
      expect(Number.POSITIVE_INFINITY).assertPosUnlimited(); // true
   })
});
```

#### assertDeepEquals

assertDeepEquals(expectValue: any): void;

@since1.0.4 检验actualvalue和expectvalue是否完全相等。

**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| expectValue  | any | 是  | 期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。  |


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('deepEquals_array_not_have_true', 0, () => {
      const a: Array<number> = [];
      const b: Array<number> = [];
      expect(a).assertDeepEquals(b);
   })
});
```

//////Promise<void>预览有问题
#### assertPromiseIsPending

assertPromiseIsPending(): Promise<void>;

@since1.0.4 判断promise是否处于Pending状态。

**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsPendingTest', 0, async () => {
      let p: Promise<void> = new Promise<void>(() => {});
      await expect(p).assertPromiseIsPending();
   })
});
```

#### assertPromiseIsRejected

assertPromiseIsRejected(): Promise<void>;

@since1.0.4 判断promise是否处于Rejected状态。


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsRejectedTest', 0, async () => {
      let info: PromiseInfo = {res: "no"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      await expect(p).assertPromiseIsRejected();
   })
});
```

#### assertPromiseIsRejectedWith

assertPromiseIsRejectedWith(expectValue?: any): Promise<void>;

@since1.0.4 判断promise是否处于Rejected状态，并且比较执行的结果值。


**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| expectValue  | any | 否  | 期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。  |


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsRejectedWithTest', 0, async () => {
      let info: PromiseInfo = {res: "reject value"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      await expect(p).assertPromiseIsRejectedWith(info);
   })
});
```



#### assertPromiseIsRejectedWithError

assertPromiseIsRejectedWithError(expectValue?: any): Promise<void>;

@since1.0.4 判断promise是否处于Rejected状态并有异常，同时比较异常的类型和message值。

**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| expectValue  | any | 否  | 期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。  |


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsRejectedWithErrorTest', 0, async () => {
      let p1: Promise<TypeError> = Promise.reject(new TypeError('number'));
      await expect(p1).assertPromiseIsRejectedWithError(TypeError);
   })
});
```

#### assertPromiseIsResolved

assertPromiseIsResolved(): Promise<void>;

@since1.0.4 判断promise是否处于Resolved状态。


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsResolvedTest', 0, async () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      await expect(p).assertPromiseIsResolved();
   })
});
```



#### assertPromiseIsResolvedWith

assertPromiseIsResolvedWith(expectValue?: any): Promise<void>;

@since1.0.4 判断promise是否处于Resolved状态，并且比较执行的结果值。

**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| expectValue  | any | 否  | 期望值，即待验证的表达式或变量的值。可为任意类型，包括基本类型、对象、函数等。  |


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Promise<void>	 | Promise对象，返回无结果的Promise对象。|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('expectTest', ()=> {
   it('assertPromiseIsResolvedWithTest', 0, async () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      await expect(p).assertPromiseIsResolvedWith(info);
   })
});
```

#### not

not(): Assert;

@since1.0.4 断言取反,支持上面所有的断言功能。


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Assert	 | 断言对象，提供一系列匹配器方法用于执行具体断言操作|


**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('assertNot', ()=> {
   it('assertNot001', 0, () => {
      expect(1).not().assertLargerOrEqual(1)
   })
});
```

#### message

message(msg: string): Assert;

@since1.0.17自定义断言异常信息。

**参数：**

| 参数名  | 类型              | 必填 | 说明                                         |
|---|-----------------|----|--------------------------------------------|
| msg  | string | 是  | 设置断言失败时的自定义错误消息  |


**返回值：**

| 类型             | 说明                                        |
|----------------|-------------------------------------------|
| Assert	 | 断言对象，提供一系列匹配器方法用于执行具体断言操作|

**示例：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

describe('assertMessage', () => {

   it('assertMessage001', 0, () => {
      let actualValue = 0
      let expectValue = 2
      expect(actualValue).message('0 != 2').assertEqual(expectValue);
   })
})
```
#### 公共系统能力

| No.  | API                                                     | 功能描述                                                     |
| ---- | ------------------------------------------------------- | ------------------------------------------------------------ |
| 1    | existKeyword | @since1.0.3 hilog日志中查找指定字段是否存在，keyword是待查找关键字，timeout为设置的查找时间 |
| 2    | actionStart                         | @since1.0.3 cmd窗口输出开始tag                               |
| 3    | actionEnd                           | @since1.0.3 cmd窗口输出结束tag                               |

#### existKeyword

existKeyword(keyword: string, timeout: number): boolean

检测hilog日志中是否打印，仅支持检测单行日志。

**参数：**

| 参数名  | 类型     | 必填 | 说明                                       |
|---|--------|----|------------------------------------------|
| keyword  | string | 是  | 待查找关键字  |
| timeout     | number | 否  |  设置的查找时间     |

**返回值：**

| 类型             | 说明                  |
|----------------|---------------------|
| boolean	 | 查找指定字段是否存在，true:存在;false:不存在 |


示例：
```javascript
import { describe, it, expect, SysTestKit} from '@ohos/hypium';

export default function existKeywordTest() {
    describe('existKeywordTest', function () {
        it('existKeyword',DEFAULT, async function () {
            console.info("HelloTest");
            let isExist = await SysTestKit.existKeyword('HelloTest');
            console.info('isExist ------>' + isExist);
        })
    })
}
```


#### actionStart

actionStart(tag: string): void

添加用例执行过程打印自定义日志

**参数：**

| 参数名  | 类型     | 必填 | 说明      |
|---|--------|----|---------|
| tag  | string | 是  | 自定义日志信息 |

#### actionEnd

actionEnd(tag: string): void

添加用例执行过程打印自定义日志。

**参数：**

| 参数名  | 类型     | 必填 | 说明      |
|---|--------|----|---------|
| tag  | string | 是  | 自定义日志信息 |

actionStart，actionEnd示例：
```javascript
import { describe, it, expect, SysTestKit} from '@ohos/hypium';

export default function actionTest() {
    describe('actionTest', function () {
        it('existKeyword',DEFAULT, async function () {
            let tag = '[MyTest]';
			SysTestKit.actionStart(tag);
            //do something
            SysTestKit.actionEnd(tag);
        })
    })
}
```

#### 专项能力

- 测试用例属性筛选能力：hypium支持根据用例属性筛选执行指定测试用例，使用方式是先在测试用例上标记用例属性后，再在测试应用的启动shell命令后新增" -s ${Key} ${Value}"。

| Key      | 含义说明     | Value取值范围                                                                                                                                          |
| -------- | ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| level    | 用例级别     | "0","1","2","3","4", 例如：-s level 1                                                                                                                  |
| size     | 用例粒度     | "small","medium","large", 例如：-s size small                                                                                                          |
| testType | 用例测试类型 | "function","performance","power","reliability","security","global","compatibility","user","standard","safety","resilience", 例如：-s testType function |

示例代码

```javascript
import { describe, it, expect, TestType, Size, Level } from '@ohos/hypium';

export default function attributeTest() {
    describe('attributeTest', function () {
        it("testAttributeIt", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, function () {
            console.info('Hello Test');
        })
    })
}
```

示例命令
```shell  
XX -s level 1 -s size small -s testType function
```
该命令的作用是：筛选测试应用中同时满足a）用例级别是1 b）用例粒度是small c）用例测试类型是function 三个条件的用例执行。

- 测试套/测试用例名称筛选能力（测试套与用例名称用“#”号连接，多个用“,”英文逗号分隔）

| Key      | 含义说明                | Value取值范围                                                                                |
| -------- | ----------------------- | -------------------------------------------------------------------------------------------- |
| class    | 指定要执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s class attributeTest#testAttributeIt    |
| notClass | 指定不执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s notClass attributeTest#testAttributeIt |

示例命令
```shell  
XX -s class attributeTest#testAttributeIt,abilityTest#testAbilityIt
```
该命令的作用是：筛选测试应用中attributeTest测试套下的testAttributeIt测试用例，abilityTest测试套下的testAbilityIt测试用例，只执行这两条用例。

- 其他能力

| 能力项       | Key     | 含义说明                     | Value取值范围                                  |
| ------------ | ------- | ---------------------------- | ---------------------------------------------- |
| 随机执行能力 | random  | 测试套&测试用例随机执行      | true, 不传参默认为false， 例如：-s random true |
| 空跑能力     | dryRun  | 显示要执行的测试用例信息全集 | true , 不传参默认为false，例如：-s dryRun true |
| 异步超时能力 | timeout | 异步用例执行的超时时间       | 正整数 , 单位ms，例如：-s timeout 5000         |

##### 约束限制
随机执行能力和空跑能力从npm包1.0.3版本开始支持

#### Mock能力

##### 约束限制

单元测试框架Mock能力从npm包[1.0.1版本](https://repo.harmonyos.com/#/cn/application/atomService/@ohos%2Fhypium/v/1.0.1)开始支持

## 约束

***
    本模块首批接口从OpenHarmony SDK API version 8开始支持。

## Hypium开放能力隐私声明

-  我们如何收集和使用您的个人信息
   您在使用集成了Hypium开放能力的测试应用时，Hypium不会处理您的个人信息。
-  SDK处理的个人信息
   不涉及。
-  SDK集成第三方服务声明
   不涉及。
-  SDK数据安全保护
   不涉及。
-  SDK版本更新声明
   为了向您提供最新的服务，我们会不时更新Hypium版本。我们强烈建议开发者集成使用最新版本的Hypium。

