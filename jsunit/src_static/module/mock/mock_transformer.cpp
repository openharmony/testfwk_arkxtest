/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "c/abckit.h"
#include "c/extensions/arkts/metadata_arkts.h"
#include "c/ir_core.h"
#include "c/isa/isa_dynamic.h"
#include "c/isa/isa_static.h"
#include "c/metadata_core.h"
#include "c/statuses.h"

#include <fcntl.h>
#include <functional>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstring>
#include <memory>
using namespace std;

const static string MOCKKIT_MODULE_NAME = "entry.src.hypium.module.mock.MockKit";
const static string MOCKKIT_CLASS_NAME = "MockKit";
const static string MOCKKIT_FUNC_NAME =
    "mockFunc:" + MOCKKIT_MODULE_NAME + "." + MOCKKIT_CLASS_NAME +
    ";std.core.Object;std.core.Function;std.core.Function0;";
const static string STUB_FIELD_NAME = "__LocalMocker__";
const static string SETMOCKER_NAME = "__setMocker__";
const static int32_t TWO = 2;
const static int32_t THREE = 3;
const static int32_t FOUR = 4;

static auto g_impl = AbckitGetApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_implI = AbckitGetInspectApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_implArkI = AbckitGetArktsInspectApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_implM = AbckitGetModifyApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_implG = AbckitGetGraphApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_dynG = AbckitGetIsaApiDynamicImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_statG = AbckitGetIsaApiStaticImpl(ABCKIT_VERSION_RELEASE_1_0_0);
static auto g_implArkM = AbckitGetArktsModifyApiImpl(ABCKIT_VERSION_RELEASE_1_0_0);

static bool g_hasError = false;

static AbckitCoreClass *mockKitClass;
static AbckitType *mockKitType;
static AbckitCoreClass *noResultClass;
static AbckitType *noResultType;
static AbckitCoreClass *nullClass;
static AbckitType *nullType;
static AbckitCoreClass *objectClass;
static AbckitType *objectType;
static AbckitCoreClass *doubleClass;
static AbckitType *doubleType;
static AbckitArktsModule *stdCoreArktsModule;
static AbckitArktsModule *escompatArktsModule;

// Convert AbckitString to string
static std::string GetString(AbckitString *str)
{
    auto name = g_implI->abckitStringToString(str);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "GetString error: " << g_impl->getLastError() << std::endl;
        return "";
    }
    return string(name);
}

