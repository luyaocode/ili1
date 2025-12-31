#ifndef DRMSTRUCT_H
#define DRMSTRUCT_H
// DRM/KMS相关头文件
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// 存储DRM设备信息
struct DrmInfo {
    int fd = -1;
    drmModeRes *res = nullptr;
    drmModeConnector *conn = nullptr;
    drmModeEncoder *encoder = nullptr;
    drmModeModeInfo mode;
    uint32_t crtc_id = 0;
    uint32_t fb_id = 0;
    void *fb_map = nullptr;
    int width = 0;
    int height = 0;
    int stride = 0;
    int bpp = 32; // 默认32位色深
};
#endif // DRMSTRUCT_H
