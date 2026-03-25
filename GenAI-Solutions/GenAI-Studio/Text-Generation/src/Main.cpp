// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "ChatApp.hpp"

namespace
{

constexpr const std::string_view c_option_llama_config  = "--llama-config";
constexpr const std::string_view c_option_llama_base_dir = "--llama-base-dir";
constexpr const std::string_view c_option_qwen_config   = "--qwen-config";
constexpr const std::string_view c_option_qwen_base_dir = "--qwen-base-dir";
constexpr const std::string_view c_option_help          = "--help";
constexpr const std::string_view c_option_help_short    = "-h";

void PrintHelp()
{
    std::cout << "\n:::::::: Chat options ::::::::\n\n";
    std::cout << c_option_llama_config   << " <Local file path>: [Required] Path to LLaMA3 Genie config JSON.\n";
    std::cout << c_option_llama_base_dir << " <Local directory path>: [Required] Base directory for LLaMA3 bundle.\n";
    std::cout << c_option_qwen_config    << " <Local file path>: [Required] Path to Qwen2.5 Genie config JSON.\n";
    std::cout << c_option_qwen_base_dir  << " <Local directory path>: [Required] Base directory for Qwen2.5 bundle.\n";
}

void PrintWelcomeMessage()
{
    std::cout << "\n:::::::: Welcome to ChatApp ::::::::\n ";
    std::cout << "This demonstrates a multi-turn chat with an LLM.\n ";
}

} // namespace

int main(int argc, char* argv[])
{
    std::string llama_config_path;
    std::string llama_base_dir;
    std::string qwen_config_path;
    std::string qwen_base_dir;
    bool invalid_arguments = false;

    // Check if argument file path is accessible
    auto check_arg_path = [&invalid_arguments](const std::string_view& arg_name, const std::string& value)
    {
        if (value.empty() || !std::filesystem::exists(value))
        {
            std::cout << "\nInvalid argument for " << arg_name
                      << ": It must be a valid and accessible local path. Provided: " << value << std::endl;
            invalid_arguments = true;
        }
    };

    // Arg parser
    for (int i = 1; i < argc; ++i)
    {
        if (c_option_llama_config == argv[i])
        {
            if (i + 1 < argc)
            {
                llama_config_path = argv[++i];
                check_arg_path(c_option_llama_config, llama_config_path);
            }
            else
            {
                std::cout << "\nMissing value for " << c_option_llama_config << " option.\n";
                invalid_arguments = true;
            }
        }
        else if (c_option_llama_base_dir == argv[i])
        {
            if (i + 1 < argc)
            {
                llama_base_dir = argv[++i];
                check_arg_path(c_option_llama_base_dir, llama_base_dir);
            }
            else
            {
                std::cout << "\nMissing value for " << c_option_llama_base_dir << " option.\n";
                invalid_arguments = true;
            }
        }
        else if (c_option_qwen_config == argv[i])
        {
            if (i + 1 < argc)
            {
                qwen_config_path = argv[++i];
                check_arg_path(c_option_qwen_config, qwen_config_path);
            }
            else
            {
                std::cout << "\nMissing value for " << c_option_qwen_config << " option.\n";
                invalid_arguments = true;
            }
        }
        else if (c_option_qwen_base_dir == argv[i])
        {
            if (i + 1 < argc)
            {
                qwen_base_dir = argv[++i];
                check_arg_path(c_option_qwen_base_dir, qwen_base_dir);
            }
            else
            {
                std::cout << "\nMissing value for " << c_option_qwen_base_dir << " option.\n";
                invalid_arguments = true;
            }
        }
        else if (c_option_help == argv[i] || c_option_help_short == argv[i])
        {
            PrintHelp();
            return 0;
        }
        else
        {
            std::cout << "Unsupported option " << argv[i] << " provided.\n";
            invalid_arguments = true;
        }
    }

    // If invalid arguments or required arguments are missing, print help and exit.
    if (invalid_arguments || llama_config_path.empty() || llama_base_dir.empty()
                          || qwen_config_path.empty()  || qwen_base_dir.empty())
    {
        PrintHelp();
        return 1;
    }

    try
    {
        // Load LLaMA config JSON
        std::string llama_config;
        {
            std::ifstream config_file(llama_config_path);
            if (!config_file)
            {
                throw std::runtime_error("Failed to open LLaMA config file: " + llama_config_path);
            }
            llama_config.assign((std::istreambuf_iterator<char>(config_file)),
                                 std::istreambuf_iterator<char>());
        }

        // Load Qwen config JSON
        std::string qwen_config;
        {
            std::ifstream config_file(qwen_config_path);
            if (!config_file)
            {
                throw std::runtime_error("Failed to open Qwen config file: " + qwen_config_path);
            }
            qwen_config.assign((std::istreambuf_iterator<char>(config_file)),
                                std::istreambuf_iterator<char>());
        }

        PrintWelcomeMessage();

        App::ChatApp app(llama_config, llama_base_dir, qwen_config, qwen_base_dir);

        app.Run(8088);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error.\n";
        return 1;
    }
    return 0;
}
