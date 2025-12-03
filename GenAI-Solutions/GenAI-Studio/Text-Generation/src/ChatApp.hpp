// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#pragma once

#include <string>

#include "Genie.hpp"

namespace App
{
constexpr const std::string_view c_exit_prompt = "exit";

class ChatApp
{
  private:
    std::string config_;
    std::string m_user_name;
    Genie genie_; 

  public:
    /**
     * ChatApp: Initializes ChatApp
     *    - Uses provided Genie configuration string
     *    - Creates handle for Genie
     *
     * @param config: JSON string containing Genie configuration
     *
     * @throws on failure to create handle for Genie config, dialog
     *
     */
    ChatApp(const std::string& config);
    ChatApp() = delete;
    ChatApp(const ChatApp&) = delete;
    ChatApp(ChatApp&&) = delete;
    ChatApp& operator=(const ChatApp&) = delete;
    ChatApp& operator=(ChatApp&&) = delete;
    ~ChatApp();

    /**
     * ChatWithUser: Starts Chat with user using previously loaded config
     *
     * @param user_name: User name to use during chat
     *
     * @throws on failure to query model response during chat
     *
     */
    void ChatLoop();
};
} // namespace App
