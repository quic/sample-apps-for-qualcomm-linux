// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#pragma once

#include <string>
#include <mutex>
#include <stdexcept>
#include <iostream>

#include "GenieCommon.h"
#include "GenieDialog.h"
#include "GenieSampler.h"

class Genie {
public:
    explicit Genie(std::string config_path, uint32_t max_tokens = 200);
    ~Genie();

    // Lifecycle
    void initialize();          // throws std::runtime_error on failure
    void cleanup() noexcept;    // no-throw
    void reload();              // cleanup + initialize; throws on failure
    bool reset() noexcept;      // soft reset; returns true on success

    // Configuration
    bool setMaxTokens(uint32_t max_tokens) noexcept;
    uint32_t maxTokens() const noexcept { return max_tokens_; }
    bool isReady() const noexcept;

    void applySamplerConfig(const std::string& samplerBlock);
    // Inference
    std::string query(const std::string& prompt,
                      GenieDialog_SentenceCode_t sentence_code =
                          GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE);

private:
    std::string config_path_;
    uint32_t max_tokens_{200};

    GenieDialogConfig_Handle_t cfg_{nullptr};
    GenieDialog_Handle_t dlg_{nullptr};

    mutable std::mutex mu_;

    // Unlocked helpers: MUST be called with mu_ already held
    void _cleanupUnlocked() noexcept;
    void _initializeUnlocked(); // may throw

    // Static C-style callback to collect tokens into user_data string
    static void Callback(const char* response_back,
                         const GenieDialog_SentenceCode_t sentence_code,
                         const void* user_data);
};

