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
    ScreenCopy(float scale, ScreenCopyHandler handler);
    virtual ~ScreenCopy();
    bool Setup();
    void Destroy();
    sptr<Surface> GetInputSurface() const;
    void HandleConsumerBuffer();
    void ReleaseImgBuf();
private:
    void ConsumeFrameBuffer(const sptr<SurfaceBuffer> &buf);
    const float scale_ = 0;
    const ScreenCopyHandler handler_ = nullptr;
    sptr<IBufferConsumerListener> bufferListener_ = nullptr;
    sptr<IConsumerSurface> consumerSurface_ = nullptr;
    sptr<Surface> producerSurface_ = nullptr;
    ScreenId virtualScreenId_ = 0;
    uint8_t *imgBuf_ = nullptr;
    unsigned long imgDataLen_ = 0;
    static constexpr size_t SHARED_MEM_SIZE = 4 * 1024 * 1024;
};
static unique_ptr<ScreenCopy> g_currentScreenCopy = nullptr;

ScreenCopy::ScreenCopy(float scale, ScreenCopyHandler handler): scale_(scale), handler_(handler) {}

void ScreenCopy::ReleaseImgBuf()
{
    LOG_D("ReleaseImgBuf");
    if (imgBuf_ != nullptr) {
        free(imgBuf_);
        imgBuf_ = nullptr;
    }
    imgDataLen_ = 0;
}

ScreenCopy::~ScreenCopy()
{
    if (virtualScreenId_ > 0) {
        ScreenManager::GetInstance().DestroyVirtualScreen(virtualScreenId_);
        virtualScreenId_ = 0;
    }
    producerSurface_ = nullptr;
    if (consumerSurface_ != nullptr) {
        consumerSurface_->UnregisterConsumerListener();
    }
    consumerSurface_ = nullptr;
    ReleaseImgBuf();
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
    auto mainScreenId = static_cast<ScreenId>(DisplayManager::GetInstance().GetDefaultDisplayId());
    auto mainScreen = ScreenManager::GetInstance().GetScreenById(mainScreenId);
    if (mainScreenId == SCREEN_ID_INVALID || mainScreen == nullptr) {
        LOG_E("Get main screen failed!");
        return false;
    }
    VirtualScreenOption option = {
        .name_ = "virtualScreen",
        .width_ = mainScreen->GetWidth(), // * scale_,
        .height_ = mainScreen->GetHeight(), // * scale_,
        .density_ = 2.0,
        .surface_ = producerSurface_,
        .flags_ = 0,
        .isForShot_ = true,
    };
    ScreenId virtualScreenId = ScreenManager::GetInstance().CreateVirtualScreen(option);
    vector<ScreenId> mirrorIds;
    mirrorIds.push_back(virtualScreenId);
    ScreenId screenGroupId = static_cast<ScreenId>(1);
    auto ret = ScreenManager::GetInstance().MakeMirror(mainScreenId, mirrorIds, screenGroupId);
    if (ret != DMError::DM_OK) {
        LOG_E("Make mirror screen for default screen failed");
        return false;
    }
    return true;
}

void ScreenCopy::Destroy()
{
    this->~ScreenCopy();
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

void ScreenCopy::ConsumeFrameBuffer(const sptr<SurfaceBuffer> &buf)
{
    auto bufHdl = buf->GetBufferHandle();
    if (bufHdl == nullptr) {
        LOG_E("GetBufferHandle failed");
        return;
    }
    if (handler_ == nullptr) {
        LOG_W("Consumer handler is nullptr, ignore this frame");
        return;
    }
    if (imgBuf_ != nullptr) {
        LOG_W("Last frame not consumed, ignore this frame");
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
    jpeg.image_width = width;
    jpeg.image_height = height;
    jpeg.input_components = RGB_PIXEL_BYTES;
    jpeg.in_color_space = JCS_RGB;
    jpeg_set_defaults(&jpeg);
    constexpr int32_t COMPRESS_QUALITY = 75;
    jpeg_set_quality(&jpeg, COMPRESS_QUALITY, 1);
    jpeg_mem_dest(&jpeg, &imgBuf_, &imgDataLen_);
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
    handler_(imgBuf_, imgDataLen_);
}

bool StartScreenCopy(float scale, ScreenCopyHandler handler)
{
    if (scale <= 0 || scale > 1 || handler == nullptr) {
        LOG_E("Illegal arguments");
        return false;
    }
    StopScreenCopy();
    g_currentScreenCopy = make_unique<ScreenCopy>(scale, handler);
    return g_currentScreenCopy->Setup();
}

void StopScreenCopy()
{
    if (g_currentScreenCopy != nullptr) {
        g_currentScreenCopy->Destroy();
        g_currentScreenCopy = nullptr;
    }
}

void NotifyScreenCopyFrameConsumed()
{
    if (g_currentScreenCopy != nullptr) {
        g_currentScreenCopy->ReleaseImgBuf();
    }
}
}