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

#define LOG_TAG "light"

#include "Light.h"

#include <log/log.h>

#include <cmath>
#include <cstdio>

#define LCD_BRIGHTNESS_MIN 1  // Matches config_screenBrightnessSettingMinimum
#define LCD_BRIGHTNESS_MAX 255
#define LCD_BRIGHTNESS_DELTA (LCD_BRIGHTNESS_MAX - LCD_BRIGHTNESS_MIN)

namespace {
using aidl::android::hardware::light::HwLightState;

static uint32_t rgbToBrightness(const HwLightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) + (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >>
           8;
}

static bool isLit(const HwLightState& state) { return (state.color & 0x00ffffff); }
}  // anonymous namespace

namespace aidl::android::hardware::light {

Light::Light(std::ofstream&& backlight, std::ofstream&& emotionalBlinkPattern,
             std::ofstream&& emotionalOnOffPattern)
    : mBacklight(std::move(backlight)),
      mEmotionalBlinkPath(std::move(emotionalBlinkPattern)),
      mEmotionalOnOffPath(std::move(emotionalOnOffPattern)) {
    auto attnFn(std::bind(&Light::setAttentionLight, this, std::placeholders::_1));
    auto backlightFn(std::bind(&Light::setBacklight, this, std::placeholders::_1));
    auto batteryFn(std::bind(&Light::setBatteryLight, this, std::placeholders::_1));
    auto notifFn(std::bind(&Light::setNotificationLight, this, std::placeholders::_1));
    mLights.emplace(std::make_pair(static_cast<int32_t>(LightType::ATTENTION), attnFn));
    mLights.emplace(std::make_pair(static_cast<int32_t>(LightType::BACKLIGHT), backlightFn));
    mLights.emplace(std::make_pair(static_cast<int32_t>(LightType::BATTERY), batteryFn));
    mLights.emplace(std::make_pair(static_cast<int32_t>(LightType::NOTIFICATIONS), notifFn));
}

ndk::ScopedAStatus Light::setLightState(int32_t id, const HwLightState& state) {
    auto it = mLights.find(id);
    if (it == mLights.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    it->second(state);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Light::getLights(std::vector<HwLight>* lights) {
    lights->clear();
    for (auto const& light : mLights) {
        lights->push_back(
            {.id = light.first, .ordinal = 0, .type = static_cast<LightType>(light.first)});
    }
    return ndk::ScopedAStatus::ok();
}

void Light::setAttentionLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mAttentionState = state;
    checkLightStateLocked();
}

static uint32_t applyGamma(const uint32_t brightness) {
    if (brightness < LCD_BRIGHTNESS_MIN) return 0;
    return LCD_BRIGHTNESS_MIN +
           LCD_BRIGHTNESS_DELTA *
               cbrt(((double)brightness - LCD_BRIGHTNESS_MIN) / LCD_BRIGHTNESS_DELTA);
}

void Light::setBacklight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    uint32_t brightness = rgbToBrightness(state);
    brightness = applyGamma(brightness);
    mBacklight << brightness << std::endl;
}

void Light::setBatteryLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mBatteryState = state;
    checkLightStateLocked();
}

void Light::setNotificationLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mNotificationState = state;
    checkLightStateLocked();
}

void Light::checkLightStateLocked() {
    if (isLit(mNotificationState)) {
        setLightLocked(mNotificationState);
    } else if (isLit(mAttentionState)) {
        setLightLocked(mAttentionState);
    } else if (isLit(mBatteryState)) {
        setLightLocked(mBatteryState);
    } else {
        /* Lights off */
        mEmotionalBlinkPath << "0x0,-1,-1" << std::endl;
        mEmotionalOnOffPath << "0x0" << std::endl;
    }
}

void Light::setLightLocked(const HwLightState& state) {
    int onMS, offMS;
    uint32_t color;
    char pattern[64];

    switch (state.flashMode) {
        case FlashMode::TIMED:
            onMS = state.flashOnMs;
            offMS = state.flashOffMs;
            break;
        case FlashMode::NONE:
            onMS = 0;
            offMS = 0;
            break;
        default:
            onMS = -1;
            offMS = -1;
            break;
    }

    color = state.color & 0x00ffffff;

    if (offMS <= 0) {
        snprintf(pattern, sizeof(pattern), "0x%06x", color);
        ALOGD("%s: Using onoff pattern: inColor=0x%06x\n", __func__, color);
        mEmotionalOnOffPath << pattern << std::endl;
    } else {
        snprintf(pattern, sizeof(pattern), "0x%06x,%d,%d", color, onMS, offMS);
        ALOGD("%s: Using blink pattern: inColor=0x%06x delay_on=%d, delay_off=%d\n", __func__,
              color, onMS, offMS);
        mEmotionalBlinkPath << pattern << std::endl;
    }
}

}  // namespace aidl::android::hardware::light
