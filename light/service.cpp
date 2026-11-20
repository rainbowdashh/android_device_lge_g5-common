/*
 * Copyright (C) 2017-2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light-service.alice"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <cerrno>
#include <cstring>

#include "Light.h"

using aidl::android::hardware::light::Light;

const static std::string kBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
const static std::string kEmotionalBlinkPath = "/sys/class/lg_rgb_led/use_patterns/blink_patterns";
const static std::string kEmotionalOnOffPath = "/sys/class/lg_rgb_led/use_patterns/onoff_patterns";

int main() {
    std::ofstream backlight(kBacklightPath);
    if (!backlight) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kBacklightPath.c_str(), error, strerror(error));
        return -error;
    }

    std::ofstream emotionalBlinkPattern(kEmotionalBlinkPath);
    if (!emotionalBlinkPattern) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kEmotionalBlinkPath.c_str(), error, strerror(error));
        return -error;
    }

    std::ofstream emotionalOnOffPattern(kEmotionalOnOffPath);
    if (!emotionalOnOffPattern) {
        int error = errno;
        ALOGE("Failed to open %s (%d): %s", kEmotionalOnOffPath.c_str(), error, strerror(error));
        return -error;
    }

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<Light>(
        std::move(backlight), std::move(emotionalBlinkPattern), std::move(emotionalOnOffPattern));
    const std::string instance = std::string(Light::descriptor) + "/default";
    if (AServiceManager_addService(service->asBinder().get(), instance.c_str()) != STATUS_OK) {
        ALOGE("Cannot register Light HAL service");
        return 1;
    }
    ABinderProcess_joinThreadPool();
    return 1;
}
