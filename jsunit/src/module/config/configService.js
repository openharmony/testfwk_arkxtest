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

import { ClassFilter, NotClassFilter, SuiteAndItNameFilter, TestTypesFilter, NestFilter } from './Filter';
import { TAG, TESTTYPE, LEVEL, SIZE, KEYSET } from '../../Constant';
const STRESS_RULE = /^[1-9]\d*$/;

class ConfigService {
    constructor(attr) {
        this.id = attr.id;
        this.supportAsync = true; // 默认异步处理测试用例
        this.random = false;
        this.filterValid = [];
        this.filter = 0;
        this.flag = false;
        this.suite = null;
        this.itName = null;
        this.testType = null;
        this.level = null;
        this.size = null;
        this.class = null;
        this.notClass = null;
        this.timeout = null;
        // 遇错即停模式配置
        this.breakOnError = false;
        // 压力测试配置
        this.stress = null;
        this.skipMessage = false;
        this.runSkipped = '';
        this.filterXdescribe = [];
        this.testTag = [];
    }

    init(coreContext) {
        this.coreContext = coreContext;
    }

    isNormalInteger(str) {
        const n = Math.floor(Number(str));
        return n !== Infinity && String(n) === String(str) && n >= 0;
    }


    getStress() {
        if (this.stress === undefined || this.stress === '' || this.stress === null) {
            return 1;
        }
        return !this.stress.match(STRESS_RULE) ? 1 : Number.parseInt(this.stress);
    }

    basicParamValidCheck(params) {
        let size = params.size;
        if (size !== undefined && size !== '' && size !== null) {
            let sizeArray = ['small', 'medium', 'large'];
            if (sizeArray.indexOf(size) === -1) {
                this.filterValid.push('size:' + size);
            }
        }
        let level = params.level;
        if (level !== undefined && level !== '' && level !== null) {
            let levelArray = ['0', '1', '2', '3', '4'];
            if (levelArray.indexOf(level) === -1) {
                this.filterValid.push('level:' + level);
            }
        }
        let testType = params.testType;
        if (testType !== undefined && testType !== '' && testType !== null) {
            let testTypeArray = ['function', 'performance', 'power', 'reliability', 'security',
                'global', 'compatibility', 'user', 'standard', 'safety', 'resilience'];
            if (testTypeArray.indexOf(testType) === -1) {
                this.filterValid.push('testType:' + testType);
            }
        }
    }

