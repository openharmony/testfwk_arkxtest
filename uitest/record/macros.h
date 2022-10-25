/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef BASE_UTILS_MACROS_H
#define BASE_UTILS_MACROS_H

#define UITEST_FORCE_EXPORT __attribute__((visibility("default")))

#ifndef UITEST_EXPORT
#ifndef WEARABLE_PRODUCT
#define UITEST_EXPORT UITEST_FORCE_EXPORT
#else
#define UITEST_EXPORT
#endif
#endif

// The macro "UITEST_FORCE_EXPORT_WITH_PREVIEW" is used to replace the macro "UITEST_FORCE_EXPORT"
// when adapting the napi to the previewer.
#ifndef UITEST_FORCE_EXPORT_WITH_PREVIEW
#ifndef WINDOWS_PLATFORM
#define UITEST_FORCE_EXPORT_WITH_PREVIEW UITEST_FORCE_EXPORT
#else
#define UITEST_FORCE_EXPORT_WITH_PREVIEW __declspec(dllexport)
#endif
#endif

// The macro "UITEST_EXPORT_WITH_PREVIEW" is used to replace the macro "UITEST_EXPORT"
// when adapting the napi to the previewer.
#ifndef UITEST_EXPORT_WITH_PREVIEW
#ifndef WINDOWS_PLATFORM
#define UITEST_EXPORT_WITH_PREVIEW UITEST_EXPORT
#else
#ifndef WEARABLE_PRODUCT
#define UITEST_EXPORT_WITH_PREVIEW __declspec(dllexport)
#else
#define UITEST_EXPORT_WITH_PREVIEW
#endif
#endif
#endif

#ifdef UITEST_DEBUG

#ifdef NDEBUG
#define CANCEL_NDEBUG
#undef NDEBUG
#endif // NDEBUG

#include <cassert>

#ifdef CANCEL_NDEBUG
#define NDEBUG
#undef CANCEL_NDEBUG
#endif // CANCEL_NDEBUG

#define UITEST_DCHECK(expr) assert(expr)
#else
#define UITEST_DCHECK(expr) ((void)0)

#endif // UITEST_DEBUG

#endif // BASE_UTILS_MACROS_H
