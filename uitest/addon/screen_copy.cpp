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

#include <condition_variable>
#include <display_manager.h>
#include <jpeglib.h>
#include <memory>
#include <mutex>
#include <pixel_map.h>
#include <refbase.h>
#include <screen_manager.h>
#include <securec.h>
#include <csetjmp>
#include "common_utilities_hpp.h"
#include "screen_copy.h"

namespace OHOS::uitest {
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Rosen;

using OnScreenChangeHandler = function<void()>;
class ScreenChangeListener : public ScreenManager::IScreenListener {
public:
    explicit ScreenChangeListener(OnScreenChangeHandler hdl, ScreenId id): handler_(hdl), targetId_(id) {}
    void OnConnect(ScreenId id) override {};
    void OnDisconnect(ScreenId id) override {};
    void OnChange(ScreenId id) override
    {
        if (handler_ != nullptr && id == targetId_) {
            handler_();
        }
    };
    void Destroy() { handler_ = nullptr; }
private:
    OnScreenChangeHandler handler_ = nullptr;
    ScreenId targetId_ = SCREEN_ID_INVALID;
};

class ScreenCopy {
public:
    explicit ScreenCopy(float scale): scale_(scale) {};
    virtual ~ScreenCopy();
    bool Run();
    void Destroy();
    const char* pendingError_ = nullptr;
    const float scale_ = 0.5f;
private:
    void PollAndNotifyFrames();
    void WaitAndConsumeFrames();
    void UpdateFrameLocked(shared_ptr<PixelMap> frame, bool &changed, bool &muted);
    shared_ptr<PixelMap> ScaleNewsetFrameLocked();
    sptr<Screen> sourceScreen_;
    shared_ptr<PixelMap> lastFrame_ = nullptr;
    shared_ptr<PixelMap> newestFrame_ = nullptr;
    mutex frameLock_;
    condition_variable frameCond_;
    unique_ptr<thread> snapshotThread = nullptr;
    unique_ptr<thread> encodeThread = nullptr;
    atomic_bool stopped_ = false;
    static sptr<ScreenChangeListener> screenChangeListener_;
};
sptr<ScreenChangeListener> ScreenCopy::screenChangeListener_ = nullptr;
static unique_ptr<ScreenCopy> g_screenCopy = nullptr;
static ScreenCopyHandler g_screenCopyHandler = nullptr;


static void AdapteScreenChange()
{
    if (g_screenCopy == nullptr) {
        return;
    }
    // destrory current one and create a new one
    LOG_D("Screen changed, auto restart ScreenCopy");
    const auto scale = g_screenCopy->scale_;
    g_screenCopy->Destroy();
    g_screenCopy = make_unique<ScreenCopy>(scale);
    g_screenCopy->Run();
}

ScreenCopy::~ScreenCopy()
{
    if (!stopped_.load()) {
        Destroy();
    }
}

bool ScreenCopy::Run()
{
    if (scale_ <= 0 || scale_ > 1.0) {
        pendingError_ = "Error: Illegal scale value!";
        return false;
    }
    // get source screen
    auto id = static_cast<ScreenId>(DisplayManager::GetInstance().GetDefaultDisplayId());
    sourceScreen_ = ScreenManager::GetInstance().GetScreenById(id);
    if (id == SCREEN_ID_INVALID || sourceScreen_ == nullptr) {
        pendingError_ = "Error: Get main screen failed!";
        return false;
    }
    // listen screen changes for auto-adapting
    if (screenChangeListener_ == nullptr) {
        screenChangeListener_ = new ScreenChangeListener([]() { AdapteScreenChange(); }, id);
        auto ret = ScreenManager::GetInstance().RegisterScreenListener(screenChangeListener_);
        LOG_D("Register ScreenListener, ret=%{public}d", ret);
    }
    // run snapshot thread and encode thread
    snapshotThread = make_unique<thread>([this]() { this->PollAndNotifyFrames(); });
    encodeThread = make_unique<thread>([this]() { this->WaitAndConsumeFrames(); });
    return true;
}

void ScreenCopy::Destroy()
{
    if (stopped_.load()) {
        return;
    }
    unique_lock<mutex> lock(frameLock_);
    stopped_.store(true);
    frameCond_.notify_all(); // mark stopped and wakeup the waiting thread
    lock.unlock();
    LOG_D("Begin to wait for threads exit");
    if (snapshotThread != nullptr && snapshotThread->joinable()) {
        snapshotThread->join();
        snapshotThread = nullptr;
    }
    sourceScreen_ = nullptr;
    lastFrame_ = nullptr;
    newestFrame_ = nullptr;
    if (encodeThread != nullptr && encodeThread->joinable()) {
        encodeThread->join();
        encodeThread = nullptr;
    }
    LOG_D("All threads exited");
}

void ScreenCopy::PollAndNotifyFrames()
{
    constexpr int32_t screenCheckIntervalUs = 50 * 1000;
    auto &dm = DisplayManager::GetInstance();
    const auto displayId = dm.GetDefaultDisplayId();
    LOG_I("Start PollAndNotifyFrames");
    bool changed = false;
    bool screenOff = false;
    while (!stopped_.load()) {
        if (screenOff) {
            usleep(screenCheckIntervalUs);
            if (dm.GetDisplayState(displayId) != DisplayState::OFF) {
                screenOff = false;
                LOG_I("Screen turned on! resume screenCopy");
            }
            continue;
        }
        shared_ptr<PixelMap> frame = dm.GetScreenshot(displayId);
        if (frame == nullptr) {
            continue;
        }
        frameLock_.lock();
        UpdateFrameLocked(frame, changed, screenOff);
        frameLock_.unlock();
        LOG_D("GetOneFrameDone, Changed=%{public}d", changed);
        if (changed) {
            frameCond_.notify_all();
        }
        if (screenOff) {
            LOG_I("Screen turned off! mute screenCopy");
        }
    }
}

void ScreenCopy::UpdateFrameLocked(shared_ptr<PixelMap> frame, bool &changed, bool &screenOff)
{
    DCHECK(sourceScreen_);
    lastFrame_ = newestFrame_;
    newestFrame_ = frame;
    changed = true;
    const size_t newestFrameSize = newestFrame_->GetHeight() * newestFrame_->GetRowStride();
    // if this is the first frame
    if (lastFrame_ == nullptr) {
        // if screen copy starts with screen-off, given a black image
        if (DisplayManager::GetInstance().GetDisplayState(sourceScreen_->GetId()) == DisplayState::OFF) {
            memset_s(frame->GetWritablePixels(), newestFrameSize, 0, newestFrameSize);
        }
        return;
    }
    // compare this frame and last frame
    const size_t lastFrameSize = lastFrame_->GetHeight() * lastFrame_->GetRowStride();
    if (lastFrameSize == newestFrameSize) {
        changed = memcmp(lastFrame_->GetPixels(), newestFrame_->GetPixels(), newestFrameSize) != 0;
    }
    // detect screen of only when not changed
    if (!changed && !screenOff) {
        screenOff = DisplayManager::GetInstance().GetDisplayState(sourceScreen_->GetId()) == DisplayState::OFF;
        if (screenOff) {
            // mark changed and reset pixels to black so we provide a black image
            changed = true;
            memset_s(frame->GetWritablePixels(), newestFrameSize, 0, newestFrameSize);
        }
    }
}

struct MissionErrorMgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

static void AdaptJpegSize(jpeg_compress_struct &jpeg, uint32_t width, uint32_t height)
{
    constexpr int32_t alignment = 32;
    if (width % alignment == 0) {
        jpeg.image_width = width;
    } else {
        LOG_D("The width need to be adapted!");
        jpeg.image_width = ceil((double)width / (double)alignment) * alignment;
    }
    jpeg.image_height = height;
}

shared_ptr<PixelMap> ScreenCopy::ScaleNewsetFrameLocked()
{
    if (newestFrame_ == nullptr) {
        return newestFrame_;
    }
    // resize the pixelmap to fit scale
    auto origWidth = newestFrame_->GetWidth();
    auto origHeight = newestFrame_->GetHeight();
    Media::Rect rect = {.left = 0, .top = 0, .width = origWidth, .height = origHeight};
    Media::InitializationOptions opt;
    opt.size.width = ceil(origWidth * scale_);
    opt.size.height = ceil(origHeight * scale_);
    opt.scaleMode = Media::ScaleMode::FIT_TARGET_SIZE;
    opt.editable = false;
    return Media::PixelMap::Create(*newestFrame_, rect, opt);
}

void ScreenCopy::WaitAndConsumeFrames()
{
    LOG_I("Start WaitAndConsumeFrames");
    while (!stopped_.load()) {
        unique_lock<mutex> lock(frameLock_);
        frameCond_.wait(lock);
        if (stopped_.load()) {
            break;
        }
        LOG_D("ConsumeFrame_Begin");
        // resize the pixelmap to fit scale
        auto scaledPixels = ScaleNewsetFrameLocked();
        lock.unlock();
        if (scaledPixels == nullptr) {
            continue;
        }
        constexpr int32_t rgbaPixelBytes = 4;
        jpeg_compress_struct jpeg;
        MissionErrorMgr jerr;
        jpeg.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&jpeg);
        AdaptJpegSize(jpeg, scaledPixels->GetWidth(), scaledPixels->GetHeight());
        jpeg.input_components = rgbaPixelBytes;
        jpeg.in_color_space = JCS_EXT_RGBX;
        jpeg_set_defaults(&jpeg);
        constexpr int32_t compressQuality = 75;
        jpeg_set_quality(&jpeg, compressQuality, 1);
        uint8_t *imgBuf = nullptr;
        unsigned long imgSize = 0;
        jpeg_mem_dest(&jpeg, &imgBuf, &imgSize);
        jpeg_start_compress(&jpeg, 1);
        JSAMPROW rowPointer[1024 * 4];
        const auto stride = scaledPixels->GetRowStride();
        auto memAddr = (uint8_t *)(scaledPixels->GetPixels());
        for (int32_t rowIndex = 0; rowIndex < scaledPixels->GetHeight(); rowIndex++) {
            rowPointer[rowIndex] = memAddr;
            memAddr += stride;
        }
        (void)jpeg_write_scanlines(&jpeg, rowPointer, jpeg.image_height);
        jpeg_finish_compress(&jpeg);
        jpeg_destroy_compress(&jpeg);
        LOG_D("ConsumeFrame_End, size=%{public}lu", imgSize);
        if (g_screenCopyHandler != nullptr) {
            g_screenCopyHandler(imgBuf, imgSize);
        } else {
            free(imgBuf);
        }
    }
    LOG_I("Stop WaitAndConsumeFrames");
}

