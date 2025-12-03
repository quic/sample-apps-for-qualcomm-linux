// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "ChatApp.hpp"
#include "PromptHandler.hpp"
#include "crow.h"
#include <iostream>
#include "Logger.hpp"

using namespace App;

ChatApp::ChatApp(const std::string& config)
    : config_(config),
      genie_(config_, /*max_tokens=*/200)  // pass default tokens
{
    
   
    // Initialize once (e.g., ChatApp ctor)
    Logger::instance().setFile("/var/log/llamachat.txt", /*append=*/true);
    Logger::instance().rotateOnSize(5 * 1024 * 1024, 3);
    Logger::instance().enableConsole(true);
    Logger::instance().setLevel(LogLevel::Debug);
          
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
            
            AppUtils::PromptHandler prompt_handler;
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
                genie_.reload();
                json_response["answer"] = "Model reload successful";
                json_response["status"] = "success";
                return crow::response(json_response);
            } catch (const std::exception& e) {
                json_response["answer"] = std::string("Model reload failed: ") + e.what();
                json_response["status"] = "failure";
                return crow::response(json_response);
            }
        }
    );

    // Bind & run (adjust as needed)
    c_app.bindaddr("0.0.0.0").port(8088).multithreaded().run();
}

