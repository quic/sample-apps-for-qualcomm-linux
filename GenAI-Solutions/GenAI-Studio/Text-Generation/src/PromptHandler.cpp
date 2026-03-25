// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "PromptHandler.hpp"


using namespace AppUtils;

// ── LLaMA3 tokens ────────────────────────────────────────────────────
constexpr std::string_view c_llama3_begin_system =
    "<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n";
constexpr std::string_view c_llama3_end_system   = " <|eot_id|>\n\n";
constexpr std::string_view c_llama3_begin_user   = "<|start_header_id|>user<|end_header_id|>\n\n";
constexpr std::string_view c_llama3_end_user     = "<|eot_id|>";
constexpr std::string_view c_llama3_begin_asst   = "<|start_header_id|>assistant<|end_header_id|>\n\n";
constexpr std::string_view c_llama3_end_asst     = "<|eot_id|>";

// ── Qwen2 tokens ─────────────────────────────────────────────────────
// Ref: https://huggingface.co/Qwen/Qwen2.5-7B-Instruct#quickstart
constexpr std::string_view c_qwen2_begin_system  = "<|im_start|>system\n";
constexpr std::string_view c_qwen2_end_system    = "<|im_end|>\n";
constexpr std::string_view c_qwen2_begin_user    = "<|im_start|>user\n";
constexpr std::string_view c_qwen2_end_user      = "<|im_end|>\n";
constexpr std::string_view c_qwen2_begin_asst    = "<|im_start|>assistant\n";
constexpr std::string_view c_qwen2_end_asst      = "<|im_end|>\n";

// ─────────────────────────────────────────────────────────────────────

PromptHandler::PromptHandler()
    : m_is_first_prompt(true)
{
}

void PromptHandler::SetSystemPrompt(std::string system_prompt) {
    m_system_prompt = std::move(system_prompt);
    m_is_first_prompt = true;
}

void PromptHandler::SetModelType(ModelType model_type) {
    m_model_type = model_type;
    m_is_first_prompt = true;
}

void PromptHandler::Reset() {
	    m_is_first_prompt = true;
}

std::string PromptHandler::GetPromptWithTag(const std::string& user_prompt)
{
    if (m_model_type == ModelType::QWEN2)
    {
        if (m_is_first_prompt)
        {
            m_is_first_prompt = false;
            return std::string(c_qwen2_begin_system) + m_system_prompt + c_qwen2_end_system.data() +
                   c_qwen2_begin_user.data() + user_prompt + c_qwen2_end_user.data() +
                   c_qwen2_begin_asst.data();
        }
        return std::string(c_qwen2_end_asst) + c_qwen2_begin_user.data() + user_prompt +
               c_qwen2_end_user.data() + c_qwen2_begin_asst.data();
    }

    // Default: LLaMA3
    if (m_is_first_prompt)
    {
        m_is_first_prompt = false;
        return std::string(c_llama3_begin_system) + m_system_prompt + c_llama3_end_system.data() +
               c_llama3_begin_user.data() + user_prompt + c_llama3_end_user.data() +
               c_llama3_begin_asst.data();
    }
    return std::string(c_llama3_end_asst) + c_llama3_begin_user.data() + user_prompt +
           c_llama3_end_user.data() + c_llama3_begin_asst.data();
}