static bool StartsWith(std::string str, std::string prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

static bool EndsWith(std::string str, std::string suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Find the module with the specified name in the file
static AbckitCoreModule *FindModule(AbckitFile *file, string moduleName)
{
    AbckitCoreModule *targetModule;
    std::function<void(AbckitCoreModule *)> cbModule = [&](AbckitCoreModule *m) {
        if (GetString(g_implI->moduleGetName(m)) == moduleName) {
            targetModule = m;
        }
        return !g_hasError;
    };
    g_implI->fileEnumerateModules(file, &cbModule, [](AbckitCoreModule *m, void *cb) {
        return (*reinterpret_cast<std::function<bool(AbckitCoreModule *)> *>(cb))(m);
    });
    return targetModule;
}

// Find the class with the specified name in the file
static AbckitCoreClass *FindClass(AbckitFile *file, string moduleName, string className)
{
    AbckitCoreModule *targetModule = FindModule(file, moduleName);
    AbckitCoreClass *targetClass;
    if (targetModule == nullptr) {
        return targetClass;
    }
    std::function<void(AbckitCoreClass *)> cbClass = [&](AbckitCoreClass *c) {
        if (GetString(g_implI->classGetName(c)) == className) {
            targetClass = c;
        }
        return !g_hasError;
    };
    g_implI->moduleEnumerateClasses(targetModule, (void *)&cbClass, [](AbckitCoreClass *c, void *cb) {
        return (*reinterpret_cast<std::function<bool(AbckitCoreClass *)> *>(cb))(c);
    });
    return targetClass;
}

// Find the method with the specified name in the file
static AbckitCoreFunction *FindFunction(AbckitFile *file, string moduleName, string className, string functionName)
{
    AbckitCoreClass *targetClass = FindClass(file, moduleName, className);
    AbckitCoreFunction *targetFunction;
    if (targetClass == nullptr) {
        return targetFunction;
    }
    std::function<bool(AbckitCoreFunction *)> cbFunc = [&](AbckitCoreFunction *f) {
        if (GetString(g_implI->functionGetName(f)).find(functionName + ":" + moduleName + "." + className) == 0) {
            targetFunction = f;
        }
        return !g_hasError;
    };
    g_implI->classEnumerateMethods(targetClass, (void *)&cbFunc, [](AbckitCoreFunction *f, void *cb) {
        return (*reinterpret_cast<std::function<bool(AbckitCoreFunction *)> *>(cb))(f);
    });
    return targetFunction;
}

// Find the method with the specified name in the module
static AbckitCoreFunction *ClassFindFunction(AbckitCoreClass *targetClass, string functionName)
{
    AbckitCoreFunction *targetFunction = nullptr;
    if (targetClass == nullptr) {
        return targetFunction;
    }
    AbckitCoreModule *targetModule = g_implI->classGetModule(targetClass);
    string moduleName = GetString(g_implI->moduleGetName(targetModule));
    string className = GetString(g_implI->classGetName(targetClass));
    std::function<bool(AbckitCoreFunction *)> cbFunc = [&](AbckitCoreFunction *func) {
        if (GetString(g_implI->functionGetName(func)).find(functionName + ":" + moduleName + "." + className) == 0) {
            targetFunction = func;
        }
        return !g_hasError;
    };
    g_implI->classEnumerateMethods(targetClass, (void *)&cbFunc, [](AbckitCoreFunction *func, void *cb) {
        return (*reinterpret_cast<std::function<bool(AbckitCoreFunction *)> *>(cb))(func);
    });
    return targetFunction;
}

// Find the field with the specified name in the class
static string ClassFindFieldFullName(AbckitCoreClass *targetClass, string fieldName)
{
    if (targetClass == nullptr) {
        return "";
    }
    AbckitCoreModule *targetModule = g_implI->classGetModule(targetClass);
    string moduleName = GetString(g_implI->moduleGetName(targetModule));
    string className = GetString(g_implI->classGetName(targetClass));
    return moduleName + "." + className + "." + fieldName;
}

// Find the first instruction of the specified type
AbckitInst *FindFirstOperatorFuncInst(AbckitGraph *graph, AbckitIsaApiStaticOpcode opcode,
                                      const function<bool(AbckitInst *)> &findIf = [](AbckitInst *) { return true; })
{
    std::vector<AbckitBasicBlock *> bbs;
    g_implG->gVisitBlocksRpo(graph, &bbs, [](AbckitBasicBlock *bb, void *data) {
        reinterpret_cast<std::vector<AbckitBasicBlock *> *>(data)->emplace_back(bb);
        return true;
    });
    for (auto *bb : bbs) {
        auto *curInst = g_implG->bbGetFirstInst(bb);
        while (curInst != nullptr) {
            if (g_statG->iGetOpcode(curInst) == opcode && findIf(curInst)) {
                return curInst;
            }
            curInst = g_implG->iGetNext(curInst);
        }
    }
    return nullptr;
}

// Find instructions for specified types and conditions
static void FindOperatorFuncInsts(AbckitGraph *graph, vector<AbckitInst*>& mockFuncInsts,
                                  AbckitIsaApiStaticOpcode opcode,
                                  const function<bool(AbckitInst *)> &findIf = [](AbckitInst *) { return true; })
{
    std::vector<AbckitBasicBlock*> bbs;
    g_implG->gVisitBlocksRpo(graph, &bbs, [](AbckitBasicBlock *bb, void *data) {
        reinterpret_cast<std::vector<AbckitBasicBlock *> *>(data)->emplace_back(bb);
        return true;
    });
    for (auto *bb : bbs) {
        auto *curInst = g_implG->bbGetFirstInst(bb);
        while (curInst != nullptr) {
            if (g_statG->iGetOpcode(curInst) == opcode && findIf(curInst)) {
                mockFuncInsts.emplace_back(curInst);
            }
            curInst = g_implG->iGetNext(curInst);
        }
    }
}

// Parse the mocked method through specific instruction
static void GetMockedFunction(AbckitInst *mockedClassCtorInst, AbckitInst *lambdaClassCtorInst,
                              map<AbckitCoreFunction*, vector<AbckitCoreClass*>> &mockedFunctions)
{
    AbckitCoreFunction *mockedClassCtor = g_implG->iGetFunction(mockedClassCtorInst);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR || mockedClassCtor == nullptr) {
        std::cerr << "Get mockedClassCtor failed" << std::endl;
        return;
    }
    AbckitCoreClass *mockedClass = g_implI->functionGetParentClass(mockedClassCtor);
    auto mockedClassName = g_implI->classGetName(mockedClass);
    auto mockedClassNameStr = GetString(mockedClassName);
    AbckitCoreFunction *lambdaClassCtor = g_implG->iGetFunction(lambdaClassCtorInst);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR || lambdaClassCtor == nullptr) {
        std::cerr << "Get lambdaClassCtor failed" << std::endl;
        return;
    }
    AbckitCoreClass *lambdaClass = g_implI->functionGetParentClass(lambdaClassCtor);
    AbckitCoreFunction *lambdaInvokeFunc = ClassFindFunction(lambdaClass, "$_invoke");
    if (lambdaInvokeFunc == nullptr) {
        return;
    }
    AbckitGraph *graph = g_implI->createGraphFromFunction(lambdaInvokeFunc);
    std::vector<AbckitInst*> mockedFuncInsts;
    FindOperatorFuncInsts(graph, mockedFuncInsts, ABCKIT_ISA_API_STATIC_OPCODE_CALL_VIRTUAL);
    if (mockedFuncInsts.size() != 1) {
        return;
    }
    AbckitCoreFunction *calledVirtMethod = g_implG->iGetFunction(mockedFuncInsts[0]);
    AbckitCoreClass *calledVirtClass = g_implI->functionGetParentClass(calledVirtMethod);
    auto calledVirtClassName = g_implI->classGetName(calledVirtClass);
    auto calledVirtClassNameStr = GetString(calledVirtClassName);
    if (calledVirtClassNameStr != mockedClassNameStr) {
        return;
    }
    vector<AbckitCoreClass*> lambdaClasses;
    auto it = mockedFunctions.find(calledVirtMethod);
    if (it != mockedFunctions.end()) {
        lambdaClasses = it->second;
    }
    lambdaClasses.push_back(lambdaClass);
    mockedFunctions[calledVirtMethod] = lambdaClasses;
}

