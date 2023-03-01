/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <csignal>
#include <iostream>

extern "C" void __gcov_dump();

using namespace std;
static void sighandler(int signo)
{
    cout << "######gcov_dump_start" << endl;
    __gcov_dump();
    cout << "######gcov_dump_end" << endl;
}

__attribute__((constructor)) static void ctor()
{
    int sigs[] = {
    SIGTTIN
    };
    int i;
    struct sigaction sa;
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i)
        {
            if (sigaction(sigs[i], &sa, nullptr) == -1)
                {
                    cout << "Could not set signal handler" << endl;
                }
        }
}

