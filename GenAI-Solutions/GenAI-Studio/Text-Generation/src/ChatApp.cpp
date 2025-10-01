// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "ChatApp.hpp"
#include "PromptHandler.hpp"
#include <fstream>
#include <iostream>
#include "crow.h"

using namespace App;

std::string output;

namespace
{

constexpr const int c_chat_separater_length = 80;

//
// ChatSplit - Line to split during Chat for UX
// Adds split line to separate out sections in output.
//
void ChatSplit(bool end_line = true)
{
    std::string split_line(c_chat_separater_length, '-');
    std::cout << "\n" << split_line;
    if (end_line)
    {
        std::cout << "\n";
    }
}

//
// GenieCallBack - Callback to handle response from Genie
//   - Captures response from Genie into user_data
//   - Print response to stdout
//   - Add ChatSplit upon sentence completion
//
void GenieCallBack(const char* response_back, const GenieDialog_SentenceCode_t sentence_code, const void* user_data)
{
    std::string* user_data_str = static_cast<std::string*>(const_cast<void*>(user_data));
    user_data_str->append(response_back);

    // Write user response to output.
    std::cout << response_back;
    output += response_back;

    if (sentence_code == GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_END)
    {
        ChatSplit(false);
    }
}

} // namespace

ChatApp::ChatApp(const std::string& config)
{
    // Create Genie config
    if (GENIE_STATUS_SUCCESS != GenieDialogConfig_createFromJson(config.c_str(), &m_config_handle))
    {
        throw std::runtime_error("Failed to create the Genie Dialog config. Please check config.");
    }

    // Create Genie dialog handle
    if (GENIE_STATUS_SUCCESS != GenieDialog_create(m_config_handle, &m_dialog_handle))
    {
        throw std::runtime_error("Failed to create the Genie Dialog.");
    }
}

ChatApp::~ChatApp()
{
    if (m_config_handle != nullptr)
    {
        if (GENIE_STATUS_SUCCESS != GenieDialogConfig_free(m_config_handle))
        {
            std::cerr << "Failed to free the Genie Dialog config.";
        }
    }

    if (m_dialog_handle != nullptr)
    {
        if (GENIE_STATUS_SUCCESS != GenieDialog_free(m_dialog_handle))
        {
            std::cerr << "Failed to free the Genie Dialog.";
        }
    }
}

void ChatApp::ChatLoop()
{
    crow::SimpleApp c_app;

    // Initiate Chat with infinite loop.
    // User to provide `exit` as a prompt to exit.
    //Defining a route to handle POST requests at the /process URL

    CROW_ROUTE(c_app, "/process").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        // std::string user_prompt;
        std::string model_response;
        AppUtils::PromptHandler prompt_handler;
        crow::json::wvalue json_response;

        // Input user prompt
        ChatSplit();
        std::cout << "Input: ";
        // std::getline(std::cin, user_prompt);

        auto body_params = crow::json::load(req.body);
        if (!body_params) {
            return crow::response(400, "Invalid JSON");
        }

        std::string user_prompt = body_params["prompt"].s();
        std::cout << "Input: "<<user_prompt;
        // Exit prompt

        if (user_prompt.compare(c_exit_prompt) == 0)
        {
            std::cout << "Exiting chat per user's request.";
            return crow::response(200, "Prompt received successfully.");
        }
        // User provides an empty prompt
        if (user_prompt.empty())
        {
            std::cout << "\nPlease enter prompt.\n";
            return crow::response(400, "Prompt is empty. Please provide a valid prompt.");
        }

        std::string tagged_prompt = prompt_handler.GetPromptWithTag(user_prompt);

        // Bot's response
        std::cout << "Output: ";
        if (GENIE_STATUS_SUCCESS != GenieDialog_query(m_dialog_handle, tagged_prompt.c_str(),
                                                      GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE,
                                                      GenieCallBack, &model_response))
        {
            throw std::runtime_error("Failed to get response from GenieDialog. Please restart the ChatApp.");
        }
    json_response["answer"] = output;
    output = "";
    return crow::response(json_response);
    });
    //Change the IP address
    std::string ip_address = "0.0.0.0";
    int port_number = 8088;
    c_app.bindaddr(ip_address).port(port_number).multithreaded().run();
}
