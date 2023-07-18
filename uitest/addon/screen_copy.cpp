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

#include <display_manager.h>
#include <iconsumer_surface.h>
#include <jpeglib.h>
#include <pixel_map.h>
#include <refbase.h>
#include <screen_manager.h>
#include <securec.h>
#include <csetjmp>
#include <surface.h>
#include <poll.h>
#include "common_utilities_hpp.h"
#include "screen_copy.h"

namespace OHOS::uitest {
using namespace std;
using namespace OHOS;
using namespace OHOS::Rosen;

using OnBufferAvailableHandler = function<void()>;
class BufferConsumerListener : public IBufferConsumerListener {
public:
    explicit BufferConsumerListener(OnBufferAvailableHandler handler): handler_(handler) {}
    void OnBufferAvailable() override
    {
        if (handler_ != nullptr) {
            handler_();
        }
    }

private:
    OnBufferAvailableHandler handler_ = nullptr;
};

class ScreenCopy {
public:
    ScreenCopy() = default;
    virtual ~ScreenCopy();
    bool Setup();
    bool Run();
    bool Pause();
    void Destroy();
    void HandleConsumerBuffer();
private:
    void ConsumeFrameBuffer(const sptr<SurfaceBuffer> &buf);
    sptr<IBufferConsumerListener> bufferListener_ = nullptr;
    sptr<IConsumerSurface> consumerSurface_ = nullptr;
    sptr<Surface> producerSurface_ = nullptr;
    pair<uint32_t, uint32_t> screenSize_;
    ScreenId mainScreenId_ = SCREEN_ID_INVALID;
    ScreenId virtualScreenId_ = SCREEN_ID_INVALID;
    bool workable_ = false;
};
static unique_ptr<ScreenCopy> g_screenCopy = nullptr;
static ScreenCopyHandler g_screenCopyHandler = nullptr;

ScreenCopy::~ScreenCopy()
{
    Destroy();
}

void ScreenCopy::Destroy()
{
    Pause();
    producerSurface_ = nullptr;
    if (consumerSurface_ != nullptr) {
        consumerSurface_->UnregisterConsumerListener();
        consumerSurface_ = nullptr;
    }
}

bool ScreenCopy::Setup()
{
    // create pruducer surface and consumer surface
    consumerSurface_ = IConsumerSurface::Create();
    if (consumerSurface_ == nullptr) {
        LOG_E("Failed to create IConsumerSurface");
        return false;
    }
    auto producer = consumerSurface_->GetProducer();
    producerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface_ == nullptr) {
        LOG_E("Failed to CreateSurfaceAsProducer");
        return false;
    }
    auto handler = [this]() {
        this->HandleConsumerBuffer();
    };
    bufferListener_ = new BufferConsumerListener(handler);
    if (consumerSurface_->RegisterConsumerListener(bufferListener_) != 0) {
        LOG_E("Failed to RegisterConsumerListener");
        return false;
    }
    // make screen mirror from main screen to accept frames with producer surface buffer
    mainScreenId_ = static_cast<ScreenId>(DisplayManager::GetInstance().GetDefaultDisplayId());
    auto mainScreen = ScreenManager::GetInstance().GetScreenById(mainScreenId_);
    if (mainScreenId_ == SCREEN_ID_INVALID || mainScreen == nullptr) {
        LOG_E("Get main screen failed!");
        return false;
    }
    screenSize_.first = mainScreen->GetWidth();
    screenSize_.second = mainScreen->GetHeight();
    workable_ = true;
    return true;
}

bool ScreenCopy::Run()
{
    if (!workable_) {
        return false;
    }
    if (virtualScreenId_ != SCREEN_ID_INVALID) {
        LOG_W("ScreenCopy already running!");
        return false;
    }
    VirtualScreenOption option = {
        .name_ = "virtualScreen",
        .width_ = screenSize_.first, // * scale_,
        .height_ = screenSize_.second, // * scale_,
        .density_ = 2.0,
        .surface_ = producerSurface_,
        .flags_ = 0,
        .isForShot_ = true,
    };
    virtualScreenId_ = ScreenManager::GetInstance().CreateVirtualScreen(option);
    vector<ScreenId> mirrorIds;
    mirrorIds.push_back(virtualScreenId_);
    ScreenId screenGroupId = static_cast<ScreenId>(1);
    auto ret = ScreenManager::GetInstance().MakeMirror(mainScreenId_, mirrorIds, screenGroupId);
    if (ret != DMError::DM_OK) {
        LOG_E("Make mirror screen for default screen failed");
        return false;
    }
    return true;
}

