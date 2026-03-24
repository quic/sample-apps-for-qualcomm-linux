// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "Genie.hpp"

Genie::Genie(std::string config_path, uint32_t max_tokens)
    : config_path_(std::move(config_path)),
      max_tokens_(max_tokens) {}

Genie::~Genie() {
    // Ensure resources are freed
    cleanup();
}

bool Genie::isReady() const noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    return cfg_ != nullptr && dlg_ != nullptr;
}

// ----------------------
// Unlocked helpers (require mu_ held)
// ----------------------
void Genie::_cleanupUnlocked() noexcept {
    // Free dialog first
    if (dlg_) {
        if (GENIE_STATUS_SUCCESS != GenieDialog_free(dlg_)) {
            std::cerr << "[Genie] Warning: GenieDialog_free failed." << std::endl;
        }
        dlg_ = nullptr;
    }

    // Then free config
    if (cfg_) {
        if (GENIE_STATUS_SUCCESS != GenieDialogConfig_free(cfg_)) {
            std::cerr << "[Genie] Warning: GenieDialogConfig_free failed." << std::endl;
        }
        cfg_ = nullptr;
    }
}

void Genie::_initializeUnlocked() {
    // Create config from JSON file path
    if (GENIE_STATUS_SUCCESS != GenieDialogConfig_createFromJson(config_path_.c_str(), &cfg_) || !cfg_) {
        throw std::runtime_error("GenieDialogConfig_createFromJson failed. Check config path/content: " + config_path_);
    }

    // Create dialog from config
    if (GENIE_STATUS_SUCCESS != GenieDialog_create(cfg_, &dlg_) || !dlg_) {
        _cleanupUnlocked();
        throw std::runtime_error("GenieDialog_create failed.");
    }

    // Set max tokens on dialog
    if (GENIE_STATUS_SUCCESS != GenieDialog_setMaxNumTokens(dlg_, max_tokens_)) {
        _cleanupUnlocked();
        throw std::runtime_error("GenieDialog_setMaxNumTokens failed.");
    }
}


void Genie::applySamplerConfig(const std::string& samplerBlock) {

    
    Genie_Status_t status;
    
    GenieSampler_Handle_t samplerHandle = nullptr;
    status = GenieDialog_getSampler(dlg_, &samplerHandle);
    
    if (status != GENIE_STATUS_SUCCESS || !samplerHandle) {
        throw std::runtime_error("Failed to get Genie sampler handle");
    }

    // 2) Create sampler config handle from *full* sampler JSON
    GenieSamplerConfig_Handle_t samplerConfigHandle = nullptr;
    status = GenieSamplerConfig_createFromJson(samplerBlock.c_str(),
                                               &samplerConfigHandle);
    
    if (status != GENIE_STATUS_SUCCESS || !samplerConfigHandle) {
        throw std::runtime_error("Failed to create sampler config handle");
    }
    // Updating with new sampler block
    status = GenieSamplerConfig_setParam(samplerConfigHandle, "", samplerBlock.c_str());
    if (status != GENIE_STATUS_SUCCESS) {
        throw std::runtime_error("Failed to set top-p");
    }
    // 4) Apply the sampler config to the sampler
    status = GenieSampler_applyConfig(samplerHandle, samplerConfigHandle);
    if (status != GENIE_STATUS_SUCCESS) {
        throw std::runtime_error("Failed to apply sampler config");
    }

    // 5) (optional) free the config when done
    // GenieSamplerConfig_free(samplerConfigHandle);

    
}

// ----------------------
// Public API (locks once)
// ----------------------
void Genie::initialize() {
    std::lock_guard<std::mutex> lock(mu_);
    // If there was previous state, clear it without re-locking
    if (cfg_ || dlg_) {
        _cleanupUnlocked();
    }
    _initializeUnlocked(); // may throw
}

void Genie::cleanup() noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    _cleanupUnlocked();
}

void Genie::reload() {
    // Atomically cleanup + initialize under a single lock
    std::lock_guard<std::mutex> lock(mu_);
    _cleanupUnlocked();
    _initializeUnlocked(); // may throw
}

bool Genie::reset() noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    if (!dlg_) return false;
    return GENIE_STATUS_SUCCESS == GenieDialog_reset(dlg_);
}

bool Genie::setMaxTokens(uint32_t max_tokens) noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    max_tokens_ = max_tokens;
    if (!dlg_) return false;
    return GENIE_STATUS_SUCCESS == GenieDialog_setMaxNumTokens(dlg_, max_tokens_);
}

void Genie::Callback(const char* response_back,
                     const GenieDialog_SentenceCode_t /*sentence_code*/,
                     const void* user_data) {
    // user_data is a pointer to std::string that we will append to
    auto* out = static_cast<std::string*>(const_cast<void*>(user_data));
    if (out && response_back) {
        out->append(response_back);
    }
}

std::string Genie::query(const std::string& prompt,
                         GenieDialog_SentenceCode_t sentence_code) {
    // Hold the lock for the duration of the SDK call to avoid races
    std::lock_guard<std::mutex> lock(mu_);

    if (!dlg_) {
        throw std::runtime_error("Genie query called before initialization (dialog not ready).");
    }

    std::string model_response;
    const auto status = GenieDialog_query(
        dlg_,
        prompt.c_str(),
        sentence_code,
        &Genie::Callback,
        &model_response);

    if (status != GENIE_STATUS_SUCCESS) {
        // Let caller decide recovery (e.g., reload() + retry)
        throw std::runtime_error("GenieDialog_query failed with status: " + std::to_string(status));
    }

    return model_response;
}

