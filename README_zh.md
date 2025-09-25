# 自动化测试框架使用介绍

## 简介
 OpenHarmony自动化测试框架代码部件仓arkxtest，包含单元测试框架(JsUnit)、Ui测试框架(UiTest)和白盒性能测试框架（PerfTest）。

 单元测试框架(JsUnit)提供单元测试用例执行能力，提供用例编写基础接口，生成对应报告，用于测试系统或应用接口。

 Ui测试框架(UiTest)通过简洁易用的API提供查找和操作界面控件能力，支持用户开发基于界面操作的自动化测试脚本。

 PerfTest提供基于代码段的白盒性能测试能力，支持采集指定代码段执行期间或指定场景发生时的性能数据。

## 目录

```
arkxtest 
  |-----jsunit  单元测试框架
  |-----UiTest  Ui测试框架
  |-----perftest  白盒性能测试框架
```
## 约束限制
本模块首批接口从API version 8开始支持。后续版本的新增接口，采用上角标单独标记接口的起始版本。

## 单元测试框架功能特性

| No.  | 特性     | 功能说明                                               |
| ---- | -------- |----------------------------------------------------|
| 1    | 基础流程 | 支持编写及异步执行基础用例。                                     |
| 2    | 断言库   | 判断用例实际结果值与预期值是否相符。                                 |
| 3    | Mock能力 | 支持函数级mock能力，对定义的函数进行mock后修改函数的行为，使其返回指定的值或者执行某种动作。 |
| 4    | 数据驱动 | 提供数据驱动能力，支持复用同一个测试脚本，使用不同输入数据驱动执行。                 |
| 5    | 专项能力 | 支持测试套与用例筛选、随机执行、压力测试、超时设置、遇错即停模式，跳过，支持测试套嵌套等。      |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe代表一个测试套， it代表一条用例。

| No. | API               | 功能说明                                                                   |
|-----| ----------------- |------------------------------------------------------------------------|
| 1   | describe          | 定义一个测试套，支持两个参数：测试套名称和测试套函数。其中测试套函数不能是异步函数。                             |
| 2   | beforeAll         | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数。                        |
| 3   | beforeEach        | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数。          |
| 4 | beforeEachIt | @since1.0.25 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，支持一个参数：预置动作函数。<br />外层测试套定义的beforeEachIt会在内部测试套中的测试用例执行前执行。 |
| 5   | afterEach         | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数。          |
| 6 | afterEachIt | @since1.0.25 在测试套内定义一个单元预置条件，在每条测试用例结束后执行，支持一个参数：预置动作函数。<br />外层测试套定义的afterEachIt会在内部测试套中的测试用例执行结束后执行。 |
| 7   | afterAll          | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数。                        |
| 8   | beforeItSpecified | @since1.0.15在测试套内定义一个单元预置条件，仅在指定测试用例开始前执行，支持两个参数：单个用例名称或用例名称数组、预置动作函数。 |
| 9   | afterItSpecified  | @since1.0.15在测试套内定义一个单元清理条件，仅在指定测试用例结束后执行，支持两个参数：单个用例名称或用例名称数组、清理动作函数。 |
| 10  | it                | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数。                                        |
| 11  | expect            | 支持bool类型判断等多种断言方法。                                                     |
| 12 | xdescribe    | @since1.0.17定义一个跳过的测试套，支持两个参数：测试套名称和测试套函数。                             |
| 13 | xit                | @since1.0.17定义一条跳过的测试用例，支持三个参数：用例名称，过滤参数和用例函数。                         |


beforeItSpecified, afterItSpecified 示例代码：

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

beforeEachIt, afterEachIt 示例代码：

```javascript
import { describe, beforeEach, afterEach, beforeEachIt, afterEachIt, it, expect } from '@ohos/hypium';
let str = "";
export default function test() {
  describe('test0', () => {
    beforeEach(async () => {
      str += "A"
    })
    beforeEachIt(async () => {
      str += "B"
    })
    afterEach(async () => {
      str += "C"
    })
    afterEachIt(async () => {
      str += "D"
    })
    it('test0000', 0, () => {
      expect(str).assertEqual("BA");
    })
    describe('test1', () => {
      beforeEach(async () => {
        str += "E"
      })
      beforeEachIt(async () => {
        str += "F"
      })
      it('test1111', TestType.FUNCTION | Size.MEDIUMTEST | Level.LEVEL3, async () => {
        expect(str).assertEqual("BACDBFE");
      })
    })
  })
}
```
####  断言库

##### 断言功能列表


| No.  | API                | 功能说明                                                        |
| :--- | :------------------|-------------------------------------------------------------|
| 1    | assertClose        | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1)。         |
| 2    | assertContain      | 检验actualvalue中是否包含expectvalue。                              |
| 3    | assertEqual        | 检验actualvalue是否等于expectvalue[0]。                            |
| 4    | assertFail         | 抛出一个错误。                                                     |
| 5    | assertFalse        | 检验actualvalue是否是false。                                      |
| 6    | assertTrue         | 检验actualvalue是否是true。                                       |
| 7    | assertInstanceOf   | 检验actualvalue是否是expectvalue类型，支持基础类型。                       |
| 8    | assertLarger       | 检验actualvalue是否大于expectvalue。                               |
| 9    | assertLess         | 检验actualvalue是否小于expectvalue。                               |
| 10   | assertNull         | 检验actualvalue是否是null。                                       |
| 11   | assertThrowError   | 检验actualvalue抛出Error内容是否是expectValue。                       |
| 12   | assertUndefined    | 检验actualvalue是否是undefined。                                  |
| 13   | assertNaN          | @since1.0.4 检验actualvalue是否是一个NaN。                          |
| 14   | assertNegUnlimited | @since1.0.4 检验actualvalue是否等于Number.NEGATIVE_INFINITY。      |
| 15   | assertPosUnlimited | @since1.0.4 检验actualvalue是否等于Number.POSITIVE_INFINITY。      |
| 16   | assertDeepEquals   | @since1.0.4 检验actualvalue和expectvalue是否完全相等。                |
| 17   | assertPromiseIsPending | @since1.0.4 判断promise是否处于Pending状态。                         |
| 18   | assertPromiseIsRejected | @since1.0.4 判断promise是否处于Rejected状态。                        |
| 19   | assertPromiseIsRejectedWith | @since1.0.4 判断promise是否处于Rejected状态，并且比较执行的结果值。             |
| 20   | assertPromiseIsRejectedWithError | @since1.0.4 判断promise是否处于Rejected状态并有异常，同时比较异常的类型和message值。 |
| 21   | assertPromiseIsResolved | @since1.0.4 判断promise是否处于Resolved状态。                        |
| 22   | assertPromiseIsResolvedWith | @since1.0.4 判断promise是否处于Resolved状态，并且比较执行的结果值。             |
| 23   | not                | @since1.0.4 断言取反,支持上面所有的断言功能。                               |
| 24   | message                | @since1.0.17自定义断言异常信息。                                      |

expect断言示例代码：

```javascript
import { describe, it, expect } from '@ohos/hypium';

export default function expectTest() {
  describe('expectTest', () => {
    it('assertBeClose_success', 0, () => {
      let a: number = 100;
      let b: number = 0.1;
      expect(a).assertClose(99, b);
    })
    it('assertInstanceOf_success', 0, () => {
      let a: string = 'strTest';
      expect(a).assertInstanceOf('String');
    })
    it('assertNaN_success', 0, () => {
      expect(Number.NaN).assertNaN(); // true
    })
    it('assertNegUnlimited_success', 0, () => {
      expect(Number.NEGATIVE_INFINITY).assertNegUnlimited(); // true
    })
    it('assertPosUnlimited_success', 0, () => {
      expect(Number.POSITIVE_INFINITY).assertPosUnlimited(); // true
    })
    it('not_number_true', 0, () => {
      expect(1).not().assertLargerOrEqual(2);
    })
    it('not_number_true_1', 0, () => {
      expect(3).not().assertLessOrEqual(2);
    })
    it('not_NaN_true', 0, () => {
      expect(3).not().assertNaN();
    })
    it('not_contain_true', 0, () => {
      let a: string = "abc";
      let b: string = "cdf";
      expect(a).not().assertContain(b);
    })
    it('not_large_true', 0, () => {
      expect(3).not().assertLarger(4);
    })
    it('not_less_true', 0, () => {
      expect(3).not().assertLess(2);
    })
    it('not_undefined_true', 0, () => {
      expect(3).not().assertUndefined();
    })
    it('deepEquals_null_true', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      expect(null).assertDeepEquals(null);
    })
    it('deepEquals_array_not_have_true', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const a: Array<number> = [];
      const b: Array<number> = [];
      expect(a).assertDeepEquals(b);
    })
    it('deepEquals_map_equal_length_success', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const a: Map<number, number> = new Map();
      const b: Map<number, number> = new Map();
      a.set(1, 100);
      a.set(2, 200);
      b.set(1, 100);
      b.set(2, 200);
      expect(a).assertDeepEquals(b);
    })
    it("deepEquals_obj_success_1", 0, () => {
      const a: SampleTest = {x: 1};
      const b: SampleTest = {x: 1};
      expect(a).assertDeepEquals(b);
    })
    it("deepEquals_regExp_success_0", 0, () => {
      const a: RegExp = new RegExp("/test/");
      const b: RegExp = new RegExp("/test/");
      expect(a).assertDeepEquals(b);
    })
    it('test_isPending_pass_1', 0, () => {
      let p: Promise<void> = new Promise<void>(() => {});
      expect(p).assertPromiseIsPending();
    })
    it('test_isRejected_pass_1', 0, () => {
      let info: PromiseInfo = {res: "no"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      expect(p).assertPromiseIsRejected();
    })
    it('test_isRejectedWith_pass_1', 0, () => {
      let info: PromiseInfo = {res: "reject value"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      expect(p).assertPromiseIsRejectedWith(info);
    })
    it('test_isRejectedWithError_pass_1', 0, () => {
      let p1: Promise<TypeError> = Promise.reject(new TypeError('number'));
      expect(p1).assertPromiseIsRejectedWithError(TypeError);
    })
    it('test_isResolved_pass_1', 0, () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      expect(p).assertPromiseIsResolved();
    })
    it('test_isResolvedTo_pass_1', 0, () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      expect(p).assertPromiseIsResolvedWith(info);
    })
    it("test_message", 0, () => {
      expect(1).message('1 is not equal 2!').assertEqual(2); // fail
    })
  })
}

interface SampleTest {
  x: number;
}

interface PromiseInfo {
  res: string;
}
```