bool ScreenCopy::Pause()
{
    if (!workable_ || virtualScreenId_ == SCREEN_ID_INVALID) {
        return true;
    }
    vector<ScreenId> mirrorIds;
    mirrorIds.push_back(virtualScreenId_);
    auto err0 = ScreenManager::GetInstance().StopMirror(mirrorIds);
    auto err1 = ScreenManager::GetInstance().DestroyVirtualScreen(virtualScreenId_);
    virtualScreenId_ = SCREEN_ID_INVALID;
    if (err0 == DMError::DM_OK && err1 == DMError::DM_OK) {
        return true;
    } else {
        LOG_E("Pause screenCopy failed, stopMirrorErr=%{public}d, destroyVirScrErr=%{public}d", err0, err1);
        return false;
    }
}

void ScreenCopy::HandleConsumerBuffer()
{
    LOG_D("ScreenCopy::HandleConsumerBuffer");
    sptr<SurfaceBuffer> consumerSuffer = nullptr;
    int32_t fenceFd = -1;
    int64_t tick = 0;
    int32_t timeout = 3000;
    Rect rect;
    auto aquireRet = consumerSurface_->AcquireBuffer(consumerSuffer, fenceFd, tick, rect);
    if (aquireRet != 0 || consumerSuffer == nullptr) {
        LOG_E("AcquireBuffer failed");
        return;
    }
    int pollRet = -1;
    struct pollfd pfd = {0};
    pfd.fd = fenceFd;
    pfd.events = POLLIN;
    do {
        pollRet = poll(&pfd, 1, timeout);
    } while (pollRet == -1 && (errno == EINTR || errno == EAGAIN));

    if (pollRet == 0) {
        pollRet = -1;
        errno = ETIME;
    } else if (pollRet > 0) {
        pollRet = 0;
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            pollRet = -1;
            errno = EINVAL;
        }
    }
    if (pollRet < 0) {
        LOG_E("Poll on fenceFd failed: %{public}s", strerror(errno));
        return;
    }
    LOG_D("start ConsumerFrameBuffer");
    ConsumeFrameBuffer(consumerSuffer);
    LOG_D("end ConsumeFrameBuffer");
    if (consumerSurface_->ReleaseBuffer(consumerSuffer, -1) != 0) {
        LOG_E("ReleaseBuffer failed");
        return;
    }
}

static void ConvertRGBA2RGB(const uint8_t* input, uint8_t* output, int32_t pixelNum)
{
    constexpr uint8_t BLUE_INDEX = 0;
    constexpr uint8_t GREEN_INDEX = 1;
    constexpr uint8_t RED_INDEX = 2;
    constexpr uint32_t RGB_PIXEL_BYTES = 3;
    constexpr uint8_t SHIFT_8_BIT = 8;
    constexpr uint8_t SHIFT_16_BIT = 16;
    constexpr uint32_t RGBA_MASK_BLUE = 0x000000FF;
    constexpr uint32_t RGBA_MASK_GREEN = 0x0000FF00;
    constexpr uint32_t RGBA_MASK_RED = 0x00FF0000;
    DCHECK(input != nullptr && output != nullptr && pixelNum > 0);
    auto pRgba = reinterpret_cast<const uint32_t*>(input);
    for (int32_t index = 0; index < pixelNum; index++) {
        output[index * RGB_PIXEL_BYTES + RED_INDEX] = (pRgba[index] & RGBA_MASK_RED) >> SHIFT_16_BIT;
        output[index * RGB_PIXEL_BYTES + GREEN_INDEX] = (pRgba[index] & RGBA_MASK_GREEN) >> SHIFT_8_BIT;
        output[index * RGB_PIXEL_BYTES + BLUE_INDEX] = (pRgba[index] & RGBA_MASK_BLUE);
    }
}