bool StartScreenCopy(float scale, ScreenCopyHandler handler)
{
    if (scale <= 0 || scale > 1 || handler == nullptr) {
        LOG_E("Illegal arguments");
        return false;
    }
    StopScreenCopy();
    g_screenCopyHandler = handler;
    g_screenCopy = make_unique<ScreenCopy>(scale);
    bool success = g_screenCopy != nullptr && g_screenCopy->Run();
    if (!success) {
        constexpr size_t BUF_SIZE = 128;
        auto buf = (uint8_t *)malloc(BUF_SIZE);
        memset_s(buf, BUF_SIZE, 0, BUF_SIZE);
        if (g_screenCopy->pendingError_ != nullptr) {
            g_screenCopy->pendingError_ = "Failed to run ScreenCopy, unknown error";
        }
        memcpy_s(buf, BUF_SIZE, g_screenCopy->pendingError_, strlen(g_screenCopy->pendingError_));
        LOG_E("The error message is %{public}s", buf);
        handler(buf, strlen(g_screenCopy->pendingError_));
        g_screenCopy->pendingError_ = nullptr;
        StopScreenCopy();
    }
    return success;
}

void StopScreenCopy()
{
    if (g_screenCopy != nullptr) {
        g_screenCopy->Destroy();
        g_screenCopy = nullptr;
    }
}
}
