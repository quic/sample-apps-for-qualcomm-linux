// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "ChatApp.hpp"
#include "PromptHandler.hpp"
#include "crow.h"
#include <iostream>
#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace App;
using json = nlohmann::json;
json json_config_;
AppUtils::PromptHandler prompt_handler;

ChatApp::ChatApp(const std::string& config)
    : config_(config),
      genie_(config_, /*max_tokens=*/200)  // pass default tokens
{
    Logger::instance().setFile("llamachat.txt", /*append=*/true);
    Logger::instance().rotateOnSize(5 * 1024 * 1024, 3);
    Logger::instance().enableConsole(true);
    Logger::instance().setLevel(LogLevel::Debug);

    APP_LOG_DEBUG() << "Everything is setup properly";
    // Initialize Genie once at startup
    genie_.initialize();
}

ChatApp::~ChatApp() {
    // Genie destructor cleans up resources
    genie_.cleanup();
}

void ChatApp::ChatLoop() {
    crow::SimpleApp c_app;

    // /process
    CROW_ROUTE(c_app, "/process").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
            crow::json::wvalue json_response;
            APP_LOG_INFO()  << "Incoming /process body size=" << req.body.size();
            auto body_params = crow::json::load(req.body);
            if (!body_params || !body_params.has("prompt")) {
                return crow::response(400, "Invalid JSON: missing 'prompt'");
            }
            std::string user_prompt = body_params["prompt"].s();
            // AppUtils::PromptHandler prompt_handler;
            std::string tagged_prompt = prompt_handler.GetPromptWithTag(user_prompt);

            APP_LOG_DEBUG() << "Prompt: " << tagged_prompt;
            //Asking question
            try {
                std::string answer = genie_.query(tagged_prompt,
                    GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE);
                json_response["answer"] = answer;
                json_response["status"] = "success";
                return crow::response(json_response);
            } catch (const std::exception& e) {  // If it fails then again reload the model and try to ask again
                APP_LOG_ERROR() << "Genie query failed: " << e.what();
                try {
                    genie_.reload();
                    std::string answer = genie_.query(tagged_prompt,
                        GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE);
                    json_response["answer"] = answer;
                    json_response["status"] = "success";
                    return crow::response(json_response);
                } catch (const std::exception& e2) {
                    json_response["answer"] = std::string("Genie query failed: ") + e2.what();
                    json_response["status"] = "failure";
                    return crow::response(json_response);
                }
            }
        });

    // /reset_model
    CROW_ROUTE(c_app, "/reset_model").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
            crow::json::wvalue json_response;
            bool ok = genie_.reset();
            APP_LOG_DEBUG() << "ResetModel: " << ok;
            if (ok) {
                json_response["answer"] = "Model reset successful";
                json_response["status"] = "success";
                return crow::response(json_response);
            } else {
                json_response["answer"] = "Model reset unsuccessful";
                json_response["status"] = "failure";
                return crow::response(json_response);
            }
        }
    );
    // /reload_model
    CROW_ROUTE(c_app, "/reload_model").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
            crow::json::wvalue json_response;
            try {
                auto body = crow::json::load(req.body);
                if (!body) {
                    json_response["answer"] = "Invalid JSON payload";
                    json_response["status"] = "failure";
                    return crow::response(400, json_response);
                }
                std::string system_prompt = body["system_prompt"].s();
                std::string sampler_block = body["sampler_block"].s();
                int max_tokens = body["max_tokens"].i();

                genie_.applySamplerConfig(sampler_block);

                if (max_tokens >= 0 /* and <= MODEL_MAX_TOKENS */) {
                    genie_.setMaxTokens(static_cast<uint32_t>(max_tokens));
                }
                // Initialize with the system prompt
                prompt_handler.SetSystemPrompt(std::move(system_prompt));
                bool ok = genie_.reset();
                json_response["answer"] = "Model reload successful";
                json_response["status"] = "success";
                return crow::response(json_response);
            } catch (const std::exception& e) {
                json_response["answer"] = std::string("Model reload failed: ") + e.what();
                json_response["status"] = "failure";
                return crow::response(json_response);
            }
        });

    // Bind & run (adjust as needed)
    c_app.bindaddr("0.0.0.0").port(8088).multithreaded().run();
}