struct MissionErrorMgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

static void AdaptJpegSize(jpeg_compress_struct &jpeg, uint32_t width, uint32_t height)
{
    constexpr int32_t VIRTUAL_SCREEN_SCALE = 32;
    if (width % VIRTUAL_SCREEN_SCALE == 0) {
        jpeg.image_width = width;
    } else {
        LOG_D("The width need to be adapted!");
        jpeg.image_width = ceil((double)width / (double)VIRTUAL_SCREEN_SCALE) * VIRTUAL_SCREEN_SCALE;
    }
    jpeg.image_height = height;
}

void ScreenCopy::ConsumeFrameBuffer(const sptr<SurfaceBuffer> &buf)
{
    auto bufHdl = buf->GetBufferHandle();
    if (bufHdl == nullptr) {
        LOG_E("GetBufferHandle failed");
        return;
    }
    if (g_screenCopyHandler == nullptr) {
        LOG_W("Consumer handler is nullptr, ignore this frame");
        return;
    }
    LOG_I("ConsumeFrameBuffer_BeginEncodeFrameToJPEG");
    auto width = static_cast<uint32_t>(bufHdl->width);
    auto height = static_cast<uint32_t>(bufHdl->height);
    auto stride = static_cast<uint32_t>(bufHdl->stride);
    auto data = (uint8_t *)buf->GetVirAddr();
    constexpr int32_t RGBA_PIXEL_BYTES = 4;
    constexpr int32_t RGB_PIXEL_BYTES = 3;
    int32_t rgbSize = stride * height * RGB_PIXEL_BYTES / RGBA_PIXEL_BYTES;
    auto rgb = new uint8_t[rgbSize];
    ConvertRGBA2RGB(data, rgb, rgbSize / RGB_PIXEL_BYTES);

    jpeg_compress_struct jpeg;
    MissionErrorMgr jerr;
    jpeg.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&jpeg);
    AdaptJpegSize(jpeg, width, height);
    jpeg.input_components = RGB_PIXEL_BYTES;
    jpeg.in_color_space = JCS_RGB;
    jpeg_set_defaults(&jpeg);
    constexpr int32_t COMPRESS_QUALITY = 75;
    jpeg_set_quality(&jpeg, COMPRESS_QUALITY, 1);
    uint8_t *imgBuf = nullptr;
    unsigned long imgSize = 0;
    jpeg_mem_dest(&jpeg, &imgBuf, &imgSize);
    jpeg_start_compress(&jpeg, 1);
    JSAMPROW rowPointer[1];
    for (uint32_t rowIndex = 0; rowIndex < jpeg.image_height; rowIndex++) {
        rowPointer[0] = const_cast<uint8_t *>(rgb + rowIndex * jpeg.image_width * RGB_PIXEL_BYTES);
        (void)jpeg_write_scanlines(&jpeg, rowPointer, 1);
    }
    jpeg_finish_compress(&jpeg);
    jpeg_destroy_compress(&jpeg);
    free(rgb);
    LOG_I("ConsumeFrameBuffer_EndEncodeFrameToJPEG");
    if (g_screenCopyHandler != nullptr) {
        g_screenCopyHandler(imgBuf, imgSize);
    } else {
        free(imgBuf);
    }
}

bool StartScreenCopy(float scale, ScreenCopyHandler handler)
{
    if (scale <= 0 || scale > 1 || handler == nullptr) {
        LOG_E("Illegal arguments");
        return false;
    }
    g_screenCopyHandler = handler;
    if (g_screenCopy == nullptr) {
        g_screenCopy = make_unique<ScreenCopy>();
        g_screenCopy->Setup();
    }
    return g_screenCopy->Run();
}

void StopScreenCopy()
{
    if (g_screenCopy != nullptr) {
        g_screenCopy->Pause();
    }
}
}