// Scan and search for mocked methods
static map<AbckitCoreFunction*, vector<AbckitCoreClass*>> ScanMockFunctions(AbckitFile *file)
{
    map<AbckitCoreFunction*, vector<AbckitCoreClass*>> mockedFunctions;
    std::function<bool(AbckitInst *)> findIf = [&](AbckitInst *callVirtInst) {
        AbckitCoreFunction *calledVirtMethod = g_implG->iGetFunction(callVirtInst);
        auto methodName = g_implI->functionGetName(calledVirtMethod);
        auto methodNameStr = GetString(methodName);
        return methodNameStr == MOCKKIT_FUNC_NAME;
    };
    std::function<bool(AbckitCoreFunction *)> cbFunc = [&](AbckitCoreFunction *func) {
        AbckitArktsFunction *arktsFunc = g_implArkI->coreFunctionToArktsFunction(func);
        if (g_implI->functionIsExternal(func) || g_implArkI->functionIsAbstract(arktsFunc) ||
            g_implArkI->functionIsNative(arktsFunc)) {
            std::cout << "Skip func:  " << GetString(g_implI->functionGetName(func)) << std::endl;
            return !g_hasError;
        }
        AbckitGraph *graph = g_implI->createGraphFromFunction(func);
        std::vector<AbckitInst*> mockFuncInsts;
        FindOperatorFuncInsts(graph, mockFuncInsts, ABCKIT_ISA_API_STATIC_OPCODE_CALL_VIRTUAL, findIf);
        for (auto mockFuncInst : mockFuncInsts) {
            uint32_t inputCount = g_implG->iGetInputCount(mockFuncInst);
            if (inputCount != THREE) {
                continue;
            }
            AbckitInst *mockedClassCtorInst = g_implG->iGetInput(mockFuncInst, 1);
            AbckitInst *lambdaClassCtorInst = g_implG->iGetInput(mockFuncInst, 2);
            GetMockedFunction(mockedClassCtorInst, lambdaClassCtorInst, mockedFunctions);
        }
        g_impl->destroyGraph(graph);
        return !g_hasError;
    };
    std::function<void(AbckitCoreModule *)> cbModule = [&](AbckitCoreModule *mod) {
        if (!EndsWith(GetString(g_implI->moduleGetName(mod)), ".test")) {
            return !g_hasError;
        }
        g_implI->moduleEnumerateTopLevelFunctions(mod, (void *)&cbFunc, [](AbckitCoreFunction *func, void *cb) {
            std::cout << "[Top FUNCTION] " << GetString(g_implI->functionGetName(func)) << std::endl;
            return (*reinterpret_cast<std::function<bool(AbckitCoreFunction *)> *>(cb))(func);
        });
        return !g_hasError;
    };
    g_implI->fileEnumerateModules(file, &cbModule, [](AbckitCoreModule *mod, void *cb) {
        return (*reinterpret_cast<std::function<bool(AbckitCoreModule *)> *>(cb))(mod);
    });
    return mockedFunctions;
}

