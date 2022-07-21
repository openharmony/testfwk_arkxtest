# OpenHarmony自动化测试框架使用介绍

## 简介
 OpenHarmony自动化测试框架代码部件仓arkXtest，包含单元测试框架(JsUnit)和Ui测试框架(UiTest)。

 单元测试框架(JsUnit)提供单元测试用例执行能力，提供用例编写基础接口，生成对应报告，用于测试系统或应用接口。

 Ui测试框架(UiTest)通过简洁易用的API提供查找和操作界面控件能力，支持用户开发基于界面操作的自动化测试脚本。

## 目录

```
arkXtest 
  |-----jsunit  单元测试框架
  |-----uitest  Ui测试框架
```
## 约束限制
本模块首批接口从API version 8开始支持。后续版本的新增接口，采用上角标单独标记接口的起始版本。

## 单元测试框架功能特性

| No.  | 特性     | 功能说明                           |
| ---- | -------- | ---------------------------------- |
| 1    | 基础流程 | 支持编写及执行基础用例             |
| 2    | 断言库   | 判断用例实际期望值与预期值是否相符 |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe代表一个测试套， it代表一条用例。

| No.  | API        | 功能说明                                                     |
| ---- | ---------- | ------------------------------------------------------------ |
| 1    | describe   | 定义一个测试套，支持两个参数：测试套名称和测试套函数         |
| 2    | beforeAll  | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数 |
| 3    | beforeEach | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数 |
| 4    | afterEach  | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数 |
| 5    | afterAll   | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数 |
| 6    | it         | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数 |
| 7    | expect     | 支持bool类型判断等多种断言方法                               |

示例代码：

```javascript
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium'
import demo from '@ohos.bundle'

export default async function abilityTest() {
  describe('ActsAbilityTest', function () {
    it('String_assertContain_success', 0, function () {
      let a = 'abc'
      let b = 'b'
      expect(a).assertContain(b)
      expect(a).assertEqual(a)
    })
    it('getBundleInfo_0100', 0, async function () {
      const NAME1 = "com.example.MyApplicationStage"
      await demo.getBundleInfo(NAME1,
        demo.BundleFlag.GET_BUNDLE_WITH_ABILITIES | demo.BundleFlag.GET_BUNDLE_WITH_REQUESTED_PERMISSION)
        .then((value) => {
          console.info(value.appId)
        })
        .catch((err) => {
          console.info(err.code)
        })
    })
  })
}
```



####  断言库

断言功能列表：


| No.  | API              | 功能说明                                                     |
| :--- | :--------------- | ------------------------------------------------------------ |
| 1    | assertClose      | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1) |
| 2    | assertContain    | 检验actualvalue中是否包含expectvalue                         |
| 3    | assertEqual      | 检验actualvalue是否等于expectvalue[0]                        |
| 4    | assertFail       | 抛出一个错误                                                 |
| 5    | assertFalse      | 检验actualvalue是否是false                                   |
| 6    | assertTrue       | 检验actualvalue是否是true                                    |
| 7    | assertInstanceOf | 检验actualvalue是否是expectvalue类型                         |
| 8    | assertLarger     | 检验actualvalue是否大于expectvalue                           |
| 9    | assertLess       | 检验actualvalue是否小于expectvalue                           |
| 10   | assertNull       | 检验actualvalue是否是null                                    |
| 11   | assertThrowError | 检验actualvalue抛出Error内容是否是expectValue                |
| 12   | assertUndefined  | 检验actualvalue是否是undefined                               |

示例代码：

```javascript
import { describe, it, expect } from '@ohos/hypium'
export default async function abilityTest() {
  describe('assertClose', function () {
    it('assertBeClose success', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(99, b)
    })
    it('assertBeClose fail', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(1, b)
    })
    it('assertBeClose fail', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(null, b)
    })
    it('assertBeClose fail', 0, function () {
      expect(null).assertClose(null, 0)
    })
  })
}
```
### 使用方式

