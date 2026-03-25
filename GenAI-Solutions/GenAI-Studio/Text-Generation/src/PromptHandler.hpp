// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#pragma once
#include <string>

namespace AppUtils {

enum class ModelType {
    LLAMA3,
    QWEN2
};

class PromptHandler {
private:
    bool m_is_first_prompt{true};
    std::string m_system_prompt{"You're a helpful AI assistant"};
    ModelType m_model_type{ModelType::LLAMA3};

public:
    PromptHandler();
    void SetSystemPrompt(std::string system_prompt);
    void SetModelType(ModelType model_type);
    void Reset();
    std::string GetPromptWithTag(const std::string& user_prompt);
};

} // namespace AppUtils