// Initializer, search for native classes and create types
static void Initializer(AbckitFile *file)
{
    mockKitClass = FindClass(file, MOCKKIT_MODULE_NAME, MOCKKIT_CLASS_NAME);
    mockKitType = g_implM->createReferenceType(file, mockKitClass);
    AbckitCoreClass *cls1 = g_implI->typeGetReferenceClass(mockKitType);
    std::cout << "Create MockKit type: " << GetString(g_implI->classGetName(cls1)) << std::endl;

    string noResultClassName = "NoResult";
    noResultClass = FindClass(file, MOCKKIT_MODULE_NAME, noResultClassName);
    noResultType = g_implM->createReferenceType(file, noResultClass);
    AbckitCoreClass *cls2 = g_implI->typeGetReferenceClass(noResultType);
    std::cout << "Create NoResult type: " << GetString(g_implI->classGetName(cls2)) << std::endl;

    string coreMoudleName = "std.core";
    string nullClassName = "Null";
    nullClass = FindClass(file, coreMoudleName, nullClassName);
    const char *nullTypeName = "std.core.Null";
    nullType = g_implM->createType(file, ABCKIT_TYPE_ID_REFERENCE);
    g_implM->typeSetName(nullType, nullTypeName, strlen(nullTypeName));

    string objectClassName = "Object";
    objectClass = FindClass(file, coreMoudleName, objectClassName);
    objectType = g_implM->createReferenceType(file, objectClass);
    AbckitCoreClass *cls4 = g_implI->typeGetReferenceClass(objectType);
    std::cout << "Create Object type: " << GetString(g_implI->classGetName(cls4)) << std::endl;

    string doubleClassName = "Double";
    doubleClass = FindClass(file, coreMoudleName, doubleClassName);
    doubleType = g_implM->createReferenceType(file, doubleClass);
    AbckitCoreClass *cls5 = g_implI->typeGetReferenceClass(doubleType);
    std::cout << "Create Double type: " << GetString(g_implI->classGetName(cls5)) << std::endl;

    string stdCoreModuleName = "std.core";
    AbckitCoreModule *stdCoreModule = FindModule(file, stdCoreModuleName);
    stdCoreArktsModule = g_implArkI->coreModuleToArktsModule(stdCoreModule);

    string escompatModuleName = "escompat";
    AbckitCoreModule *escompatModule = FindModule(file, escompatModuleName);
    escompatArktsModule = g_implArkI->coreModuleToArktsModule(escompatModule);
}

static string PraseFunctionName(string originalFunctionName)
{
    size_t pos1 = originalFunctionName.rfind(":");
    if (pos1 == std::string::npos) {
        return "";
    }
    return originalFunctionName.substr(0, pos1);
}

static void AnnotationModify(AbckitFile *file, vector<AbckitCoreClass*> lambdaClasses,
                             AbckitArktsFunction *arktsFunction)
{
    AbckitCoreFunction *coreFunction = g_implArkI->arktsFunctionToCoreFunction(arktsFunction);
    for (auto lambdaClass : lambdaClasses) {
        cout << "AnnotationModify lambdaClass = " << GetString(g_implI->classGetName(lambdaClass)) << endl;
        AbckitCoreAnnotation *targetAnno;
        std::function<void(AbckitCoreAnnotationElement *)> cbAnnoElem = [&](AbckitCoreAnnotationElement *annoElem) {
            auto arktsAnno = g_implArkI->coreAnnotationToArktsAnnotation(targetAnno);
            auto arktsAnnoElem = g_implArkI->coreAnnotationElementToArktsAnnotationElement(annoElem);
            g_implArkM->annotationRemoveAnnotationElement(arktsAnno, arktsAnnoElem);
            struct AbckitArktsAnnotationElementCreateParams annoElementCreateParams {};
            auto value = g_implM->createValueMethod(file, coreFunction);
            annoElementCreateParams.value = value;
            annoElementCreateParams.name = "value";
            g_implArkM->annotationAddAnnotationElement(arktsAnno, &annoElementCreateParams);
            return !g_hasError;
        };
        std::function<void(AbckitCoreAnnotation *)> cbAnno = [&](AbckitCoreAnnotation *anno) {
            targetAnno = anno;
            if (GetString(g_implI->annotationGetName(anno)) != "FunctionalReference") {
                return !g_hasError;
            }
            g_implI->annotationEnumerateElements(anno, &cbAnnoElem,
                [](AbckitCoreAnnotationElement *annoElem, void *cb) {
                return (*reinterpret_cast<std::function<bool(AbckitCoreAnnotationElement *)> *>(cb))(annoElem);
            });
            return !g_hasError;
        };
        g_implI->classEnumerateAnnotations(lambdaClass, &cbAnno, [](AbckitCoreAnnotation *anno, void *cb) {
            return (*reinterpret_cast<std::function<bool(AbckitCoreAnnotation *)> *>(cb))(anno);
        });
    }
}

