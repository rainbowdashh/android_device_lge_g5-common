#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import blob_fixup
from extract_utils.main import ExtractUtils, ExtractUtilsModule

common = ExtractUtils.get_module('msm8996-common', 'lge')

module = ExtractUtilsModule(
    'g5-common',
    'lge',
    blob_fixups={
        **common.blob_fixups,
        'vendor/lib/libbwfocuspeaking.so': blob_fixup().add_needed('libshim_bwfocus.so'),
    },
    lib_fixups=common.lib_fixups,
    namespace_imports=common.namespace_imports + ['vendor/lge/msm8996-common'],
)

if __name__ == '__main__':
    ExtractUtils.device_with_common(module, 'msm8996-common').run()