单元测试框架以npm包（hypium）形式发布至[服务组件官网](https://repo.harmonyos.com/#/cn/application/atomService/@ohos%2Fhypium)，开发者可以下载Deveco Studio后，在应用工程中配置依赖后使用框架能力，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001263160453)。

### 单元测试框架Mock能力

目前支持函数级mock能力，对定义的函数进行mock后修改函数的行为，使其返回指定的值或者执行某种动作。

#### 约束限制

单元测试框架Mock能力从npm包1.0.1版本开始支持，需修改工程中package.info中配置依赖npm包版本号后使用。

-  **API列表：**

| No. | API | 功能说明 |
| --- | --- | --- |
| 1 |  mockFunc( f：founction )：founction() |    对定义的方法进行mock后修改方法的行为,返回一个被修改后的方法 |
| 2 | when(mockedfunc：function) | 对传入后方法做检查，检查是否被mock并标记过，返回的是一个方法声明 |
| 3 |afterReturn(x：value) | 设定预期返回一个自定义的值value，比如某个字符串 |
| 4 | afterReturnNothing() | 设定预期没有返回值，即 undefine |
| 5 |  afterAction(x：action) | 设定预期返回一个函数执行的操作 |
| 6 |  afterThrow(x：msg) | 设定预期抛出异常，并指定异常msg |
| 7 |  clear() | 用例执行完毕后，进行数据mocker对象的清理 |
| 8 |  any | 设定用户传任何类型参数（undefine和null除外），执行的结果都是预期的值 |
| 9 |  anyString | 设定用户传任何字符串参数，执行的结果都是预期的值 |
| 10 |  anyBoolean | 设定用户传任何boolean类型参数，执行的结果都是预期的值 |
| 11 |  anyFounction | 设定用户传任何function类型参数，执行的结果都是预期的值 |
| 12 | anyNumber | 设定用户传任何数字类型参数，执行的结果都是预期的值 |
| 13 | anyObj | 设定用户传任何对象类型参数，执行的结果都是预期的值 |
| 14 | matchRegexs(Regex) | 设定用户传任何正则表达式类型参数Regex，执行的结果都是预期的值 |

-  **使用示例：**

用户可以通过一下方式进行引入mock模块进行测试用例编写：

```javascript
//必须引入的mock能力模块api: MockKit, when, mockFunc等
//根据自己用例需要引入断言能力api
import { describe, expect, it, MockKit, when, mockFunc } from '@ohos/hypium'
```
**示例1：afterReturn 的使用**

```javascript
import { describe, expect, it, MockKit, when, mockFunc } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");
            
            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();
            
            //2.定义需要被mock的方法func
            var func = function() {
                return "test";
            };
            
            //3.把api mockFunc定义给mocker对象
            mocker.mockFunc = mockFunc;
            
            //4.进行mock操作
            let mockfunc = mocker.mockFunc(func);
            when(mockfunc)('test').afterReturn('1');

            //5.对mock后的函数进行断言，看是否符合预期
            //执行成功案例，参数为'test'
            expect(mockfunc('test')).assertEqual('1');//执行通过
            
            //执行失败案例，参数为 'abc'
            expect(mockfunc('abc')).assertEqual('1');//执行失败
        })
    })
}
```
- **特别注意：**
`when(mockfunc)('test').afterReturn('1');`
这句代码中的`('test')`是mock后的函数需要传递的匹配参数，目前支持一个参数
`afterReturn('1')`是用户需要预期返回的结果。
有且只有在参数是`('test')`的时候，执行的结果才是用户自定义的预期结果。

**示例2： afterReturnNothing 的使用**

```javascript
import { describe, expect, it, MockKit, when, mockFunc } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");
            
            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();
            
            //2.定义需要被mock的方法func
            var func = function() {
                return "test";
            };
            
            //3.把api mockFunc定义给mocker对象
            mocker.mockFunc = mockFunc;
            
            //4.进行mock操作
            let mockfunc = mocker.mockFunc(func);
            //根据自己需求进行选择 执行完毕后的动作，比如这里我选择afterReturnNothing();即不返回任何值
            when(mockfunc)('test').afterReturnNothing();

            //5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
           
             //执行成功案例，参数为'test'
            expect(mockfunc('test')).assertUndefined();//执行通过
            
            //执行失败案例，参数传为 123
            expect(mockfunc(123)).assertUndefined();//执行失败
        })
    })
}
```

**示例3： 设定参数类型为any ，即接受任何参数（undefine和null除外）的使用**

```javascript
//注意需要引入ArgumentMatchers类，即参数匹配器，any系列同理
import {any} from '@ohos/hypium/src/module/mock/ArgumentMatchers'
```


```javascript
import { describe, expect, it, MockKit, when, mockFunc, any} from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");
            
            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();
            
            //2.定义需要被mock的方法func
            var func = function() {
                return "test";
            };
            
            //3.把api mockFunc定义给mocker对象
            mocker.mockFunc = mockFunc;
            
            //4.进行mock操作
            let mockfunc = mocker.mockFunc(func);
            //根据自己需求进行选择 
            when(mockfunc)(any).afterReturn('1');
            
            //5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例1，传参为字符串类型
            expect(mockfunc('test')).assertEqual('1');//用例执行通过。
             //执行成功的案例2，传参为数字类型123
            //expect(mockfunc(123)).assertEqual('1');//用例执行通过。
             //执行成功的案例3，传参为boolean类型true
            //expect(mockfunc(true)).assertEqual('1');//用例执行通过。
            
            //执行失败的案例，传参为数字类型空
            //expect(mockfunc()).assertEqual('1');//用例执行失败。
        })
    })
}
```

**示例4： 设定参数类型为anyString,anyBoolean等 的使用**
```javascript
import { describe, expect, it, MockKit, when, mockFunc, anyString} from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");
            
            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();
            
            //2.定义需要被mock的方法func
            var func = function() {
                return "test";
            };
            
            //3.把api mockFunc定义给mocker对象
            mocker.mockFunc = mockFunc;
            
            //4.进行mock操作
            let mockfunc = mocker.mockFunc(func);
            //根据自己需求进行选择 
            when(mockfunc)(anyString).afterReturn('1');
            
            //5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例，传参为字符串类型
            expect(mockfunc('test')).assertEqual('1');//用例执行通过。
            
            //执行失败的案例，传参为数字类型
            //expect(mockfunc(123)).assertEqual('1');//用例执行失败。
        })
    })
}
```

**示例5： 设定参数类型为matchRegexs（Regex）等 的使用**
```javascript
import { describe, expect, it, MockKit, when, mockFunc, matchRegexs} from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");
            
            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();
            
            //2.定义需要被mock的方法func
            var func = function() {
                return "test";
            };
            
            //3.把api mockFunc定义给mocker对象
            mocker.mockFunc = mockFunc;
            
            //4.进行mock操作
            let mockfunc = mocker.mockFunc(func);
            //根据自己需求进行选择，这里假设匹配正则，且正则为/123456/
            when(mockfunc)(matchRegexs(/123456/)).afterReturn('1');
            
            //5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例，传参为字符串 比如 '1234567898'
            //expect(mockfunc('1234567898')).assertEqual('1');//用例执行通过。
            //因为字符串 '1234567898'可以和正则/123456/匹配上
            
            //执行失败的案例，传参为字符串'1234'
            //expect(mockfunc('1234')).assertEqual('1');//用例执行失败。
        })
    })
}
```

## Ui测试框架功能特性

| No.  | 特性        | 功能说明                                                     |
| ---- | ----------- | ------------------------------------------------------------ |
| 1    | UiDriver    | Ui测试的入口，提供查找控件，检查控件存在性以及注入按键能力   |
| 2    | By          | 用于描述目标控件特征(文本、id、类型等)，`UiDriver`根据`By`描述的控件特征信息来查找控件 |
| 3    | UiComponent | UiDriver查找返回的控件对象，提供查询控件属性，滑动查找等触控和检视能力 |

**使用者在测试脚本通过如下方式引入使用：**

```typescript
import {UiDriver,BY,UiComponent,MatchPattern} from '@ohos.uitest'
```

> 注意事项
> 1. `By`类提供的接口全部是同步接口，使用者可以使用`builder`模式链式调用其接口构造控件筛选条件。
> 2. `UiDrivier`和`UiComponent`类提供的接口全部是异步接口(`Promise`形式)，**需使用`await`语法**。
> 3. Ui测试用例均需使用**异步**语法编写用例，需遵循单元测试框架异步用例编写规范。

   

在测试用例文件中import `By/UiDriver/UiComponent`类，然后调用API接口编写测试用例。

```javascript
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from '@ohos/hypium'
import {BY, UiDriver, UiComponent, MatchPattern} from '@ohos.uitest'

export default async function abilityTest() {
  describe('uiTestDemo', function() {
    it('uitest_demo0', 0, async function() {
      // create UiDriver
      let driver = await UiDriver.create()
      // find component by text
      let button = await driver.findComponent(BY.text('hello').enabled(true))
      // click component
      await button.click()
      // get and assert component text
      let content = await button.getText()
      expect(content).assertEquals('clicked!')
    })
  })
}
```

### UiDriver使用说明

`UiDriver`类作为UiTest测试框架的总入口，提供查找控件，注入按键，单击坐标，滑动控件，手势操作，截图等能力。

| No.  | API                                                          | 功能描述               |
| ---- | ------------------------------------------------------------ | ---------------------- |
| 1    | create():Promise<UiDriver>                                   | 静态方法，构造UiDriver |
| 2    | findComponent(b:By):Promise<UiComponent>                     | 查找匹配控件           |
| 3    | pressBack():Promise<void>                                    | 单击BACK键             |
| 4    | click(x:number, y:number):Promise<void>                      | 基于坐标点的单击       |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | 基于坐标点的滑动       |
| 6    | assertComponentExist(b:By):Promise<void>                     | 断言匹配的控件存在     |
| 7    | delayMs(t:number):Promise<void>                              | 延时                   |
| 8    | screenCap(s:path):Promise<void>                              | 截屏                   |

其中assertComponentExist接口是断言API，用于断言当前界面存在目标控件；如果控件不存在，该API将抛出JS异常，使当前测试用例失败。

```javascript
import {BY,UiDriver,UiComponent} from '@ohos.uitest'

export default async function abilityTest() {
  describe('UiTestDemo', function() {
    it('Uitest_demo0', 0, async function(done) {
      try{
        // create UiDriver
        let driver = await UiDriver.create()
        // assert text 'hello' exists on current Ui
        await assertComponentExist(BY.text('hello'))
      } finally {
        done()
      }
    })
  })
}
```

### By使用说明

Ui测试框架通过`By`类提供了丰富的控件特征描述API，用来匹配查找要操作或检视的目标控件。`By`提供的API能力具有以下特点：

- 支持匹配单属性和匹配多属性组合，例如同时指定目标控件text和id。
- 控件属性支持多种匹配模式(等于，包含，`STARTS_WITH`，`ENDS_WITH`)。
- 支持相对定位控件，可通过`isBefore`和`isAfter`等API限定邻近控件特征进行辅助定位。

| No.  | API                                | 功能描述                                       |
| ---- | ---------------------------------- | ---------------------------------------------- |
| 1    | id(i:number):By                    | 指定控件id                                     |
| 2    | text(t:string, p?:MatchPattern):By | 指定控件文本，可指定匹配模式                   |
| 3    | type(t:string)):By                 | 指定控件类型                                   |
| 4    | enabled(e:bool):By                 | 指定控件使能状态                               |
| 5    | clickable(c:bool):By               | 指定控件可单击状态                             |
| 6    | focused(f:bool):By                 | 指定控件获焦状态                               |
| 7    | scrollable(s:bool):By              | 指定控件可滑动状态                             |
| 8    | selected(s:bool):By                | 指定控件选中状态                               |
| 9    | isBefore(b:By):By                  | **相对定位**，限定目标控件位于指定特征控件之前 |
| 10   | isAfter(b:By):By                   | **相对定位**，限定目标控件位于指定特征控件之后 |