##### 自定义断言@since1.0.18

示例代码：

```javascript
import { describe, Assert, beforeAll, expect, Hypium, it } from '@ohos/hypium';

// custom.ets
interface customAssert extends Assert {
  // 自定义断言声明
  myAssertEqual(expectValue: boolean): void;
}

//自定义断言实现
let myAssertEqual = (actualValue: boolean, expectValue: boolean) => {
  interface R {
  pass: boolean,
  message: string
}

let result: R = {
  pass: true,
  message: 'just is a msg'
}

let compare = () => {
  if (expectValue === actualValue) {
    result.pass = true;
    result.message = '';
  } else {
    result.pass = false;
    result.message = 'expectValue !== actualValue!';
  }
  return result;
}
result = compare();
return result;
}

export default function customAssertTest() {
  describe('customAssertTest', () => {
    beforeAll(() => {
      //注册自定义断言，只有先注册才可以使用
      Hypium.registerAssert(myAssertEqual);
    })
    it('assertContain1', 0, () => {
      let a = true;
      let b = true;
      (expect(a) as customAssert).myAssertEqual(b);
      Hypium.unregisterAssert(myAssertEqual);
    })
    it('assertContain2', 0, () => {
      Hypium.registerAssert(myAssertEqual);
      let a = true;
      let b = true;
      (expect(a) as customAssert).myAssertEqual(b);
      // 注销自定义断言，注销以后就无法使用
      Hypium.unregisterAssert(myAssertEqual);
      try {
        (expect(a) as customAssert).myAssertEqual(b);
      }catch(e) {
        expect(e.message).assertEqual("myAssertEqual is unregistered");
      }
    })
  })
}
```

#### 异步代码测试

 **异步测试错误示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

async function callBack(fn: Function) {
   setTimeout(fn, 5000, 'done')
}

export default function callBackErrorTest() {
  describe('callBackErrorTest', () => {
    it('callBackErrorTest', 0, () => {
      callBack((result: string) => {
        try {
          // 用例失败
          expect().assertFail();
        } catch (e) {
        } finally {
        }
      })
    })
  })
}
```
> - 上述测试用例中,测试函数结束后, 回调函数才执行, 导致用例结果错误。
> - 当使用框架测试异步代码时,框架需要知道它测试的代码何时完成。以确保测试用例结果正常统计,测试框架用以下两种方式来处理这个问题。

##### Async/Await
> 使用 Async关键字定义一个异步测试函数,在测试函数中使用await等待测试函数完成。

**Promise示例代码：**

```javascript
import { describe, it, expect } from '@ohos/hypium';

async function method_1() {
  return new Promise<string>((res: Function, rej: Function) => {
    //做一些异步操作
    setTimeout(() => {
      console.log('执行');
      res('method_1_call');
    }, 5000);
  });
}

export default function abilityTest() {
  describe('ActsAbilityTest', () => {
    it('assertContain', 0, async () => {
      let result = await method_1();
      expect(result).assertEqual('method_1_call');
    })
  })
}
```
##### done 函数
>  - done函数是测试函数的一个可选回调参数,在测试用例中手动调用,测试框架将等待done回调被调用,然后才完成测试。
>  - 当测试函数中定义done函数参数时,测试用例中必须手动调用done函数,否则用例失败会出现超时错误。


**Promise回调示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';
async function method_1() {
  return new Promise<string>((res: Function, rej: Function) => {
    //做一些异步操作
    setTimeout(() => {
      console.log('执行');
      res('method_1_call');
    }, 5000);
  });
}

export default function abilityTest() {
  describe('Promise_Done', () => {
    it('Promise_Done', 0,(done: Function) => {
      method_1().then((result: string) => {
        try {
          expect(result).assertEqual('method_1_call');
        } catch (e) {
        } finally {
          // 调用done函数,用例执行完成,必须手动调用done函数,否则出现超时错误。
          done()
        }
      })

    })
  })
}
```

**回调函数示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

async function callBack(fn: Function) {
   setTimeout(fn, 5000, 'done')
}

export default function callBackTestTest() {
  describe('CallBackTest', () => {
    it('CallBackTest_001', 0, (done: Function) => {
      callBack( (result: string) => {
        try {
          expect(result).assertEqual('done');
        } catch (e) {
        } finally {
          // 调用done函数,用例执行完成,必须手动调用done函数,否则出现超时错误。
          done()
        }
      })
    })
  })
}
```

#### SysTestKit的公共能力

| No. | API                | 功能说明              |
|:----| :------------------|-------------------|
| 1   | getDescribeName        | 获取当前测试用例所属的测试套名称。 |
| 2   | getItName        | 获取当初测试用例名称。       |
| 3   | getItAttribute        | 获取当初测试用例等级。       |
| 4   | actionStart        | 添加用例执行过程打印自定义日志。  |
| 5   | actionEnd        | 添加用例执行过程打印自定义日志。  |
| 6   | existKeyword        | 检测hilog日志中是否打印，仅支持检测单行日志。 |
| 7 | clearLog | 清理被测样机的hilog缓存。 |

##### 获取当前测试用例所属的测试套名称

示例代码：
```javascript
import { describe, it, expect, SysTestKit } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetDescribeName", 0, () => {
      hilog.debug(domain, tag, `testGetDescribeName start`);
      const describeName = SysTestKit.getDescribeName();
      hilog.debug(domain, tag, `testGetDescribeName describeName, ${describeName}`);
      expect(describeName).assertEqual('SysTestKitTest');
      hilog.debug(domain, tag, `testGetDescribeName end`);
    })
  })
}
```

##### 获取当初测试用例名称

示例代码：
```javascript
import { describe, it, expect, SysTestKit } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetItName", 0, () => {
      hilog.debug(domain, tag, `testGetDescribeName start`);
      const itName = SysTestKit.getItName();
      hilog.debug(domain, tag, `testGetDescribeName itName, ${itName}`);
      expect(itName).assertEqual('testGetItName');
      hilog.debug(domain, tag, `testGetDescribeName end`);
    })
  })
}
```

##### 获取当初测试用例级别

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetItAttribute", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      hilog.debug(domain, tag, `testGetItAttribute start`);
      const testType: TestType | Size | Level = SysTestKit.getItAttribute();
      hilog.debug(domain, tag, `testGetDescribeName testType, ${ testType }`);
      expect(testType).assertEqual(TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0);
      hilog.debug(domain, tag, `testGetItAttribute end`);
    })
  })
}
```

##### 添加自定义打印日志。

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testActionStart", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      hilog.debug(domain, tag, `testActionStart start `);
      SysTestKit.actionStart('testActionStart 自定义日志 ');
      SysTestKit.actionEnd('testActionStart end 自定义日志 ');
      hilog.debug(domain, tag, `testActionStart end`);
    })
  })
}
```

##### 检测hilog日志中是否打印。

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'

function logTest() {
  hilog.info(domain, 'test', `logTest called selfTest`);
}

export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testExistKeyword", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, async () => {
      await SysTestKit.clearLog();
      hilog.debug(domain, tag, `testExistKeyword start `);
      logTest();
      const isCalled = await SysTestKit.existKeyword('logTest');
      hilog.debug(domain, tag, `testExistKeyword isCalled, ${isCalled} `);
      expect(isCalled).assertTrue();
      hilog.debug(domain, tag, `testExistKeyword end`);
    })
  })
}
```

#### Mock能力

##### 约束限制

单元测试框架Mock能力从npm包[1.0.1版本](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)开始支持，需修改源码工程中package.info中配置依赖npm包版本号后使用。
> -  仅支持mock自定义对象,不支持mock系统API对象。
> -  不支持mock对象的私有函数。 
-  **接口列表：**

| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | mockFunc(obj: object, f：function()) | mock某个类的对象obj的函数f，那么需要传两个参数：obj和f，支持使用异步函数（说明：对mock而言原函数实现是同步或异步没太多区别，因为mock并不关注原函数的实现）。                                                        |
| 2 | when(mockedfunc：function) | 对传入后方法做检查，检查是否被mock并标记过，返回的是一个方法声明。                                                                                                             |
| 3 | afterReturn(x：value) | 设定预期返回一个自定义的值value，比如某个字符串或者一个promise。                                                                                                          |
| 4 | afterReturnNothing() | 设定预期没有返回值，即 undefined。                                                                                                                          |
| 5 | afterAction(x：action) | 设定预期返回一个函数执行的操作。                                                                                                                                |
| 6 | afterThrow(x：msg) | 设定预期抛出异常，并指定异常msg。                                                                                                                              |
| 7 | clear(obj: object) | 用例执行完毕后，进行数据mocker对象的还原处理（还原之后对象恢复被mock之前的功能）。                                                                                                  |
| 8 | any | 设定用户传任何类型参数（undefined和null除外），执行的结果都是预期的值，使用ArgumentMatchers.any方式调用。                                                                           |
| 9 | anyString | 设定用户传任何字符串参数，执行的结果都是预期的值，使用ArgumentMatchers.anyString方式调用。                                                                                      |
| 10 | anyBoolean | 设定用户传任何boolean类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyBoolean方式调用。                                                                               |
| 11 | anyFunction | 设定用户传任何function类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyFunction方式调用。                                                                             |
| 12 | anyNumber | 设定用户传任何数字类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyNumber方式调用。                                                                                     |
| 13 | anyObj | 设定用户传任何对象类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyObj方式调用。                                                                                        |
| 14 | matchRegexs(Regex) | 设定用户传任何正则表达式类型参数Regex，执行的结果都是预期的值，使用ArgumentMatchers.matchRegexs(Regex)方式调用。                                                                    |
| 15 | verify(methodName, argsArray) | 验证methodName（函数名字符串）所对应的函数和其参数列表argsArray的执行行为是否符合预期，返回一个VerificationMode：一个提供验证模式的类，它有times(count)、once()、atLeast(x)、atMost(x)、never()等函数可供选择。 |
| 16 | times(count) | 验证行为调用过count次。                                                                                                                                  |
| 17 | once() | 验证行为调用过一次。                                                                                                                                      |
| 18 | atLeast(count) | 验证行为至少调用过count次。                                                                                                                                |
| 19 | atMost(count) | 验证行为至多调用过count次。                                                                                                                                |
| 20 | never | 验证行为从未发生过。                                                                                                                                      |
| 21 | ignoreMock(obj, method) | 使用ignoreMock可以还原obj对象中被mock后的函数，对被mock后的函数有效。                                                                                                   |
| 22 | clearAll() | 用例执行完毕后，进行数据和内存清理,不会还原obj对象中被mock后的函数。                                                                                                                  |                                                                                                                            |

-  **使用示例：**

用户可以通过以下方式引入mock模块进行测试用例编写：

- **须知：**
使用时候必须引入的mock能力模块： MockKit，when，根据自己用例需要引入断言能力api。
例如：`import { describe, expect, it, MockKit, when} from '@ohos/hypium'`

**示例1: afterReturn 的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function afterReturnTest() {
  describe('afterReturnTest', () => {
    it('afterReturnTest', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以'test'为入参时调用函数返回结果'1'
      when(mockfunc)('test').afterReturn('1');
      // 5.对mock后的函数进行断言，看是否符合预期
      // 执行成功案例，参数为'test'
      expect(claser.method_1('test')).assertEqual('1'); // 执行通过
    })
  })
}
```
- **须知：**
`when(mockfunc)('test').afterReturn('1');`  
这句代码中的`('test')`是mock后的函数需要传递的匹配参数，目前支持传递多个参数。  
`afterReturn('1')`是用户需要预期返回的结果。  
有且只有在参数是`('test')`的时候，执行的结果才是用户自定义的预期结果。

**示例2: afterReturnNothing 的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function  afterReturnNothingTest() {
  describe('afterReturnNothingTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以'test'为入参时调用函数返回结果undefined
      when(mockfunc)('test').afterReturnNothing();
      // 5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功案例，参数为'test'，这时候执行原对象claser.method_1的方法，会发生变化
      // 这时候执行的claser.method_1不会再返回'888888'，而是设定的afterReturnNothing()生效// 不返回任何值;
      expect(claser.method_1('test')).assertUndefined(); // 执行通过
    })
  })
}
```

**示例3: 设定参数类型为any ，即接受任何参数（undefined和null除外）的使用**


- **须知：**
需要引入ArgumentMatchers类，即参数匹配器，例如：ArgumentMatchers.any

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function argumentMatchersAnyTest() {
  describe('argumentMatchersAnyTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以任何参数调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.any).afterReturn('1');
      // 5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例1，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
      // 执行成功的案例2，传参为数字类型123
      expect(claser.method_1("123")).assertEqual('1');// 用例执行通过。
      // 执行成功的案例3，传参为boolean类型true
      expect(claser.method_1("true")).assertEqual('1');// 用例执行通过。
    })
  })
}
```

**示例4: 设定参数类型ArgumentMatchers的使用**

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function argumentMatchersTest() {
  describe('argumentMatchersTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以任何string类型为参数调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.anyString).afterReturn('1');
      // 4.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
      expect(claser.method_1('abc')).assertEqual('1'); // 用例执行通过。
    })
  })
}
```

**示例5: 设定参数类型为matchRegexs（Regex）等 的使用**
```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function matchRegexsTest() {
  describe('matchRegexsTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      let claser: ClassName = new ClassName();
      // 2.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 3.期望claser.method_1函数被mock后, 以"test"为入参调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.matchRegexs(new RegExp("test"))).afterReturn('1');
      // 4.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
    })
  })
}
```

**示例6: 验证功能 Verify函数的使用**
```javascript
import { describe, it, MockKit } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }

  method_2(...arg: string[]) {
    return '999999';
  }
}
export default function verifyTest() {
  describe('verifyTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
      mocker.mockFunc(claser, claser.method_1);
      mocker.mockFunc(claser, claser.method_2);
      // 4.方法调用如下
      claser.method_1('abc', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1();
      claser.method_2('111');
      claser.method_2('111', '222');
      // 5.现在对mock后的两个函数进行验证，验证method_2,参数为'111'执行过一次
      mocker.verify('method_2',['111']).once(); // 执行success
    })
  })
}
```

**示例7: ignoreMock(obj, method) 忽略函数的使用**
```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: number[]) {
    return '888888';
  }

  method_2(...arg: number[]) {
    return '999999';
  }
}
export default function ignoreMockTest() {
  describe('ignoreMockTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      let func_2: Function = mocker.mockFunc(claser, claser.method_2);
      // 4.期望claser.method_1函数被mock后, 以number类型为入参时调用函数返回结果'4'
      when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
      // 4.期望claser.method_2函数被mock后, 以number类型为入参时调用函数返回结果'5'
      when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');
      // 5.方法调用如下
      expect(claser.method_1(123)).assertEqual("4");
      expect(claser.method_2(456)).assertEqual("5");
      // 6.现在对mock后的两个函数的其中一个函数method_1进行忽略处理（原理是就是还原）
      mocker.ignoreMock(claser, claser.method_1);
      // 7.然后再去调用 claser.method_1函数，用断言测试結果
      expect(claser.method_1(123)).assertEqual('888888');
    })
  })
}
```

**示例8: clear(obj)函数的使用**

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: number[]) {
    return '888888';
  }

  method_2(...arg: number[]) {
    return '999999';
  }
}
export default function clearTest() {
  describe('clearTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      let func_2: Function = mocker.mockFunc(claser, claser.method_2);
      // 4.期望claser.method_1函数被mock后, 以任何number类型为参数调用函数时返回结果'4'
      when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
      // 4.期望claser.method_2函数被mock后, 以任何number类型为参数调用函数时返回结果'5'
      when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');
      // 5.方法调用如下
      expect(claser.method_1(123)).assertEqual('4');
      expect(claser.method_2(123)).assertEqual('5');
      // 6.清除obj上所有的mock能力（原理是就是还原）
      mocker.clear(claser);
      // 7.然后再去调用 claser.method_1,claser.method_2 函数，测试结果
      expect(claser.method_1(123)).assertEqual('888888');
      expect(claser.method_2(123)).assertEqual('999999');
    })
  })
}
```

**示例9: afterThrow(msg)函数的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }
}
export default function afterThrowTest() {
  describe('afterThrowTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以'test'为参数调用函数时抛出error xxx异常
      when(mockfunc)('test').afterThrow('error xxx');
      // 5.执行mock后的函数，捕捉异常并使用assertEqual对比msg否符合预期
      try {
        claser.method_1('test');
      } catch (e) {
        expect(e).assertEqual('error xxx'); // 执行通过
      }
    })
  })
}
```

