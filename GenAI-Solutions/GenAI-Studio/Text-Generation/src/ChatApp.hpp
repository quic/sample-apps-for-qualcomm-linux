// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#pragma once

#include <string>
#include <memory>
#include "Genie.hpp"
#include "PromptHandler.hpp"

namespace App {

class ChatApp {
public:
    ChatApp(std::string llama_config, std::string llama_base_dir,
            std::string qwen_config,  std::string qwen_base_dir);

    ~ChatApp();

    void Run(uint16_t port);

private:
    std::string m_llama_config, m_llama_base_dir;
    std::string m_qwen_config,  m_qwen_base_dir;

    std::unique_ptr<Genie> m_llama_genie;
    std::unique_ptr<Genie> m_qwen_genie;
    Genie* m_active_genie{nullptr};

    AppUtils::ModelType     m_active_model{AppUtils::ModelType::LLAMA3};
    AppUtils::PromptHandler prompt_handler;

    bool m_llama_initialized{true};
};

} // namespace App