其中，`text`属性支持{`MatchPattern.EQUALS`，`MatchPattern.CONTAINS`，`MatchPattern.STARTS_WITH`，`MatchPattern.ENDS_WITH`}四种匹配模式，缺省使用`MatchPattern.EQUALS`模式。

#### 控件绝对定位

**示例代码1**：查找id是`Id_button`的控件。

```javascript
let button = await driver.findComponent(BY.id(Id_button))
```

 **示例代码2**：查找id是`Id_button`并且状态是`enabled`的控件，适用于无法通过单一属性定位的场景。

```javascript
let button = await driver.findComponent(BY.id(Id_button).enabled(true))
```

通过`By.id(x).enabled(y)`来指定目标控件的多个属性。

**示例代码3**：查找文本中包含`hello`的控件，适用于不能完全确定控件属性取值的场景。

```javascript
let txt = await driver.findComponent(BY.text("hello", MatchPattern.CONTAINS))
```

通过向`By.text()`方法传入第二个参数`MatchPattern.CONTAINS`来指定文本匹配规则；默认规则是`MatchPattern.EQUALS`，即目标控件text属性必须严格等于给定值。

####  控件相对定位

**示例代码1**：查找位于文本控件`Item3_3`后面的，id是`ResourceTable.Id_switch`的Switch控件。