// Insert CheckLocalMocker block in the mocked method
static AbckitBasicBlock *CheckLocalMockerBlock(AbckitFile *file, AbckitGraph *graph, AbckitCoreClass *mockedClass,
                                               AbckitInst *&ldobjInst)
{
    AbckitInst *classInst = FindFirstOperatorFuncInst(graph, ABCKIT_ISA_API_STATIC_OPCODE_PARAMETER);
    AbckitBasicBlock *checkBlock = g_implG->bbCreateEmpty(graph);
    string localMockerFullName = ClassFindFieldFullName(mockedClass, STUB_FIELD_NAME);
    if (localMockerFullName == "") {
        std::cerr << "Cannot get LocalMocker field" << std::endl;
        return nullptr;
    }
    AbckitString *localMockerStr = g_implM->createString(file, localMockerFullName.c_str(), localMockerFullName.size());
    ldobjInst = g_statG->iCreateLoadObject(graph, classInst, localMockerStr, ABCKIT_TYPE_ID_REFERENCE);
    if (ldobjInst == nullptr) {
        std::cerr << "Cannot get LocalMocker object" << std::endl;
        return nullptr;
    }
    g_implG->bbAddInstBack(checkBlock, ldobjInst);
    AbckitInst *loadNullValueInst = g_statG->iCreateLoadNullValue(graph);
    g_implG->bbAddInstBack(checkBlock, loadNullValueInst);
    AbckitInst *ifInst = g_statG->iCreateIf(graph, ldobjInst, loadNullValueInst,
        AbckitIsaApiStaticConditionCode::ABCKIT_ISA_API_STATIC_CONDITION_CODE_CC_EQ);
    g_implG->bbAddInstBack(checkBlock, ifInst);
    return checkBlock;
}