**示例10： mock异步返回promise对象的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  async method_1(arg: string) {
    return new Promise<string>((resolve: Function, reject: Function) => {
      setTimeout(() => {
        console.log('执行');
        resolve('数据传递');
      }, 2000);
    });
  }
}
export default function mockPromiseTest() {
  describe('mockPromiseTest', () => {
    it('testMockfunc', 0, async (done: Function) => {
      console.info("it1 begin");
      // 1.创建一个mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被mock后, 以'test'为参数调用函数时返回一个promise对象
      when(mockfunc)('test').afterReturn(new Promise<string>((resolve: Function, reject: Function) => {
        console.log("do something");
        resolve('success something');
      }));
      // 5.执行mock后的函数，即对定义的promise进行后续执行
      let result = await claser.method_1('test');
      expect(result).assertEqual("success something");
      done();
    })
  })
}
```

**示例11：verify times函数的使用（验证函数调用次数）**

```javascript
import { describe, it, MockKit, when } from '@ohos/hypium'

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }
}
export default function verifyTimesTest() {
  describe('verifyTimesTest', () => {
    it('test_verify_times', 0, () => {
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.创建类对象
      let claser: ClassName = new ClassName();
      // 3.mock 类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望被mock后的函数返回结果'4'
      when(func_1)('123').afterReturn('4');
      // 5.随机执行几次函数，参数如下
      claser.method_1('123', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1('abc');
      claser.method_1();
      // 6.验证函数method_1且参数为'abc'时，执行过的次数是否为2
      mocker.verify('method_1', ['abc']).times(2);
    })
  })
}
```


**示例12：verify atLeast函数的使用(验证函数调用次数)**

```javascript
import { describe, it, MockKit, when } from '@ohos/hypium'

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }
}
export default function verifyAtLeastTest() {
  describe('verifyAtLeastTest', () => {
    it('test_verify_atLeast', 0, () => {
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.创建类对象
      let claser: ClassName = new ClassName();
      // 3.mock  类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望被mock后的函数返回结果'4'
      when(func_1)('123').afterReturn('4');
      // 5.随机执行几次函数，参数如下
      claser.method_1('123', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1();
      // 6.验证函数method_1且参数为空时，是否至少执行过2次
      mocker.verify('method_1', []).atLeast(2);
    })
  })
}
```

**示例13：mock静态函数**
> @since1.0.16 支持

```javascript
import { describe, it, expect, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  static method_1() {
    return 'ClassName_method_1_call';
  }
}

export default function staticTest() {
  describe('staticTest', () => {
    it('staticTest_001', 0, () => {
      let really_result = ClassName.method_1();
      expect(really_result).assertEqual('ClassName_method_1_call');
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.mock  类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(ClassName, ClassName.method_1);
      // 3.期望被mock后的函数返回结果'mock_data'
      when(func_1)(ArgumentMatchers.any).afterReturn('mock_data');
      let mock_result = ClassName.method_1();
      expect(mock_result).assertEqual('mock_data');
      // 清除mock能力
      mocker.clear(ClassName);
      let really_result1 = ClassName.method_1();
      expect(really_result1).assertEqual('ClassName_method_1_call');
    })
  })
}
```

#### 数据驱动

##### 约束限制

单元测试框架数据驱动能力从[框架 1.0.2版本](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)开始支持。

- 数据参数传递 : 为指定测试套、测试用例传递测试输入数据参数。
- 压力测试 : 为指定测试套、测试用例设置执行次数。

数据驱动可以根据配置参数来驱动测试用例的执行次数和每一次传入的参数，使用时依赖data.json配置文件，文件内容如下：

>说明 : data.json与测试用例*.test.js|ets文件同目录

```json
{
	"suites": [{
		"describe": ["actsAbilityTest"],
		"stress": 2,
		"params": {
			"suiteParams1": "suiteParams001",
			"suiteParams2": "suiteParams002"
		},
		"items": [{
			"it": "testDataDriverAsync",
			"stress": 2,
			"params": [{
				"name": "tom",
				"value": 5
			}, {
				"name": "jerry",
				"value": 4
			}]
		}, {
			"it": "testDataDriver",
			"stress": 3
		}]
	}]
}
```

配置参数说明：

|      | 配置项名称 | 功能                                  | 必填 |
| :--- | :--------- | :------------------------------------ | ---- |
| 1    | "suite"    | 测试套配置 。                         | 是   |
| 2    | "items"    | 测试用例配置 。                       | 是   |
| 3    | "describe" | 测试套名称 。                         | 是   |
| 4    | "it"       | 测试用例名称 。                       | 是   |
| 5    | "params"   | 测试套 / 测试用例 可传入使用的参数 。 | 否   |
| 6    | "stress"   | 测试套 / 测试用例 指定执行次数 。     | 否   |

示例代码：

DevEco Studio从V3.0 Release（2022-09-06）版本开始支持

stage模型：

在TestAbility目录下TestAbility.ets文件中导入data.json，并在Hypium.hypiumTest() 方法执行前，设置参数数据

FA模型：

在TestAbility目录下app.js或app.ets文件中导入data.json，并在Hypium.hypiumTest() 方法执行前，设置参数数据

```javascript
import AbilityDelegatorRegistry from '@ohos.application.abilityDelegatorRegistry'
import { Hypium } from '@ohos/hypium'
import testsuite from '../test/List.test'
import data from '../test/data.json';

...
Hypium.setData(data);
Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite);
...
```

```javascript
 import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

 export default function abilityTest() {
  describe('actsAbilityTest', () => {
    it('testDataDriverAsync', 0, async (done: Function, data: ParmObj) => {
      console.info('name: ' + data.name);
      console.info('value: ' + data.value);
      done();
    });

    it('testDataDriver', 0, () => {
      console.info('stress test');
    })
  })
}
 interface ParmObj {
   name: string,
   value: string
 }
```
>说明 : 若要使用数据驱动传入参数功能，测试用例it的func必须传入两个参数：done定义在前面，data定义在后面；若不使用数据驱动传入参数功能，func可以不传参或传入done

正确示例：
```javascript
    it('testcase01', 0, async (done: Function, data: ParmObj) => {
      ...
      done();
    });
    it('testcase02', 0, async (done: Function) => {
      ...
      done();
    });
    it('testcase03', 0, () => {
      ...
    });
```
错误示例：
```javascript
    it('testcase01', 0, async (data: ParmObj, done: Function) => {
      ...
      done();
    });
    it('testcase02', 0, async (data: ParmObj) => {
      ...
    });
```

#### 专项能力

该项能力需通过在cmd窗口中输入aa test命令执行触发，并通过设置执行参数触发不同功能。另外，测试应用模型与编译方式不同，对应的aa test命令也不同，具体可参考[自动化测试框架使用指导](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/application-test/arkxtest-guidelines.md#cmd%E6%89%A7%E8%A1%8C)

- **筛选能力**

  1、按测试用例属性筛选

  可以利用框架提供的Level、Size、TestType 对象，对测试用例进行标记，以区分测试用例的级别、粒度、测试类型，各字段含义及代码如下：

  | Key      | 含义说明     | Value取值范围                                                                                                                                          |
  | -------- | ------------ |----------------------------------------------------------------------------------------------------------------------------------------------------|
  | level    | 用例级别     | "0","1","2","3","4", 例如：-s level 1                                                                                                                 |
  | size     | 用例粒度     | "small","medium","large", 例如：-s size small                                                                                                         |
  | testType | 用例测试类型 | "function","performance","power","reliability","security","global","compatibility","user","standard","safety","resilience", 例如：-s testType function |

  示例代码：

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

  示例命令: 

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s testType function -s size small -s level 0
  ```

  该命令作用是筛选测试应用中同时满足，用例测试类型是“function”、用例粒度是“small”、用例级别是“0”的三个条件用例执行。

  2、按测试套/测试用例名称筛选

  注意：测试套和测试用例的命名要符合框架规则，即以字母开头，后跟一个或多个字母、数字，不能包含特殊符号。

  框架可以通过指定测试套与测试用例名称，来指定特定用例的执行，测试套与用例名称用“#”号连接，多个用“,”英文逗号分隔

  | Key      | 含义说明                | Value取值范围                                                |
  | -------- | ----------------------- | ------------------------------------------------------------ |
  | class    | 指定要执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s class attributeTest#testAttributeIt |
  | notClass | 指定不执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s notClass attributeTest#testAttribut |

  示例代码：

  ```javascript
  import { describe, it, expect, TestType, Size, Level } from '@ohos/hypium';

  export default function attributeTest() {
  describe('describeTest_000',  () => {
    it("testIt_00", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0,  () => {
      console.info('Hello Test');
    })

    it("testIt_01", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      console.info('Hello Test');
    })
  })

  describe('describeTest_001',  () => {
    it("testIt_02", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      console.info('Hello Test');
    })
  })
}
  ```

  示例命令1: 

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s class describeTest_000#testIt_00,describeTest_001
  ```

  该命令作用是执行“describeTest_001”测试套中所有用例，以及“describeTest_000”测试套中的“testIt_00”用例。

  示例命令2：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s notClass describeTest_000#testIt_01
  ```

  该命令作用是不执行“describeTest_000”测试套中的“testIt_01”用例。

- **随机执行**

  使测试套与测试用例随机执行，用于稳定性测试。

  | Key    | 含义说明                             | Value取值范围                                  |
  | ------ | ------------------------------------ | ---------------------------------------------- |
  | random | @since1.0.3 测试套、测试用例随机执行 | true, 不传参默认为false， 例如：-s random true |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s random true
  ```

- **压力测试**

  指定要执行用例的执行次数，用于压力测试。

  | Key    | 含义说明                             | Value取值范围                  |
  | ------ | ------------------------------------ | ------------------------------ |
  | stress | @since1.0.5 指定要执行用例的执行次数 | 正整数， 例如： -s stress 1000 |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s stress 1000
  ```