```javascript
let switch = await driver.findComponent(BY.id(Id_switch).isAfter(BY.text("Item3_3")))
```

通过`By.isAfter`方法，指定位于目标控件前面的特征控件属性，通过该特征控件进行相对定位。一般地，特征控件是某个具有全局唯一特征的控件(例如具有唯一的id或者唯一的text)。


类似的，可以使用`By.isBefore`控件指定位于目标控件后面的特征控件属性，实现相对定位。

### UiComponent使用说明

`UiComponent`类代表了Ui界面上的一个控件，一般是通过`UiDriver.findComponent(by)`方法查找到的。通过该类的实例，用户可以获取控件属性，单击控件，滑动查找，注入文本等操作。

`UiComponent`包含的常用API：

| No.  | API                               | 功能描述                                     |
| ---- | --------------------------------- | -------------------------------------------- |
| 1    | click():Promise<void>             | 单击该控件                                   |
| 2    | inputText(t:string):Promise<void> | 向控件中输入文本(适用于文本框控件)           |
| 3    | scrollSearch(s:By):Promise<bool>  | 在该控件上滑动查找目标控件(适用于List等控件) |
| 4    | getText():Promise<string>         | 获取控件text                                 |
| 5    | getId():Promise<number>           | 获取控件id                                   |
| 6    | getType():Promise<string>         | 获取控件类型                                 |
| 7    | isEnabled():Promise<bool>         | 获取控件使能状态                             |

