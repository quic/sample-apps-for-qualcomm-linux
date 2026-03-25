// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include "ChatApp.hpp"
#include "crow.h"
#include "Logger.hpp"

namespace App {

// -----------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------
ChatApp::ChatApp(std::string llama_config,
                 std::string llama_base_dir,
                 std::string qwen_config,
                 std::string qwen_base_dir)
    : m_llama_config(std::move(llama_config)),
      m_llama_base_dir(std::move(llama_base_dir)),
      m_qwen_config(std::move(qwen_config)),
      m_qwen_base_dir(std::move(qwen_base_dir)),
      m_active_model(AppUtils::ModelType::LLAMA3)
{
    Logger::instance().setFile("chat_app.txt", /*append=*/true);
    Logger::instance().rotateOnSize(5 * 1024 * 1024, 3);
    Logger::instance().enableConsole(true);
    Logger::instance().setLevel(LogLevel::Debug);

    APP_LOG_DEBUG() << "Initializing LLaMA3 Genie instance";

    m_llama_genie = std::make_unique<Genie>(m_llama_config, m_llama_base_dir, /*max_tokens=*/200);
    m_llama_genie->initialize();
    m_active_genie = m_llama_genie.get();

    prompt_handler.SetModelType(AppUtils::ModelType::LLAMA3);

    APP_LOG_DEBUG() << "ChatApp initialized with LLaMA3 as active model";
}

// -----------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------
ChatApp::~ChatApp()
{
    if (m_llama_genie) m_llama_genie->cleanup();
    if (m_qwen_genie)  m_qwen_genie->cleanup();
}

// -----------------------------------------------------------------
// Run
// -----------------------------------------------------------------
void ChatApp::Run(uint16_t port)
{
    crow::SimpleApp app;

    // POST /process
    CROW_ROUTE(app, "/process").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        crow::json::wvalue json_response;
        APP_LOG_INFO() << "Incoming /process body size=" << req.body.size();

        auto body_params = crow::json::load(req.body);
        if (!body_params || !body_params.has("prompt")) {
            return crow::response(400, "Invalid JSON: missing 'prompt'");
        }

        std::string user_prompt   = body_params["prompt"].s();
        std::string tagged_prompt = prompt_handler.GetPromptWithTag(user_prompt);

        APP_LOG_DEBUG() << "Prompt: " << tagged_prompt;

        try {
            std::string answer = m_active_genie->query(
                tagged_prompt,
                GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE);
            json_response["answer"] = answer;
            json_response["status"] = "success";
            return crow::response(json_response);
        } catch (const std::exception& e) {
            APP_LOG_ERROR() << "Genie query failed: " << e.what();
            try {
                m_active_genie->reload();
                std::string answer = m_active_genie->query(
                    tagged_prompt,
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

    // POST /reset_model
    CROW_ROUTE(app, "/reset_model").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& /*req*/) {
        crow::json::wvalue json_response;
        APP_LOG_INFO() << "Incoming /reset_model";

        bool ok = m_active_genie->reset();
        if (ok) {
            prompt_handler.Reset();
            json_response["status"]  = "success";
            json_response["message"] = "Model reset successfully";
        } else {
            json_response["status"]  = "failure";
            json_response["message"] = "Model reset failed";
        }
        return crow::response(json_response);
    });

    // POST /reload_model
    CROW_ROUTE(app, "/reload_model").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        crow::json::wvalue json_response;
        APP_LOG_INFO() << "Incoming /reload_model";

        auto body_params = crow::json::load(req.body);
        if (!body_params || !body_params.has("model")) {
            return crow::response(400, "Invalid JSON: missing 'model'");
        }

        std::string model_name = body_params["model"].s();
        APP_LOG_DEBUG() << "Requested model: " << model_name;

        if (model_name == "llama") {
            if (m_active_model != AppUtils::ModelType::LLAMA3) {
	    	if (m_qwen_genie) {
	            APP_LOG_DEBUG() << "Cleaning up Qwen2 before switching to LLaMA3";
		    m_qwen_genie->cleanup();
	            m_qwen_genie.reset();
		 }
	    	if (!m_llama_initialized) {
		    APP_LOG_DEBUG() << "Re-initializing LLaMA3";
		    m_llama_genie->initialize();
		    m_llama_initialized = true;
		 }
                APP_LOG_DEBUG() << "Switching to LLaMA3";
                m_active_genie = m_llama_genie.get();
                m_active_model = AppUtils::ModelType::LLAMA3;
                prompt_handler.SetModelType(AppUtils::ModelType::LLAMA3);
		prompt_handler.SetSystemPrompt("You are Llama, a helpful AI assistant developed by Meta.");
                prompt_handler.Reset();
            }
            json_response["status"]  = "success";
            json_response["message"] = "Switched to LLaMA3";

        } else if (model_name == "qwen") {
            if (!m_qwen_genie) {
		if (m_llama_genie) {
		    APP_LOG_DEBUG() << "Cleaning up LLaMA3 before loading Qwen2";
		    m_llama_genie->cleanup();
		    m_llama_initialized = false;
		 }
                APP_LOG_DEBUG() << "Lazy-loading Qwen2 Genie instance";
                m_qwen_genie = std::make_unique<Genie>(m_qwen_config, m_qwen_base_dir, /*max_tokens=*/200);
                m_qwen_genie->initialize();
            }
            if (m_active_model != AppUtils::ModelType::QWEN2) {
                APP_LOG_DEBUG() << "Switching to Qwen2";
                m_active_genie = m_qwen_genie.get();
                m_active_model = AppUtils::ModelType::QWEN2;
                prompt_handler.SetModelType(AppUtils::ModelType::QWEN2);
		prompt_handler.SetSystemPrompt("You are Qwen, a helpful AI assistant developed by Alibaba Cloud.");
                prompt_handler.Reset();
            }
            json_response["status"]  = "success";
            json_response["message"] = "Switched to Qwen2";

        } else {
            json_response["status"]  = "failure";
            json_response["message"] = "Unknown model: " + model_name;
            return crow::response(400, json_response);
        }

        return crow::response(json_response);
    });

    // -----------------------------------------------------------------
    // Start server
    // -----------------------------------------------------------------
    APP_LOG_INFO() << "Starting HTTP server on port " << port;
    app.port(port).multithreaded().run();
}

} // namespace App