    filterParamValidCheck(params) {
        let timeout = params.timeout;
        if (timeout !== undefined && timeout !== '' && timeout !== null) {
            if (!this.isNormalInteger(timeout)) {
                this.filterValid.push('timeout:' + timeout);
            }
        }

        let paramKeys = ['dryRun', 'random', 'breakOnError', 'coverage', 'skipMessage'];
        for (const key of paramKeys) {
            if (params[key] !== undefined && params[key] !== 'true' && params[key] !== 'false') {
                this.filterValid.push(`${key}:${params[key]}`);
            }
        }

        // 压力测试参数验证,正整数
        if (params.stress !== undefined && params.stress !== '' && params.stress !== null) {
            if (!params.stress.match(STRESS_RULE)) {
                this.filterValid.push('stress:' + params.stress);
            }
        }

        let nameRule = /^[A-Za-z]{1}[\w#,.]*$/;
        let paramClassKeys = ['class', 'notClass'];
        for (const key of paramClassKeys) {
            if (params[key] !== undefined && params[key] !== '' && params[key] !== null) {
                let classArray = params[key].split(',');
                classArray.forEach(item => !item.match(nameRule) ? this.filterValid.push(`${key}:${params[key]}`) : null);
            }
        }
    }

    setTestTag(tagStr) {
        console.info(`[Tag] setTestTag input: "${tagStr}"`);
        // 情况1：未传入标签 或 传入空字符串（含纯空白）
        if (tagStr === undefined || tagStr.trim() === '') {
            console.info('[Tag] Empty or undefined tag provided. Will run ALL test cases.');
            this.testTag = []; // 清空标签列表，表示不过滤
            return;
        }

        // 定义合法字符正则：只允许字母、数字、空格、逗号、加号
        // 注意：不允许多余符号如 @、#、|、: 等（| 是用例端使用的分隔符，命令行不用）
        const regex = new RegExp('^[a-zA-Z0-9, +]*$', 'g');
        if (!regex.test(tagStr)) {
            throw new Error(
                `[Tag] Invalid characters detected in tag string: "${tagStr}". ` +
                    `Allowed characters: letters, digits, space, comma (,), plus (+). ` +
                    `Ignoring tag filter and running ALL test cases.`
            );
        }
        // 按逗号分割，得到多个“条件组”
        const commaTags = tagStr.split(',')
            .map(item => item.trim())
            .filter(item => item !== '');
        console.info(`[Tag] Split by ',': ${JSON.stringify(commaTags)}`);
        // 如果分割后没有有效条件，也视为无过滤
        if (commaTags.length === 0) {
            console.info('[Tag] No valid tag conditions after parsing. Running all test cases.');
            this.testTag = [];
            return;
        }
        for (let item of commaTags) {
            if (item.includes('+')) {
                const andTags = item
                    .split('+')
                    .map(item => item.trim())
                    .filter(item => item !== '');
                // 只有非空 AND 组才加入
                if (andTags.length > 0) {
                    console.info(`[Tag] Detected AND condition group: ${JSON.stringify(andTags)}`);
                    this.testTag.push(andTags);
                }
            } else {
                // 不含 '+' → 单个 OR 标签
                console.info(`[Tag] Detected OR tag: "${item}"`);
                this.testTag.push(item)
            }
        }
    }

    setConfig(params) {
        this.basicParamValidCheck(params);
        this.filterParamValidCheck(params);
        try {
            this.class = params.class;
            this.notClass = params.notClass;
            this.flag = params.flag || { flag: false };
            this.suite = params.suite;
            this.itName = params.itName;
            this.filter = params.filter;
            this.testType = params.testType;
            this.level = params.level;
            this.size = params.size;
            this.timeout = params.timeout;
            this.dryRun = params.dryRun;
            this.breakOnError = params.breakOnError;
            this.random = params.random === 'true' ? true : false;
            this.stress = params.stress;
            this.coverage = params.coverage;
            this.skipMessage = params.skipMessage;
            this.runSkipped = params.runSkipped;
            this.filterParam = {
                testType: TESTTYPE,
                level: LEVEL,
                size: SIZE
            };
            this.parseParams();
            this.setTestTag(params.tag);
        } catch (err) {
            console.info(`${TAG}setConfig error: ${err.message}`);
        }
    }

    parseParams() {
        if (this.filter != null) {
            return;
        }
        let testTypeFilter = 0;
        let sizeFilter = 0;
        let levelFilter = 0;
        if (this.testType != null) {
            testTypeFilter = this.testType.split(',')
                .map(item => this.filterParam.testType[item] || 0)
                .reduce((pre, cur) => pre | cur, 0);
        }
        if (this.level != null) {
            levelFilter = this.level.split(',')
                .map(item => this.filterParam.level[item] || 0)
                .reduce((pre, cur) => pre | cur, 0);
        }
        if (this.size != null) {
            sizeFilter = this.size.split(',')
                .map(item => this.filterParam.size[item] || 0)
                .reduce((pre, cur) => pre | cur, 0);
        }
        this.filter = testTypeFilter | sizeFilter | levelFilter;
        console.info(`${TAG}filter params:${this.filter}`);
    }

    isCurrentSuite(description) {
        if (this.suite !== undefined && this.suite !== '' && this.suite !== null) {
            let suiteArray = this.suite.split(',');
            return suiteArray.indexOf(description) !== -1;
        }
        return false;
    }

    filterSuite(currentSuiteName) {
        let filterArray = [];
        if (this.suite !== undefined && this.suite !== '' && this.suite !== null) {
            filterArray.push(new SuiteAndItNameFilter(currentSuiteName, '', this.suite));
        }
        if (this.class !== undefined && this.class !== '' && this.class !== null) {
            filterArray.push(new ClassFilter(currentSuiteName, '', this.class));
        }
        if (this.notClass !== undefined && this.notClass !== '' && this.notClass !== null) {
            filterArray.push(new NotClassFilter(currentSuiteName, '', this.notClass));
        }

        let result = filterArray.map(item => item.filterSuite()).reduce((pre, cur) => pre || cur, false);
        return result;
    }

    filterDesc(currentSuiteName, desc, fi, coreContext) {
        let filterArray = [];
        if (this.itName !== undefined && this.itName !== '' && this.itName !== null) {
            filterArray.push(new SuiteAndItNameFilter(currentSuiteName, desc, this.itName));
        }
        if (this.class !== undefined && this.class !== '' && this.class !== null) {
            filterArray.push(new ClassFilter(currentSuiteName, desc, this.class));
        }
        if (this.notClass !== undefined && this.notClass !== '' && this.notClass !== null) {
            filterArray.push(new NotClassFilter(currentSuiteName, desc, this.notClass));
        }
        if (typeof (this.filter) !== 'undefined' && this.filter !== 0 && fi !== 0) {
            filterArray.push(new TestTypesFilter('', '', fi, this.filter));
        }
        let result = filterArray.map(item => item.filterIt()).reduce((pre, cur) => pre || cur, false);
        return result;
    }

    filterWithNest(desc, filter) {
        let filterArray = [];
        const nestFilter = new NestFilter();
        const targetSuiteArray = this.coreContext.getDefaultService('suite').targetSuiteArray;
        const targetSpecArray = this.coreContext.getDefaultService('suite').targetSpecArray;
        const suiteStack = this.coreContext.getDefaultService('suite').suitesStack;
        let isFilter = nestFilter.filterNestName(targetSuiteArray, targetSpecArray, suiteStack, desc);
        const isFullRun = this.coreContext.getDefaultService('suite').fullRun;
        if (typeof (this.filter) !== 'undefined' && this.filter !== 0 && filter !== 0) {
            filterArray.push(new TestTypesFilter('', '', filter, this.filter));
            return filterArray.map(item => item.filterIt()).reduce((pre, cur) => pre || cur, false);
        }
        if (isFilter && !isFullRun) {
            return true;
        }
        return nestFilter.filterNotClass(this.notClass, suiteStack, desc);

    }

    filterWithTestTag(itTestTag) {
        // 情况1：未配置任何有效标签 → 不过滤，执行所有用例
        if (this.testTag.length === 0) {
            return false;
        }
        // 情况2：用例未声明标签（itTestTag 为 undefined）
        if (itTestTag === undefined) {
            console.warn('[Tag] Test case has no tag, but global tag filter is active. Skipping this test case.');
            return true;
        }
        // 定义用例标签合法字符：字母、数字、空格、竖线 |
        const regex = new RegExp('^[a-zA-Z0-9 |]*$', 'g');
        let itTagArray = [];
        // 验证并解析用例标签
        if (typeof itTestTag === 'string' && regex.test(itTestTag)) {
            itTagArray = itTestTag.split('|')
                .map(item => item.trim())
                .filter(item => item !== '');
        } else {
            // 用例标签格式非法（如包含 @、, 等）
            console.warn(`[Tag] Invalid format in test case tag: "${itTestTag}". Skipping this test case.`);
            return true; // 保守策略：无法识别标签 → 跳过
        }
        console.info(
            `[Tag] Parsed test case tags: ${JSON.stringify(itTagArray)}, ` +
                `Global filter rules: ${JSON.stringify(this.testTag)}`
        );
        for (const tag of this.testTag) {
            if (!tag) {
                continue;
            }

            // AND 条件：要求用例标签包含数组中所有项
            if (Array.isArray(tag)) {
                const stringArray = tag
                if (stringArray.every(item => itTagArray.includes(item))) {
                    console.info(`[Tag] Matched AND condition: ${JSON.stringify(tag)}`);
                    return false;
                }
            } else if (typeof tag === 'string') {
                // OR 条件：只要用例标签包含该字符串即可
                const stringTag = tag
                if (itTagArray.includes(stringTag)) {
                    console.info(`[Tag] Matched OR tag: "${tag}"`);
                    return false;
                }
            }
        }
        console.info(`[FILTER] No condition matched. Skipping this test case`);
        return true;
    }

    isRandom() {
        return this.random || false;
    }

    isBreakOnError() {
        return this.breakOnError !== 'true' ? false : true;
    }

    setSupportAsync(value) {
        this.supportAsync = value;
    }

    isSupportAsync() {
        return this.supportAsync;
    }

    translateParams(parameters) {
        const keySet = new Set(KEYSET);
        let targetParams = {};
        for (const [key, param] of Object.entries(parameters)) {
            if (keySet.has(key)) {
                var newKey = key.replace('-s ', '');
                targetParams[newKey] = param;
            }
        }
        return targetParams;
    }
    translateParamsToString(parameters) {
        const keySet = new Set(KEYSET);
        let targetParams = '';
        for (const [key, param] of Object.entries(parameters)) {
            if (keySet.has(key)) {
                targetParams += ' ' + key + ' ' + param;
            }
        }
        return targetParams.trim();
    }

    execute() {
    }

    checkIfSuiteInSkipRun(desc) {
        return this.runSkipped.split(',').some(item => {
            return item === desc || item.startsWith(desc + '.') || item.startsWith(desc + '#') || desc.startsWith(item + '.') || this.runSkipped === 'skipped';
        });
    }

    checkIfSpecInSkipRun(desc) {
        return this.runSkipped.split(',').some(item => {
            if (item.includes('#')) {
                return item === desc;
            } else {
                return desc.startsWith(item + '.') || desc.startsWith(item + '#') || this.runSkipped === 'skipped';
            }
        }
        );
    }
}

export {
    ConfigService
};