`UiComponent`完整的API列表请参考其API文档。

**示例代码1**：单击控件。

```javascript
let button = await driver.findComponent(BY.id(Id_button))
await button.click()
```

**示例代码2**：通过get接口获取控件属性后，可以使用单元测试框架提供的assert*接口做断言检查。

```javascript
let component = await driver.findComponent(BY.id(Id_title))
expect(component !== null).assertTrue()
```

**示例代码3**：在List控件中滑动查找text是`Item3_3`的子控件。

```javascript
let list = await driver.findComponent(BY.id(Id_list))
let found = await list.scrollSearch(BY.text("Item3_3"))
expect(found).assertTrue()
```

**示例代码4**：向输入框控件中输入文本。

```javascript
let editText = await driver.findComponent(BY.type('InputText'))
await editText.inputText("user_name")
```
### 使用方式

  开发者可以下载Deveco Studio创建测试工程后，在其中调用框架提供接口进行相关测试操作，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001263160453)。
> UI测试框架使能需要执行如下命令。
>```shell
> hdc_std shell param set persist.ace.testmode.enabled 1
>```
### UI测试框架自构建方式

> Ui测试框架在OpenHarmony-3.1-Release版本中未随版本编译，需手动处理，具体指导请[参考](https://gitee.com/openharmony/arkXtest/blob/OpenHarmony-3.1-Release/README_zh.md#%E6%8E%A8%E9%80%81ui%E6%B5%8B%E8%AF%95%E6%A1%86%E6%9E%B6%E8%87%B3%E8%AE%BE%E5%A4%87)。

#### 构建命令

```shell
./build.sh --product-name rk3568 --build-target uitestkit
```
#### 推送位置

```shell
hdc_std target mount
hdc_std shell mount -o rw,remount /
hdc_std file send uitest /system/bin/uitest
hdc_std file send libuitest.z.so /system/lib/module/libuitest.z.so
hdc_std shell chmod +x /system/bin/uitest
```
