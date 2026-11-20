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

#ifndef ANDROID_HARDWARE_LIGHT_LIGHT_H
#define ANDROID_HARDWARE_LIGHT_LIGHT_H

#include <aidl/android/hardware/light/BnLights.h>

#include <fstream>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace aidl::android::hardware::light {

struct Light : public BnLights {
    Light(std::ofstream&& backlight, std::ofstream&& emotionalBlinkPattern,
          std::ofstream&& emotionalOnOffPattern);

    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;
    ndk::ScopedAStatus getLights(std::vector<HwLight>* lights) override;

  private:
    void setAttentionLight(const HwLightState& state);
    void setBacklight(const HwLightState& state);
    void setBatteryLight(const HwLightState& state);
    void setNotificationLight(const HwLightState& state);
    void checkLightStateLocked();
    void setLightLocked(const HwLightState& state);

    std::ofstream mBacklight;
    std::ofstream mEmotionalBlinkPath;
    std::ofstream mEmotionalOnOffPath;

    HwLightState mAttentionState;
    HwLightState mBatteryState;
    HwLightState mNotificationState;

    std::unordered_map<int32_t, std::function<void(const HwLightState&)>> mLights;
    std::mutex mLock;
};

}  // namespace aidl::android::hardware::light

#endif  // ANDROID_HARDWARE_LIGHT_LIGHT_H