- **用例超时时间设置**

  指定测试用例执行的超时时间，用例实际耗时如果大于超时时间，用例会抛出"timeout"异常，用例结果会显示“excute timeout XXX”

  | Key     | 含义说明                   | Value取值范围                                        |
  | ------- | -------------------------- | ---------------------------------------------------- |
  | timeout | 指定测试用例执行的超时时间 | 正整数(单位ms)，默认为 5000，例如： -s timeout 15000 |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s timeout 15000
  ```

- **遇错即停模式**

  | Key          | 含义说明                                                     | Value取值范围                                        |
  | ------------ | ------------------------------------------------------------ | ---------------------------------------------------- |
  | breakOnError | @since1.0.6 遇错即停模式，当执行用例断言失败或者发生错误时，退出测试执行流程 | true, 不传参默认为false， 例如：-s breakOnError true |
  
  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s breakOnError true
  ```
  
- **测试套中用例信息输出**

  输出测试应用中待执行的测试用例信息

  | Key    | 含义说明                     | Value取值范围                                  |
  | ------ | ---------------------------- | ---------------------------------------------- |
  | dryRun | 显示待执行的测试用例信息全集 | true, 不传参默认为false， 例如：-s dryRun true |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s dryRun true
  ```

- **嵌套能力**

  1.示例代码
  ```javascript
  // Test1.test.ets
  import { describe, expect, it } from '@ohos/hypium';
  import test2 from './Test2.test';

  export default function test1() {
    describe('test1', () => {
      it('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      // 引入测试套test2
      test2();
    })
  }
  ```

  ```javascript
  //Test2.test.ets
  import { describe, expect, it } from '@ohos/hypium';

  export default function test2() {
    describe('test2', () => {
      it('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      it('assertContain2', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
  ```

  ```javascript
  //List.test.ets
  import test1 from './nest/Test1.test';

  export default function testsuite() {
    test1();
  }
  ```

  2.示例筛选参数
    ```shell
    #执行test1的全部测试用例
    -s class test1 
    ```
    ```shell
    #执行test1的子测试用例
    -s class test1#assertContain1
    ```
    ```shell
    #执行test1的子测试套test2的测试用例
    -s class test1.test2#assertContain1
    ```

- **跳过能力**

    | Key          | 含义说明                                                     | Value取值范围                                        |
  | ------------ | ------------------------------------------------------------ | ---------------------------------------------------- |
  | skipMessage | @since1.0.17 显示待执行的测试用例信息全集中是否包含跳过测试套和跳过用例的信息 | true/false, 不传参默认为false， 例如：-s skipMessage true |
  | runSkipped | @since1.0.17 指定要执行的跳过测试套&跳过用例 | all，skipped，${describeName}#${itName}，${describeName}，不传参默认为空，例如：-s runSkipped all |

  1.示例代码
  
  ```javascript
  //Skip1.test.ets
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
  
    ```javascript
  //Skip2.test.ets
  import { describe, expect, xit, it } from '@ohos/hypium';
  
  export default function skip2() {
    describe('skip2', () => {
      //默认会跳过assertContain1
      xit('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      it('assertContain2', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
    ```



### 使用方式

单元测试框架以ohpm包形式发布至[服务组件官网](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)，开发者可以下载Deveco Studio后，在应用工程中配置依赖后使用框架能力，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001263160453)。
>**说明**
>
>1.0.8版本开始单元测试框架以HAR(Harmony Archive)格式发布
>
> 

## Ui测试框架功能特性

| No.  | 特性        | 功能说明                                            |
| ---- |-----------|-------------------------------------------------|
| 1    | Driver    | Ui测试的入口，提供查找控件，检查控件存在性以及注入按键能力。                 |
| 2    | On        | 用于描述目标控件特征(文本、id、类型等)，`Driver`根据`On`描述的控件特征信息来查找控件。 |
| 3    | Component | Driver查找返回的控件对象，提供查询控件属性，滑动查找等触控和检视能力。          |
| 4    | UiWindow  | Driver查找返回的窗口对象，提供获取窗口属性、操作窗口的能力。               |

**使用者在测试脚本通过如下方式引入使用：**

```typescript
import {Driver,ON,Component,UiWindow,MatchPattern} from '@ohos.UiTest'
```

> 须知
> 1. `On`类提供的接口全部是同步接口，使用者可以使用`builder`模式链式调用其接口构造控件筛选条件。
> 2. `Driver`和`Component`类提供的接口全部是异步接口(`Promise`形式)，**需使用`await`语法**。
> 3. Ui测试用例均需使用**异步**语法编写用例，需遵循单元测试框架异步用例编写规范。

   

在测试用例文件中import `On/Driver/Component`类，然后调用API接口编写测试用例。

```javascript
import { Driver, ON, Component } from '@kit.TestKit'
import { describe, it, expect } from '@ohos/hypium'

export default function findComponentTest() {
  describe('findComponentTest', () => {
    it('uitest_demo0', 0, async () => {
      // create Driver
      let driver: Driver = Driver.create();
      // find component by text
      let button: Component = await driver.findComponent(ON.text('Hello World').enabled(true));
      // click component
      await button.click();
      // get and assert component text
      let content: string = await button.getText();
      expect(content).assertEqual('Hello World');
    })
  })
}
```

### 基于ArkTS API的测试


#### Driver使用说明

`Driver`类作为UiTest测试框架的总入口，提供查找控件，注入按键，单击坐标，滑动控件，手势操作，截图等能力。

| No.  | API                                                             | 功能描述               |
| ---- |-----------------------------------------------------------------| ---------------------- |
| 1    | create():Promise<Driver>                                        | 静态方法，构造Driver。 |
| 2    | findComponent(on:On):Promise<Component>                         | 查找匹配控件。         |
| 3    | pressBack():Promise<void>                                       | 单击BACK键。           |
| 4    | click(x:number, y:number):Promise<void>                         | 基于坐标点的单击。      |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | 基于坐标点的滑动。      |
| 6    | assertComponentExist(on:On):Promise<void>                       | 断言匹配的控件存在。     |
| 7    | delayMs(t:number):Promise<void>                                 | 延时。                 |
| 8    | screenCap(s:path):Promise<void>                                 | 截屏。                 |
| 9    | findWindow(filter: WindowFilter): Promise<UiWindow>             | 查找匹配窗口。         |

其中assertComponentExist接口是断言API，用于断言当前界面存在目标控件；如果控件不存在，该API将抛出JS异常，使当前测试用例失败。

```javascript
import { describe, it} from '@ohos/hypium';
import { Driver, ON } from '@kit.TestKit';

export default function assertComponentExistTest() {
  describe('assertComponentExistTest', () => {
    it('Uitest_demo0', 0, async (done: Function) => {
      try{
        // create Driver
        let driver: Driver = Driver.create();
        // assert text 'hello' exists on current Ui
        await driver.assertComponentExist(ON.text('hello'));
      } finally {
        done();
      }
    })
  })
}
```

`Driver`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.UiTest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md#driver9)。

#### On使用说明

Ui测试框架通过`On`类提供了丰富的控件特征描述API，用来匹配查找要操作或检视的目标控件。`On`提供的API能力具有以下特点：

- 支持匹配单属性和匹配多属性组合，例如同时指定目标控件text和id。
- 控件属性支持多种匹配模式(等于，包含，`STARTS_WITH`，`ENDS_WITH`，`REG_EXP`，`REG_EXP_ICASE`)。

- 支持相对定位控件，可通过`isBefore`和`isAfter`等API限定邻近控件特征进行辅助定位。

| No. | API                                      | 功能描述                       |
|-----|------------------------------------------|----------------------------|
| 1   | id(i:string):On                          | 指定控件id。                    |
| 2   | text(t:string, p?:MatchPattern):On       | 指定控件文本，可指定匹配模式。            |
| 3   | type(t:string):On                        | 指定控件类型。                    |
| 4   | enabled(e:bool):On                       | 指定控件使能状态。                  |
| 5   | clickable(c:bool):On                     | 指定控件可单击状态。                 |
| 6   | longClickable(l:bool):On                 | 指定控件可长按状态。                 |
| 7   | focused(f:bool):On                       | 指定控件获焦状态。                  |
| 8   | scrollable(s:bool):On                    | 指定控件可滑动状态。                 |
| 9   | selected(s:bool):On                      | 指定控件选中状态。                  |
| 10  | checked(c:bool):On                       | 指定控件选择状态。                  |
| 11  | checkable(c:bool):On                     | 指定控件可选择状态。                 |
| 12  | isBefore(b:On):On                        | **相对定位**，限定目标控件位于指定特征控件之前。 |
| 13  | isAfter(b:On):On                         | **相对定位**，限定目标控件位于指定特征控件之后。 |
| 14   | id(i:string，p?:MatchPattern):On          | 指定控件id，可指定匹配模式。                    |
| 15   | hint(h:string, p?:MatchPattern):On       | 指定控件提示文本，可指定匹配模式。            |
| 16   | type(t:string，p?:MatchPattern):On        | 指定控件类型，可指定匹配模式。                   |
| 17   | description(d:string，p?:MatchPattern):On | 指定控件描述文本信息，可指定匹配模式。                   |

其中，`text`,`id`,`type`,`hint`,`description`属性支持{`MatchPattern.EQUALS`，`MatchPattern.CONTAINS`，`MatchPattern.STARTS_WITH`，`MatchPattern.ENDS_WITH`，`MatchPattern.REG_EXP`，`MatchPattern.REG_EXP_ICASE`}六种匹配模式，缺省使用`MatchPattern.EQUALS`模式。

`On`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.UiTest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md#on9)。

##### 控件定位

**示例代码1**：查找id是`Id_button`的控件。

```javascript
let button: Component = await driver.findComponent(ON.id('Id_button'))
```

 **示例代码2**：查找id是`Id_button`并且状态是`enabled`的控件，适用于无法通过单一属性定位的场景。

```javascript
let button: Component = await driver.findComponent(ON.id('Id_button').enabled(true))
```

通过`On.id(x).enabled(y)`来指定目标控件的多个属性。

**示例代码3**：查找文本中包含`hello`的控件，适用于不能完全确定控件属性取值的场景。

```javascript
let txt: Component = await driver.findComponent(ON.text('hello', MatchPattern.CONTAINS))
```

通过向`On.text()`方法传入第二个参数`MatchPattern.CONTAINS`来指定文本匹配规则；默认规则是`MatchPattern.EQUALS`，即目标控件text属性必须严格等于给定值。

#####  控件相对定位

**示例代码1**：查找位于文本控件`Item3_3`后面的，id是`Id_switch`的Switch控件。

```javascript
let switch: Component = await driver.findComponent(ON.id('Id_switch').isAfter(ON.text('Item3_3')))
```

通过`On.isAfter`方法，指定位于目标控件前面的特征控件属性，通过该特征控件进行相对定位。一般地，特征控件是某个具有全局唯一特征的控件(例如具有唯一的id或者唯一的text)。

类似的，可以使用`On.isBefore`控件指定位于目标控件后面的特征控件属性，实现相对定位。

#### Component使用说明

`Component`类代表了Ui界面上的一个控件，一般是通过`Driver.findComponent(on)`方法查找到的。通过该类的实例，用户可以获取控件属性，单击控件，滑动查找，注入文本等操作。

`Component`包含的常用API：

| No. | API                                | 功能描述                       |
|-----|------------------------------------|----------------------------|
| 1   | click():Promise<void>              | 单击该控件。                     |
| 2   | inputText(t:string):Promise<void>  | 向控件中输入文本(适用于文本框控件)。        |
| 3   | scrollSearch(s:On):Promise<Component>   | 在该控件上滑动查找目标控件(适用于List等控件)。 |
| 4   | scrollToTop(s:number):Promise<void>    | 滑动到该控件顶部(适用于List等控件)。      |
| 5   | scrollTobottom(s:number):Promise<void> | 滑动到该控件底部(适用于List等控件)。      |
| 6   | getText():Promise<string>          | 获取控件text。                  |
| 7   | getId():Promise<number>            | 获取控件id。                    |
| 8   | getType():Promise<string>          | 获取控件类型。                    |
| 9   | isEnabled():Promise<bool>          | 获取控件使能状态。                  |

`Component`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.UiTest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md#component9)。

**示例代码1**：单击控件。

```javascript
let button: Component = await driver.findComponent(ON.id('Id_button'))
await button.click()
```

**示例代码2**：通过get接口获取控件属性后，可以使用单元测试框架提供的assert*接口做断言检查。

```javascript
let component: Component = await driver.findComponent(ON.id('Id_title'))
expect(component !== null).assertTrue()
```

**示例代码3**：在List控件中滑动查找text是`Item3_3`的子控件。

```javascript
let list: Component = await driver.findComponent(ON.id('Id_list'))
let found: Component = await list.scrollSearch(ON.text('Item3_3'))
expect(found).assertTrue()
```

**示例代码4**：向输入框控件中输入文本。

```javascript
let editText: Component = await driver.findComponent(ON.type('InputText'))
await editText.inputText('user_name')
```
#### UiWindow使用说明

`UiWindow`类代表了Ui界面上的一个窗口，一般是通过`Driver.findWindow(WindowFilter)`方法查找到的。通过该类的实例，用户可以获取窗口属性，并进行窗口拖动、调整窗口大小等操作。

`UiWindow`包含的常用API：

| No.  | API                                                          | 功能描述                                           |
| ---- | ------------------------------------------------------------ | -------------------------------------------------- |
| 1    | getBundleName(): Promise<string>                             | 获取窗口所属应用包名。                             |
| 2    | getTitle(): Promise<string>                                  | 获取窗口标题信息。                                 |
| 3    | focus(): Promise<bool>                                       | 使得当前窗口获取焦点。                             |
| 4    | moveTo(x: number, y: number): Promise<bool>                  | 将当前窗口移动到指定位置（适用于支持移动的窗口）。 |
| 5    | resize(wide: number, height: number, direction: ResizeDirection): Promise<bool> | 调整窗口大小（适用于支持调整大小的窗口）。         |
| 6    | split(): Promise<bool>                                       | 将窗口模式切换为分屏模式(适用于支持分屏的窗口)。   |
| 7    | close(): Promise<bool>                                       | 关闭当前窗口。                                     |

`UiWindow`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.UiTest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-test-kit/js-apis-uitest.md#uiwindow9)。

**示例代码1**：获取窗口属性。

```javascript
let window: UiWindow = await driver.findWindow({actived: true})
let bundelName: string = await window.getBundleName()
```

**示例代码2**：移动窗口。

```javascript
let window: UiWindow = await driver.findWindow({actived: true})
await window.moveTo(500,500)
```

**示例代码3**：关闭窗口。

```javascript
let window: UiWindow = await driver.findWindow({actived: true})
await window.close()
```

#### 使用方式

开发者可以下载Deveco Studio创建测试工程后，在其中调用框架提供接口进行相关测试操作，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001267284568)。
UI测试框架使能需要执行如下命令。
>**说明**
>
>OpenHarmony 3.2版本需使用此命令，OpenHarmony4.0版本开始无需使用，默认使能。
>
> hdc shell param set persist.ace.testmode.enabled 1。

### 基于shell命令测试
> 在开发过程中，若需要快速进行截屏、 录屏、注入UI模拟操作、获取控件树等操作，可以使用shell命令，更方便完成相应测试。
> 
> **说明：**
> 
> 使用cmd的方式，需要配置好hdc相关的环境变量。

**命令列表**

| 命令            | 配置参数   | 描述                                                                                                                                                                                                                                                                  |
|---------------|---------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| help          |  | 显示uitest工具能够支持的命令信息。                                                                                                                                                                                                                                                |
| screenCap       |[-p savePath] | 截屏。<br> **-p**：指定存储路径和文件名, 只支持存放在/data/local/tmp/下。<br>         默认存储路径：/data/local/tmp，文件名：时间戳 + .png 。                                                                                                                                                        |
| dumpLayout      |[-p  savePath] <br/>[-w  windowId] <br/>[-b bundleName]<br/>[-m true/false ]<br/>[-i ]<br/>[-a]| 获取当前的控件树。默认过滤不可见控件并进行窗口合并。<br> **-p** ：指定存储路径和文件名, 只支持存放在/data/local/tmp/下。<br/>          默认存储路径：/data/local/tmp，文件名：时间戳 + .json。<br> **-w** ：仅获取指定windowId的应用窗口。<br/> **-b** ：仅获取指定bundleName的应用窗口。<br/> **-m** ：是否进行窗口合并。true 代表进行合并，不使用该选项时默认进行窗口合并。<br/> **-i** ：不过滤不可见控件,也不做窗口合并。<br/> **-a** ：保存 BackgroundColor、 Content、FontColor、FontSize、extraAttrs 属性数据。<br/> **-a和-i** 不可同时使用。 |
| uiRecord        | \<record/read> | 录制Ui操作。  <br> **record** ：开始录制，将当前界面操作记录到/data/local/tmp/record.csv，结束录制操作使用Ctrl+C结束录制。  <br> **read** ：读取并且打印录制数据。<br>各选项使用说明请参考[用户录制操作](#用户录制操作)。                                                                                                        |
| uiInput       | \<help / click / doubleClick / longClick / fling / swipe / drag / dircFling / inputText / text  / keyEvent> | 注入UI模拟操作。<br>各选项使用说明请参考[注入ui模拟操作](#注入ui模拟操作)。                   |
| --version |  | 获取当前工具版本号。                                                                                                                                                                                                                                                        |
| start-daemon|\<token>| token:测试应用端生产的uitest链接标识。<br/>拉起uitest测试进程，与token指向的测试应用进行连接。<br/>由测试应用注入，不支持用户通过shell命令使用。                                                                                                                                                            |


#### 截图使用示例

```bash
# 存储路径：/data/local/tmp，文件名：时间戳 + .png。
hdc shell uitest screenCap
# 指定存储路径和文件名,存放在/data/local/tmp/下。
hdc shell uitest screenCap -p /data/local/tmp/1.png
```

#### 获取控件树使用示例
```bash
hdc shell uitest dumpLayout -p /data/local/tmp/1.json
```

#### 用户录制操作
>**说明**
>
> 录制过程中，需等待当前操作的识别结果在命令行输出后，再进行下一步操作。
```bash
# 将当前界面操作记录到/data/local/tmp/record.csv，结束录制操作使用Ctrl+C结束录制。
hdc shell uitest uiRecord record
# 读取并打印录制数据。
hdc shell uitest uiRecord read
```

#### 注入UI模拟操作

| 命令   | 必填 | 描述              | 
|------|------|-----------------|
| help   | 是    | uiInput命令相关帮助信息。 |
| click   | 是    | 模拟单击操作。      | 
| doubleClick   | 是    | 模拟双击操作。      | 
| longClick   | 是    | 模拟长按操作。     | 
| fling   | 是    | 模拟快滑操作。   | 
| swipe   | 是    | 模拟慢滑操作。     | 
| drag   | 是    | 模拟拖拽操作。     | 
| dircFling   | 是    | 模拟指定方向滑动操作。     |
| inputText   | 是    | 指定坐标点，模拟输入框输入文本操作。     |
| text | 是    | 无需指定坐标点，在当前获焦处，模拟输入框输入文本操作。     |
| keyEvent   | 是    | 模拟实体按键事件(如：键盘,电源键,返回上一级,返回桌面等)，以及组合按键操作。     | 

##### uiInput click/doubleClick/longClick使用示例

| 配置参数    | 必填 | 描述            |
|---------|------|-----------------|
| point_x | 是      | 点击x坐标点。 |
| point_y | 是       | 点击y坐标点。 |

```shell
# 执行单击事件。
hdc shell uitest uiInput click 100 100

# 执行双击事件。
hdc shell uitest uiInput doubleClick 100 100

# 执行长按事件。
hdc shell uitest uiInput longClick 100 100
```

##### uiInput fling使用示例

| 配置参数  | 必填             | 描述               |      
|------|------------------|-----------------|
| from_x   | 是                | 滑动起点x坐标。 | 
| from_y   | 是                | 滑动起点y坐标。 | 
| to_x   | 是                | 滑动终点x坐标。 |
| to_y   | 是                | 滑动终点y坐标。 |
| swipeVelocityPps_   | 否      | 滑动速度，单位: (px/s)，取值范围：200-40000。<br> 默认值: 600。 | 
| stepLength_   | 否 | 滑动步长。默认值: 滑动距离/50。<br>  **为实现更好的模拟效果，推荐参数缺省/使用默认值。**  | 


```shell  
# 执行快滑操作，stepLength_缺省。
hdc shell uitest uiInput fling 10 10 200 200 500 
```

##### uiInput swipe/drag使用示例

| 配置参数  | 必填             | 描述               |      
|------|------------------|-----------------|
| from_x   | 是                | 滑动起点x坐标。 | 
| from_y   | 是                | 滑动起点y坐标。 | 
| to_x   | 是                | 滑动终点x坐标。 |
| to_y   | 是                | 滑动终点y坐标。 |
| swipeVelocityPps_   | 否      | 滑动速度，单位: (px/s)，取值范围：200-40000。<br> 默认值: 600。 | 

```shell  
# 执行慢滑操作。
hdc shell uitest uiInput swipe 10 10 200 200 500

# 执行拖拽操作。 
hdc shell uitest uiInput drag 10 10 100 100 500 
```

##### uiInput dircFling使用示例

| 配置参数             | 必填       | 描述 |
|-------------------|-------------|----------|
| direction         | 否 | 滑动方向，取值范围：[0,1,2,3]，默认值为0。<br> 0代表向左滑动，1代表向右滑动，2代表向上滑动，3代表向下滑动。    | 
| swipeVelocityPps_ | 否| 滑动速度，单位: (px/s)，取值范围：200-40000。<br> 默认值: 600。    | 
| stepLength        | 否        | 滑动步长。<br> 默认值: 滑动距离/50。为更好的模拟效果，推荐参数缺省/使用默认值。 |

```shell  
# 执行左滑操作
hdc shell uitest uiInput dircFling 0 500
# 执行向右滑动操作
hdc shell uitest uiInput dircFling 1 600
# 执行向上滑动操作。
hdc shell uitest uiInput dircFling 2 
# 执行向下滑动操作。
hdc shell uitest uiInput dircFling 3
```

##### uiInput inputText使用示例

| 配置参数             | 必填       | 描述 |       
|------|------------------|----------|
| point_x   | 是                | 输入框x坐标点。 | 
| point_y   | 是                | 输入框y坐标点。 |
| text   | 是                | 输入文本内容。  |

```shell  
# 执行输入框输入操作。
hdc shell uitest uiInput inputText 100 100 hello 
```

##### uiInput text使用示例
| 配置参数             | 必填       | 描述 |       
|------|------------------|----------|
| text   | 是                | 输入文本内容。  |

```shell  
# 无需输入坐标点，在当前获焦处，执行输入框输入操作。若当前获焦处不支持文本输入，则无实际效果。
hdc shell uitest uiInput text hello
```

##### uiInput keyEvent使用示例

| 配置参数             | 必填       | 描述 |                
|------|------|----------|
| keyID1   | 是    | 实体按键对应ID，取值范围：KeyCode/Back/Home/Power。<br>当取Back/Home/Power时，不支持输入组合键。 | 
| keyID2    | 否    | 实体按键对应ID。 |
| keyID3    | 否    | 实体按键对应ID。 |

>**说明**
>
> 最多支持传入是三个键值，具体取值请参考[KeyCode](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-input-kit/js-apis-keycode.md)。

```shell  
# 执行执行返回主页操作。
hdc shell uitest uiInput keyEvent Home
# 执行返回操作。
hdc shell uitest uiInput keyEvent Back
# 执行组合键粘贴操作。
hdc shell uitest uiInput keyEvent 2072 2038
```

#### 获取版本信息

```bash
hdc shell uitest --version
```
#### 拉起uitest测试进程

```shell  
hdc shell uitest start-daemon
```

### UI测试框架自构建方式

#### 构建命令

```shell
./build.sh --product-name rk3568 --build-target uitestkit
```
#### 推送位置

```shell
hdc target mount
hdc shell mount -o rw,remount /
hdc file send uitest /system/bin/uitest
hdc file send libuitest.z.so /system/lib/module/libuitest.z.so
hdc shell chmod +x /system/bin/uitest
```

## 版本信息

| 版本号  | 功能说明                                                     |
| :------ | :----------------------------------------------------------- |
| 3.2.2.1 | 1、增加抛滑、获取/设置屏幕方向接口<br />2、窗口处理逻辑增加不支持场景处理逻辑。  |
| 3.2.3.0 | 1、滑动控件进行滑动查找、滑动到尾部/顶部功能优化。              |
| 3.2.4.0 | 1、接口调用异常时会抛出错误码。                                 |
| 3.2.5.0 | 1、通信机制变更。                                               |
| 3.2.6.0 | 1、增加模拟鼠标操作能力接口<br />2、增加指定应用的窗口下查找目标控件接口。  |
| 4.0.1.1 | 1、支持在daemon运行时执行uitest dumpLayout。                    |
| 4.0.1.2 | 1、模拟鼠标动作、键鼠协同功能优化。                             |
| 4.0.1.3 | 1、示例代码更新<br />2、滑动控件进行滑动查找、滑动到尾部/顶部功能优化。  |
| 4.0.1.4 | 1、可选参数传入undefined时，当作默认值处理。                    |
| 4.0.2.0 | 1、支持监听toast和dialog控件出现，使用callback的形式返回结果。 |
| 4.0.3.0 | 1、增加加载运行.abc文件机制。                                |
| 4.0.4.0 | 1、支持abc_loader框架获取UI操作录制数据，屏幕数据，控件树等，并以callback的形式返回结果<br />2、修改录制数据结构。  |
| 4.1.1.1 | 1、对接批量获取控件信息能力，缩短获取控件信息的耗时。        |
| 4.1.2.0 | 1、增加shell命令方式注入UI模拟操作。                         |
| 4.1.3.0 | 1、新增命令行功能，uitest dumuLayout -a ,dump信息中包含控件的背景色、字体颜色/大小信息。 |
| 4.1.4.0 | 1、dump信息中增加hint与description字段。<br />2、优化多指操作。<br />3、优化查找控件的效率。<br />4、uitest uiInput执行效率提升。 |
| 5.0.1.0 | 1、优化swipe操作。<br />2、inputText输入中文的实现方式改为设置剪贴板数据后，长按控件点击粘贴。 |
| 5.0.1.1 | 1、节点新增以下属性，背景色：backgroundColor，背景图片：backgroundImage，透明度：opacity，模糊度：blur，事件是否透传：hitTestBehavior 。 |
| 5.0.1.2 | 1、通过test Sa发布公共事件。<br />2、节点新增clip属性，判断其子节点是否进行切割。<br />3、过滤机制调整，节点只与其clip为true的父节点进行切换计算可见区域，可见区域宽/高小于等于0记为不可见。<br />4、调用inputText时，被输入字符串超过200个字符时，实现方式调整为设置剪贴板数据后，植入ctrl + v。 |
| 5.1.1.1 | 1、控件支持正则表达式方式进行查找 <br />2、获取控件属性中的提示文本信息。  <br />3、支持横向滑动查找操作。  <br />4、支持不指定坐标模拟输入文本的shell命令 hdc shell uitest uiInput text "xxxx"。  |
| 5.1.1.2 | uitest dumpLayout 能力增强<br /> -w：仅获取指定window id的应用窗口。<br/> -b ：仅获取指定bundleName的应用窗口。<br/> -m ：是否进行窗口合并。不使用该选项时默认进行窗口合并。 |
| 6.0.1.0 | 1、inputText支持追加输入和指定通过剪贴板粘贴输入。  <br/>2、开放鼠标原子事件。  <br/>3、新增模拟手表表冠旋转。  <br/>4、支持触控屏/鼠标注入长按操作和拖拽操作时指定长按时间。 |


## 白盒性能测试框架功能特性

白盒性能测试框架PerfTest提供白盒性能测试能力，供开发者在测试场景使用，支持对指定代码段或指定场景的性能数据测试，支持自动化执行测试代码段，并采集耗时、CPU、内存、时延、帧率等性能数据。

**使用者在测试脚本通过如下方式引入使用：**

```typescript
import { PerfMetric, PerfTest, PerfTestStrategy, PerfMeasureResult } from '@kit.TestKit';
```

### 接口说明

`PerfTest`类作为白盒性能测试框架的入口，提供测试任务创建、测试代码段执行和数据采集、测量结果获取等能力。支持通过`PerfTestStrategy`设置测试执行策略，执行测试，并通过`PerfMeasureResult`获取测量结果。

| No.  | API                                                       | 功能描述               |
| ---- |-----------------------------------------------------------| ---------------------- |
| 1    | static create(strategy: PerfTestStrategy): PerfTest       | 静态方法，构造一个PerfTest对象，并返回该对象。 |
| 2    | run(): Promise\<void>                                     | 运行性能测试，迭代执行测试代码段并采集性能数据，使用Promise回调。         |
| 3    | getMeasureResult(metric: PerfMetric): PerfMeasureResult   | 获取指定性能指标的测量数据。           |
| 4    | destroy(): void                                           | 销毁PerfTest对象。      |

`PerfTestStrategy`支持设置性能测试的执行策略，如测试性能指标、被测应用包名、迭代次数等。

| 名称 | 类型   | 只读 |  可选 | 说明        |
| ---- | ------ | ---- | ---- |-----------|
| metrics     | Array\<[PerfMetric](#perfmetric)>           | 否 | 否 | 被测性能指标列表。  |
| actionCode  | Callback\<Callback\<boolean>> | 否 | 否 | 测试代码段。  |
| resetCode   | Callback\<Callback\<boolean>> | 否 | 是 | 测试结束环境重置代码段。默认为空，框架运行时不执行此代码段。  |
| bundleName  | string                      | 否 | 是 | 被测应用包名。默认为""，框架运行时测试当前测试应用的性能数据。  |
| iterations  | number                      | 否 | 是 | 测试迭代执行次数，默认值为5。  |
| timeout     | number                      | 否 | 是 | 单次代码段（actionCode/resetCode）执行的超时时间，默认值为10000ms。  |

`PerfMeasureResult`用于获取每轮测试的数据以及最大值、最小值、平均值等统计数据。

| 名称   | 类型   | 只读 | 可选 | 说明                      |
| ------ | ------ | ---- | ---- | ------------------------- |
| metric        | [PerfMetric](#perfmetric)    | 是 | 否 | 被测性能指标。  |
| roundValues   | Array\<number> | 是 | 否 | 被测性能指标的各轮测量数据值。当数据采集失败时返回-1。  |
| maximum       | number        | 是 | 否 | 各轮测量数据最大值（剔除为-1的数据后计算）。  |
| minimum       | number        | 是 | 否 | 各轮测量数据最小值（剔除为-1的数据后计算）。  |
| average       | number        | 是 | 否 | 各轮测量数据平均值（剔除为-1的数据后计算）。  |

### 使用示例

白盒性能测试框架PerfTest支持基于代码段和针对指定场景的性能测试，使用示例如下。

**示例1：基于代码段的性能测试，测试函数执行期间的应用性能**

- 在main > ets > utils文件夹下新增PerfUtils.ets文件，在文件中编写自定义的函数。

  ```ts
  export class PerfUtils {
    public static CalculateTest() {
      let num: number = 0
      for (let index = 0; index < 10000; index++) {
        num++;
      }
    }
  }
  ```

- 在ohosTest > ets > test文件夹下.test.ets文件中编写具体测试代码。

  ```ts
  import { describe, it, expect } from '@ohos/hypium';
  import { PerfMetric, PerfTest, PerfTestStrategy, PerfMeasureResult } from '@kit.TestKit';
  import { PerfUtils } from '../../../main/ets/utils/PerfUtils';

  export default function PerfTestTest() {
    describe('PerfTestTest', () => {
      it('testExample0', 0, async (done: Function) => {
        let metrics: Array<PerfMetric> = [PerfMetric.DURATION, PerfMetric.CPU_USAGE] // 指定被测指标
        let actionCode = async (finish: Callback<boolean>) => { // 测试代码段中使用uitest进行列表滑动
          await await PerfUtils.CalculateTest()
          finish(true);
        };
        let perfTestStrategy: PerfTestStrategy = {  // 定义测试策略
          metrics: metrics,
          actionCode: actionCode,
        };
        try {
          let perfTest: PerfTest = PerfTest.create(perfTestStrategy); // 创建测试任务对象PerfTest
          await perfTest.run(); // 执行测试，异步函数需使用await同步等待完成
          let res1: PerfMeasureResult = perfTest.getMeasureResult(PerfMetric.DURATION); // 获取耗时指标的测试结果
          let res2: PerfMeasureResult = perfTest.getMeasureResult(PerfMetric.CPU_USAGE); // 获取CPU使用率指标的测试结果
          perfTest.destroy(); // 销毁PerfTest对象
          expect(res1.average).assertLessOrEqual(1000); // 断言性能测试结果
          expect(res2.average).assertLessOrEqual(30); // 断言性能测试结果
        } catch (error) {
          console.error(`Failed to execute perftest. Cause:${JSON.stringify(error)}`);
          expect(false).assertTrue()
        }
        done();
      })
    })
  }
  ```

**示例1：针对指定场景的性能测试，测试当前应用内列表滑动帧率**

- 编写Index.ets页面代码，作为被测示例demo。

  ```ts
  @Entry
  @Component
  struct ListPage {
    scroller: Scroller = new Scroller()
    private arr: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    build() {
      Row() {
        Column() {
          Scroll(this.scroller) {
            Column() {
              ForEach(this.arr, (item: number) => {
                Text(item.toString())
                  .width('90%')
                  .height('40%')
                  .fontSize(80)
                  .textAlign(TextAlign.Center)
                  .margin({ top: 10 })
              }, (item: string) => item)
            }
          }
          .width('100%')
          .height('100%')
          .scrollable(ScrollDirection.Vertical)
          .scrollBar(BarState.On)
          .scrollBarColor(Color.Gray)
        }
        .width('100%')
      }
      .height('100%')
    }
  }
  ```

- 在ohosTest > ets > test文件夹下.test.ets文件中编写具体测试代码。

  ```ts
  import { describe, it, expect } from '@ohos/hypium';
  import { PerfMetric, PerfTest, PerfTestStrategy, PerfMeasureResult } from '@kit.TestKit';
  import { abilityDelegatorRegistry, Driver, ON } from '@kit.TestKit';
  import { Want } from '@kit.AbilityKit';

  const delegator: abilityDelegatorRegistry.AbilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
  export default function PerfTestTest() {
    describe('PerfTestTest', () => {
      it('testExample',0, async (done: Function) => {
        let driver = Driver.create();
        await driver.delayMs(1000);
        const bundleName = abilityDelegatorRegistry.getArguments().bundleName;
        const want: Want = {
          bundleName: bundleName,
          abilityName: 'EntryAbility'
        };
        await delegator.startAbility(want); // 打开测试页面
        await driver.delayMs(1000);
        let scroll = await driver.findComponent(ON.type('Scroll'));
        await driver.delayMs(1000);
        let center = await scroll.getBoundsCenter();  // 获取Scroll可滚动组件坐标
        await driver.delayMs(1000);
        let metrics: Array<PerfMetric> = [PerfMetric.LIST_SWIPE_FPS]  // 指定被测指标为列表滑动帧率
        let actionCode = async (finish: Callback<boolean>) => { // 测试代码段中使用uitest进行列表滑动
          await driver.fling({x: center.x, y: Math.floor(center.y * 3 / 2)}, {x: center.x, y: Math.floor(center.y / 2)}, 50, 20000);
          await driver.delayMs(3000);
          finish(true);
        };
        let resetCode = async (finish: Callback<boolean>) => {  // 复位环境，将列表划至顶部
          await scroll.scrollToTop(40000);
          await driver.delayMs(1000);
          finish(true);
        };
        let perfTestStrategy: PerfTestStrategy = {  // 定义测试策略
          metrics: metrics,
          actionCode: actionCode,
          resetCode: resetCode,
          iterations: 5,  // 指定测试迭代次数
          timeout: 50000, // 指定actionCode和resetCode的超时时间
        };
        try {
          let perfTest: PerfTest = PerfTest.create(perfTestStrategy); // 创建测试任务对象PerfTest
          await perfTest.run(); // 执行测试，异步函数需使用await同步等待完成
          let res: PerfMeasureResult = perfTest.getMeasureResult(PerfMetric.LIST_SWIPE_FPS); // 获取列表滑动帧率指标的测试结果
          perfTest.destroy(); // 销毁PerfTest对象
          expect(res.average).assertLargerOrEqual(60);  // 断言性能测试结果
        } catch (error) {
          console.error(`Failed to execute perftest. Cause:${JSON.stringify(error)}`);
        }
        done();
      })
    })
  }
  ```

PerfTest完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.test.PerfTest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-test-kit/js-apis-perftest.md)。