static AbckitInst *ObjectTypePackage(AbckitGraph *graph, AbckitInst *functionParam, AbckitBasicBlock *callBlock)
{
    AbckitType *paramType = g_implG->iGetType(functionParam);
    AbckitTypeId paramTypeId = g_implI->typeGetTypeId(paramType);
    AbckitArktsFunction *ctorArktsFunc;
    bool isPackaged = true;
    if (paramTypeId == ABCKIT_TYPE_ID_U1) {
        const char* param[] = {"u1"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Boolean", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_I32) {
        const char* param[] = {"i32"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Int", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_I64) {
        const char* param[] = {"i64"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Long", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_F64) {
        const char* param[] = {"f64"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Double", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_I16) {
        const char* param[] = {"i16"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Short", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_F32) {
        const char* param[] = {"f32"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Float", "_ctor_", "void", param, 1);
    } else if (paramTypeId == ABCKIT_TYPE_ID_U16) {
        const char* param[] = {"u16"};
        ctorArktsFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(
            stdCoreArktsModule, "Char", "_ctor_", "void", param, 1);
    } else {
        isPackaged = false;
    }
    if (isPackaged) {
        AbckitCoreFunction *ctorCoreFunc = g_implArkI->arktsFunctionToCoreFunction(ctorArktsFunc);
        AbckitInst *callCtorInst = g_statG->iCreateInitObject(graph, ctorCoreFunc, 1, functionParam);
        g_implG->bbAddInstBack(callBlock, callCtorInst);
        return callCtorInst;
    }
    return functionParam;
}

// Insert CallGetReturnInfo block in the mocked method
static AbckitBasicBlock *CallGetReturnInfoBlock(AbckitGraph *graph, vector<AbckitInst*> functionParams,
                                                vector<AbckitInst*> nameStrInsts, AbckitInst *ldobjInst,
                                                AbckitInst *&callGetReturnInfoInst)
{
    AbckitBasicBlock *callBlock = g_implG->bbCreateEmpty(graph);
    AbckitInst *checkCastInst = g_statG->iCreateCheckCast(graph, ldobjInst, mockKitType);
    g_implG->bbAddInstBack(callBlock, checkCastInst);
    g_implG->bbAddInstBack(callBlock, nameStrInsts[0]);
    g_implG->bbAddInstBack(callBlock, nameStrInsts[1]);
    AbckitInst *paramLengthInst = g_implG->gFindOrCreateConstantI32(graph, functionParams.size() - 1);
    const char *arrayCtorParams[] = {"i32"};
    auto arrayCtorFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(escompatArktsModule, "Array", "_ctor_",
                                                                                 "void", arrayCtorParams, 1);
    auto arrayCtorCoreFunc = g_implArkI->arktsFunctionToCoreFunction(arrayCtorFunc);
    auto callArrayCtorInst = g_statG->iCreateInitObject(graph, arrayCtorCoreFunc, 1, paramLengthInst);
    g_implG->bbAddInstBack(callBlock, callArrayCtorInst);
    for (int32_t paramIndex = 0; paramIndex < functionParams.size() -1; paramIndex++) {
        const char *arraySetParams[] = {"i32", "std.core.Object"};
        auto arraySetFunc = g_implArkM->moduleImportClassMethodFromArktsV2ToArktsV2(escompatArktsModule, "Array",
                                                                                    "$_set", "void", arraySetParams, 2);
        AbckitCoreFunction *arraySetCoreFunc = g_implArkI->arktsFunctionToCoreFunction(arraySetFunc);
        AbckitInst *paramIndexInst = g_implG->gFindOrCreateConstantI32(graph, paramIndex);
        AbckitTypeId objectTypeId = g_implI->typeGetTypeId(objectType);
        AbckitInst *paramObjectInst = ObjectTypePackage(graph, functionParams[paramIndex + 1], callBlock);
        AbckitInst *callArraySetInst = g_statG->iCreateCallVirtual(graph, callArrayCtorInst, arraySetCoreFunc, TWO,
                                                                   paramIndexInst, paramObjectInst);
        g_implG->bbAddInstBack(callBlock, paramIndexInst);
        g_implG->bbAddInstBack(callBlock, callArraySetInst);
    }

    AbckitCoreFunction *getReturnInfoFunc = ClassFindFunction(mockKitClass, "getReturnInfo");
    callGetReturnInfoInst = g_statG->iCreateCallVirtual(graph, ldobjInst, getReturnInfoFunc, FOUR, functionParams[0],
                                                        nameStrInsts[0], nameStrInsts[1], callArrayCtorInst);
    g_implG->bbAddInstBack(callBlock, callGetReturnInfoInst);
    AbckitInst *isNoResultInst = g_statG->iCreateIsInstance(graph, callGetReturnInfoInst, noResultType);
    g_implG->bbAddInstBack(callBlock, isNoResultInst);
    AbckitInst *zeroInst = g_implG->gFindOrCreateConstantI32(graph, 0);
    AbckitInst *ifInst = g_statG->iCreateIf(graph, isNoResultInst, zeroInst,
        AbckitIsaApiStaticConditionCode::ABCKIT_ISA_API_STATIC_CONDITION_CODE_CC_NE);
    g_implG->bbAddInstBack(callBlock, ifInst);
    return callBlock;
}

// Insert MockedReturn block in the mocked method
static AbckitBasicBlock *MockedReturnBlock(AbckitGraph *graph, AbckitInst *callGetReturnInfoInst)
{
    AbckitBasicBlock *returnBlock = g_implG->bbCreateEmpty(graph);
    AbckitInst *returnMockValueInst =  g_statG->iCreateReturn(graph, callGetReturnInfoInst);
    g_implG->bbAddInstBack(returnBlock, returnMockValueInst);
    return returnBlock;
}

// Modify the graph of the mocked method
static AbckitGraph *GraphModify(AbckitFile *file, AbckitGraph *graph, AbckitCoreClass *mockedClass,
                                string className, string functionName)
{
    std::cout << g_implG->gGetNumberOfParameters(graph) << endl;
    vector<AbckitInst*> functionParams;
    for (int paramIndex = 0; paramIndex < g_implG->gGetNumberOfParameters(graph); paramIndex++) {
        AbckitInst *param = g_implG->gGetParameter(graph, paramIndex);
        functionParams.push_back(param);
    }
    AbckitBasicBlock *startBlock = g_implG->gGetStartBasicBlock(graph);
    AbckitBasicBlock *endBlock = g_implG->gGetEndBasicBlock(graph);
    AbckitBasicBlock *afterStartBlock = g_implG->bbGetSuccBlock(startBlock, 0);
    AbckitInst *ldobjInst = nullptr;
    AbckitBasicBlock *checkBlock = CheckLocalMockerBlock(file, graph, mockedClass, ldobjInst);
    if (checkBlock == nullptr || ldobjInst == nullptr) {
        return nullptr;
    }
    g_implG->bbDisconnectSuccBlock(startBlock, 0);
    g_implG->bbInsertSuccBlock(startBlock, checkBlock, 0);
    g_implG->bbInsertSuccBlock(checkBlock, afterStartBlock, 0);

    AbckitInst *classNameStrInst = g_statG->iCreateLoadString(graph,
        g_implM->createString(file, className.c_str(), className.length()));
    AbckitInst *functionNameStrInst = g_statG->iCreateLoadString(graph,
        g_implM->createString(file, functionName.c_str(), functionName.length()));
    vector<AbckitInst*> nameStrInsts;
    nameStrInsts.push_back(classNameStrInst);
    nameStrInsts.push_back(functionNameStrInst);
    AbckitInst *callGetReturnInfoInst = nullptr;
    AbckitBasicBlock *callBlock = CallGetReturnInfoBlock(graph, functionParams, nameStrInsts, ldobjInst,
                                                         callGetReturnInfoInst);
    if (callBlock == nullptr || callGetReturnInfoInst == nullptr) {
        return nullptr;
    }
    g_implG->bbInsertSuccBlock(checkBlock, callBlock, 1);
    g_implG->bbInsertSuccBlock(callBlock, afterStartBlock, 0);
    
    AbckitBasicBlock *returnBlock = MockedReturnBlock(graph, callGetReturnInfoInst);
    if (returnBlock == nullptr) {
        return nullptr;
    }
    g_implG->bbInsertSuccBlock(callBlock, returnBlock, 1);
    g_implG->bbInsertSuccBlock(returnBlock, endBlock, 0);
    return graph;
}

// Stub in the mocked method
static void FunctionStub(AbckitFile *file, AbckitCoreFunction *mockedFunction, AbckitCoreClass *mockedClass,
                         vector<AbckitCoreClass*> lambdaClasses)
{
    // Change the return value type to union type
    auto *unionTypes = new AbckitType *[2];
    const char *typeName0 = "std.core.Object";
    unionTypes[0] = g_implM->createType(file, ABCKIT_TYPE_ID_REFERENCE);
    g_implM->typeSetName(unionTypes[0], typeName0, strlen(typeName0));
    const char *typeName1 = "std.core.Null";
    unionTypes[1] = g_implM->createType(file, ABCKIT_TYPE_ID_REFERENCE);
    g_implM->typeSetName(unionTypes[1], typeName1, strlen(typeName1));
    AbckitType *returnUnionType = g_implM->createUnionType(file, unionTypes, 2);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "Create return union type failed" << std::endl;
        return;
    }
    AbckitArktsFunction *arktsFunction = g_implArkI->coreFunctionToArktsFunction(mockedFunction);
    if (arktsFunction == nullptr) {
        std::cerr << "Get arktsFunction failed" << std::endl;
        return;
    }
    g_implArkM->functionSetReturnType(arktsFunction, returnUnionType);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "Cannot set return union type of mocked function" << std::endl;
        return;
    }
    AnnotationModify(file, lambdaClasses, arktsFunction);
    AbckitGraph *graph = g_implI->createGraphFromFunction(mockedFunction);
    string className = GetString(g_implI->classGetName(mockedClass));
    string functionName = PraseFunctionName(GetString(g_implI->functionGetName(mockedFunction)));
    AbckitGraph *modifiedGraph = GraphModify(file, graph, mockedClass, className, functionName);
    if (modifiedGraph == nullptr) {
        cout << "modifiedGraph is nullptr" << endl;
        return;
    }
    g_implM->functionSetGraph(mockedFunction, modifiedGraph);
    g_impl->destroyGraph(graph);
    cout << functionName << " has beened stubbed" << endl;
}

// Add '__setMocker__' method to the mocked class and stub in
static void SetMockerStub(AbckitFile *file, AbckitCoreClass *mockedClass)
{
    // Add method
    struct ArktsMethodCreateParams methodParams;
    methodParams.name = SETMOCKER_NAME.data();
    methodParams.params = nullptr;
    methodParams.returnType = g_implM->createType(file, ABCKIT_TYPE_ID_VOID);
    methodParams.isStatic = false;
    methodParams.isAsync = false;
    methodParams.methodVisibility = ABCKIT_ARKTS_METHOD_VISIBILITY_PUBLIC;
    AbckitArktsClass *mockedArktsClass = g_implArkI->coreClassToArktsClass(mockedClass);
    AbckitArktsFunction *arktsSetMocker = g_implArkM->classAddMethod(mockedArktsClass, &methodParams);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "Create class method failed" << std::endl;
        return;
    }
    // Add parameters to the method
    auto arktsParamCreatedParams = std::make_unique<AbckitArktsFunctionParamCreatedParams>();
    arktsParamCreatedParams->name = "mocker";
    arktsParamCreatedParams->type = mockKitType;
    AbckitArktsFunctionParam *arktsParam = g_implArkM->functionCreateParameter(file, arktsParamCreatedParams.get());
    g_implArkM->functionAddParameter(arktsSetMocker, arktsParam);

    AbckitCoreFunction *setMocker = g_implArkI->arktsFunctionToCoreFunction(arktsSetMocker);
    AbckitGraph *graph = g_implI->createGraphFromFunction(setMocker);
    if (g_implG->gGetNumberOfParameters(graph) != TWO) {
        std::cerr << "Param count of method setMocker is invalid: " << std::endl;
        return;
    }
    AbckitInst *paramInst = g_implG->gGetParameter(graph, 1);
    string localMockerFullName = ClassFindFieldFullName(mockedClass, STUB_FIELD_NAME);
    if (localMockerFullName == "") {
        std::cerr << "Cannot get LocalMocker field" << std::endl;
        return;
    }
    AbckitString *localMockerStr = g_implM->createString(file, localMockerFullName.c_str(), localMockerFullName.size());
    AbckitInst *classInst = FindFirstOperatorFuncInst(graph, ABCKIT_ISA_API_STATIC_OPCODE_PARAMETER);
    AbckitInst *stobj = g_statG->iCreateStoreObject(graph, classInst, localMockerStr, paramInst,
                                                    ABCKIT_TYPE_ID_REFERENCE);
    AbckitInst *originalRet = FindFirstOperatorFuncInst(graph, ABCKIT_ISA_API_STATIC_OPCODE_RETURN_VOID);
    g_implG->iInsertBefore(stobj, originalRet);
    g_implM->functionSetGraph(setMocker, graph);
    g_impl->destroyGraph(graph);
}

// Stub in _Ctor_ of the mocked class
static void CtorFuncStub(AbckitFile *file, AbckitCoreClass *mockedClass)
{
    AbckitCoreFunction *ctorFunc = ClassFindFunction(mockedClass, "_ctor_");
    if (ctorFunc == nullptr) {
        std::cerr << "Ctor function of mockedClass is not found" << std::endl;
        return;
    }
    AbckitGraph *graph = g_implI->createGraphFromFunction(ctorFunc);
    AbckitInst *nullInst = g_statG->iCreateLoadNullValue(graph);
    string localMockerFullName = ClassFindFieldFullName(mockedClass, STUB_FIELD_NAME);
    if (localMockerFullName == "") {
        std::cerr << "Cannot get LocalMocker field" << std::endl;
        return;
    }
    AbckitString *localMockerStr = g_implM->createString(file, localMockerFullName.c_str(), localMockerFullName.size());
    AbckitInst *classInst = FindFirstOperatorFuncInst(graph, ABCKIT_ISA_API_STATIC_OPCODE_PARAMETER);
    AbckitInst *stobj = g_statG->iCreateStoreObject(graph, classInst, localMockerStr, nullInst,
                                                    ABCKIT_TYPE_ID_REFERENCE);
    AbckitInst *originalRet = FindFirstOperatorFuncInst(graph, ABCKIT_ISA_API_STATIC_OPCODE_RETURN_VOID);
    g_implG->iInsertBefore(stobj, originalRet);
    g_implG->iInsertBefore(nullInst, stobj);
    g_implM->functionSetGraph(ctorFunc, graph);
    g_impl->destroyGraph(graph);
}

// Add '__LocalMocker__' field to the mocked class
static void ClassFiledStub(AbckitFile *file, AbckitCoreClass *mockedClass)
{
    if (mockKitClass == nullptr || nullClass == nullptr) {
        return;
    }
    auto *unionTypes = new AbckitType *[2];
    const char *typeName0 = "entry.src.hypium.module.mock.MockKit.MockKit";
    unionTypes[0] = g_implM->createType(file, ABCKIT_TYPE_ID_REFERENCE);
    g_implM->typeSetName(unionTypes[0], typeName0, strlen(typeName0));
    const char *typeName1 = "std.core.Null";
    unionTypes[1] = g_implM->createType(file, ABCKIT_TYPE_ID_REFERENCE);
    g_implM->typeSetName(unionTypes[1], typeName1, strlen(typeName1));

    AbckitType *fieldUnionType = g_implM->createUnionType(file, unionTypes, 2);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "Create field union type failed" << std::endl;
        return;
    }
    struct AbckitArktsFieldCreateParams params;
    params.name = STUB_FIELD_NAME.data();
    params.type = fieldUnionType;
    params.value = nullptr;
    params.isStatic = false;
    params.fieldVisibility = AbckitArktsFieldVisibility::PUBLIC;
    AbckitArktsClass *mockedArktsClass = g_implArkI->coreClassToArktsClass(mockedClass);
    auto ret = g_implArkM->classAddField(mockedArktsClass, &params);
    if (g_impl->getLastError() != ABCKIT_STATUS_NO_ERROR) {
        std::cerr << "Create class field failed" << std::endl;
        return;
    }
    CtorFuncStub(file, mockedClass);
    SetMockerStub(file, mockedClass);
}

// stub in the mocked class and method
static void StubInMockedFunc(AbckitFile *file, AbckitCoreFunction* mockedFunction,
                             vector<AbckitCoreClass*> lambdaClasses)
{
    AbckitCoreClass *mockedClass = g_implI->functionGetParentClass(mockedFunction);
    ClassFiledStub(file, mockedClass);
    FunctionStub(file, mockedFunction, mockedClass, lambdaClasses);
}

// Entry function, scan the mocked method and stub
static void MockTransformer(AbckitFile *file)
{
    map<AbckitCoreFunction*, vector<AbckitCoreClass*>> mockedFunctions = ScanMockFunctions(file);
    Initializer(file);
    set<string> mockedFunctionNames;
    for (auto it = mockedFunctions.begin(); it != mockedFunctions.end(); ++it) {
        AbckitCoreFunction *mockedFunction = it->first;
        vector<AbckitCoreClass*> lambdaClasses = it->second;
        string mockedFunctionName = GetString(g_implI->functionGetName(mockedFunction));
        if (mockedFunctionNames.find(mockedFunctionName) != mockedFunctionNames.end()) {
            cout << "Function: "<< mockedFunctionName <<" has been mocked" << endl;
            continue;
        }
        cout << "mockedFunction = " << mockedFunctionName << endl;
        for (auto lambdaClass : lambdaClasses) {
            cout << "lambdaClass = " << GetString(g_implI->classGetName(lambdaClass)) << endl;
        }
        StubInMockedFunc(file, mockedFunction, lambdaClasses);
        mockedFunctionNames.insert(mockedFunctionName);
    }
}

extern "C" int Entry(AbckitFile *file)
{
    MockTransformer(file);
    return g_hasError ? 1 : 0;
}