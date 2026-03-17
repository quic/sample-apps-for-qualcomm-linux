// ---------------------------------------------------------------------
// Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <onnxruntime_c_api.h>
#include "onnxruntime_session_options_config_keys.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define INPUT_DATA_SIZE  (1 * 3 * 224 * 224)
#define OUTPUT_DATA_SIZE (1 * 1000)
#define MAX_LABEL_LEN    256
#define MAX_OPTIONS      4
#define TOP_K            5

// ---------------------------------------------------------------------------
// Helper: check an OrtStatus and print any error message.
// Returns 1 on success, 0 on failure.
// ---------------------------------------------------------------------------
static int CheckStatus(const OrtApi* ort, OrtStatus* status) {
  if (status != NULL) {
    const char* msg = ort->GetErrorMessage(status);
    fprintf(stderr, "ORT Error: %s\n", msg);
    ort->ReleaseStatus(status);
    return 0;
  }
  return 1;
}

// ---------------------------------------------------------------------------
// Helper: free all dynamically allocated node metadata arrays.
// ---------------------------------------------------------------------------
static void FreeNodeInfo(OrtAllocator* allocator,
                         char** names, size_t name_count,
                         int64_t** dims, size_t* dims_count,
                         size_t node_count) {
  if (names) {
    for (size_t i = 0; i < name_count; i++) {
      if (names[i] && allocator) allocator->Free(allocator, names[i]);
    }
    free(names);
  }
  if (dims) {
    for (size_t i = 0; i < node_count; i++) free(dims[i]);
    free(dims);
  }
  free(dims_count);
}

// ---------------------------------------------------------------------------
// Helper: release an array of OrtValue tensors.
// ---------------------------------------------------------------------------
static void ReleaseTensors(const OrtApi* ort, OrtValue** tensors, size_t count) {
  if (!tensors) return;
  for (size_t i = 0; i < count; i++) {
    if (tensors[i]) ort->ReleaseValue(tensors[i]);
  }
  free(tensors);
}

// ---------------------------------------------------------------------------
// Core inference routine using the QNN Execution Provider.
// ---------------------------------------------------------------------------
static void run_ort_qnn_ep(const char* backend,
                           const char* model_path,
                           const char* input_path,
                           int generate_ctx,
                           int float32_model) {
  const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);

  // ORT handles
  OrtEnv*            env             = NULL;
  OrtSessionOptions* session_options = NULL;
  OrtSession*        session         = NULL;
  OrtAllocator*      allocator       = NULL;
  OrtMemoryInfo*     memory_info     = NULL;

  // Input node metadata
  size_t                      num_input_nodes      = 0;
  char**                      input_node_names     = NULL;
  int64_t**                   input_node_dims      = NULL;
  size_t*                     input_node_dims_count = NULL;
  ONNXTensorElementDataType*  input_types          = NULL;
  OrtValue**                  input_tensors        = NULL;

  // Output node metadata
  size_t     num_output_nodes       = 0;
  char**     output_node_names      = NULL;
  int64_t**  output_node_dims       = NULL;
  size_t*    output_node_dims_count = NULL;
  OrtValue** output_tensors         = NULL;

  // Data buffers / file handles
  float* input_data      = NULL;
  FILE*  input_raw_file  = NULL;
  FILE*  label_file      = NULL;

  // -------------------------------------------------------------------------
  // 1. Create ORT environment
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "qnn_sample", &env)))
    goto cleanup;

  // -------------------------------------------------------------------------
  // 2. Configure session options
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->CreateSessionOptions(&session_options)))         goto cleanup;
  if (!CheckStatus(ort, ort->SetIntraOpNumThreads(session_options, 1)))       goto cleanup;
  if (!CheckStatus(ort, ort->SetSessionGraphOptimizationLevel(
                            session_options, ORT_ENABLE_BASIC)))              goto cleanup;

  // -------------------------------------------------------------------------
  // 3. Append QNN Execution Provider
  // -------------------------------------------------------------------------
  {
    const char* options_keys[MAX_OPTIONS];
    const char* options_values[MAX_OPTIONS];
    size_t num_options = 0;

    // Required: path to the QNN backend shared library
    options_keys[num_options]   = "backend_path";
    options_values[num_options] = backend;
    num_options++;

    // For float32 models on HTP, enable FP16 precision to avoid slow FP32 execution
    if (float32_model) {
      options_keys[num_options]   = "enable_htp_fp16_precision";
      options_values[num_options] = "1";
      num_options++;
    }

    // Optionally generate a QNN context binary embedded in an EPContext ONNX model
    if (generate_ctx) {
      if (!CheckStatus(ort, ort->AddSessionConfigEntry(
                              session_options, kOrtSessionOptionEpContextEnable, "1")))
        goto cleanup;
    }

    if (!CheckStatus(ort, ort->SessionOptionsAppendExecutionProvider(
                            session_options, "QNN",
                            options_keys, options_values, num_options)))
      goto cleanup;
  }

  // -------------------------------------------------------------------------
  // 4. Create session (loads and compiles the model)
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->CreateSession(env, model_path, session_options, &session)))
    goto cleanup;

  if (generate_ctx) {
    printf("\nONNX model with embedded QNN context binary has been generated.\n");
    goto cleanup;
  }

  // -------------------------------------------------------------------------
  // 5. Retrieve default allocator
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->GetAllocatorWithDefaultOptions(&allocator)))
    goto cleanup;

  // -------------------------------------------------------------------------
  // 6. Query input node metadata
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->SessionGetInputCount(session, &num_input_nodes)))
    goto cleanup;

  input_node_names      = (char**)calloc(num_input_nodes, sizeof(char*));
  input_node_dims       = (int64_t**)calloc(num_input_nodes, sizeof(int64_t*));
  input_node_dims_count = (size_t*)calloc(num_input_nodes, sizeof(size_t));
  input_types           = (ONNXTensorElementDataType*)calloc(num_input_nodes,
                            sizeof(ONNXTensorElementDataType));
  input_tensors         = (OrtValue**)calloc(num_input_nodes, sizeof(OrtValue*));

  if (!input_node_names || !input_node_dims || !input_node_dims_count ||
      !input_types || !input_tensors) {
    fprintf(stderr, "Failed to allocate memory for input node metadata.\n");
    goto cleanup;
  }

  for (size_t i = 0; i < num_input_nodes; i++) {
    OrtTypeInfo*                    type_info   = NULL;
    const OrtTensorTypeAndShapeInfo* tensor_info = NULL;
    size_t num_dims = 0;

    if (!CheckStatus(ort, ort->SessionGetInputName(session, i, allocator, &input_node_names[i])))
      goto cleanup;

    if (!CheckStatus(ort, ort->SessionGetInputTypeInfo(session, i, &type_info)))
      goto cleanup;

    if (!CheckStatus(ort, ort->CastTypeInfoToTensorInfo(type_info, &tensor_info))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    if (!CheckStatus(ort, ort->GetTensorElementType(tensor_info, &input_types[i]))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    if (!CheckStatus(ort, ort->GetDimensionsCount(tensor_info, &num_dims))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    input_node_dims_count[i] = num_dims;
    input_node_dims[i] = (int64_t*)malloc(num_dims * sizeof(int64_t));
    if (!input_node_dims[i]) {
      ort->ReleaseTypeInfo(type_info);
      fprintf(stderr, "Failed to allocate memory for input dims[%zu].\n", i);
      goto cleanup;
    }

    if (!CheckStatus(ort, ort->GetDimensions(tensor_info, input_node_dims[i], num_dims))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    ort->ReleaseTypeInfo(type_info);
  }

  // -------------------------------------------------------------------------
  // 7. Query output node metadata
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->SessionGetOutputCount(session, &num_output_nodes)))
    goto cleanup;

  output_node_names      = (char**)calloc(num_output_nodes, sizeof(char*));
  output_node_dims       = (int64_t**)calloc(num_output_nodes, sizeof(int64_t*));
  output_node_dims_count = (size_t*)calloc(num_output_nodes, sizeof(size_t));
  output_tensors         = (OrtValue**)calloc(num_output_nodes, sizeof(OrtValue*));

  if (!output_node_names || !output_node_dims || !output_node_dims_count || !output_tensors) {
    fprintf(stderr, "Failed to allocate memory for output node metadata.\n");
    goto cleanup;
  }

  for (size_t i = 0; i < num_output_nodes; i++) {
    OrtTypeInfo*                    type_info   = NULL;
    const OrtTensorTypeAndShapeInfo* tensor_info = NULL;
    size_t num_dims = 0;

    if (!CheckStatus(ort, ort->SessionGetOutputName(session, i, allocator, &output_node_names[i])))
      goto cleanup;

    if (!CheckStatus(ort, ort->SessionGetOutputTypeInfo(session, i, &type_info)))
      goto cleanup;

    if (!CheckStatus(ort, ort->CastTypeInfoToTensorInfo(type_info, &tensor_info))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    if (!CheckStatus(ort, ort->GetDimensionsCount(tensor_info, &num_dims))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    output_node_dims_count[i] = num_dims;
    output_node_dims[i] = (int64_t*)malloc(num_dims * sizeof(int64_t));
    if (!output_node_dims[i]) {
      ort->ReleaseTypeInfo(type_info);
      fprintf(stderr, "Failed to allocate memory for output dims[%zu].\n", i);
      goto cleanup;
    }

    if (!CheckStatus(ort, ort->GetDimensions(tensor_info, output_node_dims[i], num_dims))) {
      ort->ReleaseTypeInfo(type_info);
      goto cleanup;
    }

    ort->ReleaseTypeInfo(type_info);
  }

  // -------------------------------------------------------------------------
  // 8. Prepare input data (default: all 1.0f; overridden by raw file if present)
  // -------------------------------------------------------------------------
  input_data = (float*)malloc(INPUT_DATA_SIZE * sizeof(float));
  if (!input_data) {
    fprintf(stderr, "Failed to allocate memory for input data.\n");
    goto cleanup;
  }

  // Fill with a default value
  for (size_t i = 0; i < INPUT_DATA_SIZE; i++) input_data[i] = 1.0f;

  // Attempt to load raw float data from file
  input_raw_file = fopen(input_path, "rb");
  if (input_raw_file) {
    fseek(input_raw_file, 0, SEEK_END);
    long file_size    = ftell(input_raw_file);
    size_t num_floats = (size_t)(file_size / sizeof(float));
    fseek(input_raw_file, 0, SEEK_SET);
    fread(input_data, sizeof(float), num_floats, input_raw_file);
    fclose(input_raw_file);
    input_raw_file = NULL;
    printf("Loaded %zu float(s) from '%s'.\n", num_floats, input_path);
  } else {
    fprintf(stderr, "Warning: could not open '%s'; using default input (all 1.0f).\n", input_path);
  }

  // -------------------------------------------------------------------------
  // 9. Create input tensor
  // -------------------------------------------------------------------------
  if (!CheckStatus(ort, ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info)))
    goto cleanup;

  {
    size_t input_data_bytes = INPUT_DATA_SIZE * sizeof(float);
    if (!CheckStatus(ort, ort->CreateTensorWithDataAsOrtValue(
                            memory_info, (void*)input_data, input_data_bytes,
                            input_node_dims[0], input_node_dims_count[0],
                            input_types[0], &input_tensors[0])))
      goto cleanup;
  }

  ort->ReleaseMemoryInfo(memory_info);
  memory_info = NULL;

  // -------------------------------------------------------------------------
  // 10. Run inference and measure wall-clock time
  // -------------------------------------------------------------------------
  {
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    if (!CheckStatus(ort, ort->Run(session, NULL,
                                   (const char* const*)input_node_names,
                                   (const OrtValue* const*)input_tensors,
                                   num_input_nodes,
                                   (const char* const*)output_node_names,
                                   num_output_nodes,
                                   output_tensors)))
      goto cleanup;

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double duration_ms = (t_end.tv_sec  - t_start.tv_sec)  * 1000.0
                       + (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
    printf("Inference time: %.1f ms\n", duration_ms);
  }

  // -------------------------------------------------------------------------
  // 11. Post-process: find the top-1 class and print the label
  // -------------------------------------------------------------------------
  {
    void*  raw_buffer  = NULL;
    float* float_buffer = NULL;

    if (!CheckStatus(ort, ort->GetTensorMutableData(output_tensors[0], &raw_buffer)))
      goto cleanup;

    float_buffer = (float*)raw_buffer;

    // Find argmax
    float max_val  = float_buffer[0];
    int   max_idx  = 0;
    for (size_t i = 1; i < OUTPUT_DATA_SIZE; i++) {
      if (float_buffer[i] > max_val) {
        max_val = float_buffer[i];
        max_idx = (int)i;
      }
    }

    // Attempt to load ImageNet labels from synset.txt
    label_file = fopen("synset.txt", "r");
    if (label_file) {
      char labels[OUTPUT_DATA_SIZE][MAX_LABEL_LEN];
      int  label_count = 0;
      memset(labels, 0, sizeof(labels));

      while (label_count < OUTPUT_DATA_SIZE &&
             fgets(labels[label_count], MAX_LABEL_LEN, label_file)) {
        // Strip trailing newline
        size_t len = strlen(labels[label_count]);
        if (len > 0 && labels[label_count][len - 1] == '\n')
          labels[label_count][len - 1] = '\0';
        label_count++;
      }
      fclose(label_file);
      label_file = NULL;

      printf("\nTop-1 Result:\n");
      printf("  index=%d  label=%s  score=%.6f\n",
             max_idx,
             (max_idx < label_count) ? labels[max_idx] : "unknown",
             max_val);
    } else {
      fprintf(stderr, "Warning: could not open synset.txt; labels unavailable.\n");
      printf("\nTop-1 Result:\n");
      printf("  index=%d  score=%.6f\n", max_idx, max_val);
    }
  }

// ---------------------------------------------------------------------------
// Cleanup: release all ORT objects and free heap memory
// ---------------------------------------------------------------------------
cleanup:
  if (input_raw_file) fclose(input_raw_file);
  if (label_file)     fclose(label_file);
  if (memory_info)    ort->ReleaseMemoryInfo(memory_info);

  ReleaseTensors(ort, input_tensors,  num_input_nodes);
  ReleaseTensors(ort, output_tensors, num_output_nodes);

  FreeNodeInfo(allocator, input_node_names,  num_input_nodes,
               input_node_dims,  input_node_dims_count,  num_input_nodes);
  FreeNodeInfo(allocator, output_node_names, num_output_nodes,
               output_node_dims, output_node_dims_count, num_output_nodes);

  free(input_types);
  free(input_data);

  if (session)         ort->ReleaseSession(session);
  if (session_options) ort->ReleaseSessionOptions(session_options);
  if (env)             ort->ReleaseEnv(env);
}

// ---------------------------------------------------------------------------
// Usage / help text
// ---------------------------------------------------------------------------
static void PrintHelp(const char* prog) {
  printf("Usage: %s <backend_flag> <model_path> <input_raw_path> [--gen_ctx]\n\n", prog);
  printf("Backend flags:\n");
  printf("  --cpu   Run on QNN CPU backend  (e.g. Inception-v3_float.onnx)\n");
  printf("  --htp   Run on QNN HTP backend  (e.g. Inception-v3_w8a8.onnx)\n");
  printf("  --qnn   Run a pre-compiled QNN context binary on HTP\n");
  printf("  --fp32  Run a float32 model on HTP with FP16 precision\n");
  printf("  --fp16  Run a float16 model on HTP\n\n");
  printf("Optional flags:\n");
  printf("  --gen_ctx  Generate an ONNX model with embedded QNN context binary\n\n");
  printf("Examples:\n");
  printf("  %s --cpu  Inception-v3_float.onnx input.raw\n", prog);
  printf("  %s --htp  Inception-v3_w8a8.onnx  input.raw\n", prog);
  printf("  %s --htp  Inception-v3_w8a8.onnx  input.raw --gen_ctx\n", prog);
  printf("  %s --qnn  qnn_ctx_binary.onnx      input_nhwc.raw\n", prog);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  // Supported command-line flags
  static const char* FLAG_CPU      = "--cpu";
  static const char* FLAG_HTP      = "--htp";
  static const char* FLAG_QNN      = "--qnn";
  static const char* FLAG_FP32     = "--fp32";
  static const char* FLAG_FP16     = "--fp16";
  static const char* FLAG_GEN_CTX  = "--gen_ctx";

  const char* backend     = NULL;
  const char* model_path  = NULL;
  const char* input_path  = NULL;
  int generate_ctx        = 0;
  int float32_model       = 0;

  // Expect exactly 3 positional args + optional --gen_ctx
  if (argc != 4 && argc != 5) {
    PrintHelp(argv[0]);
    return EXIT_FAILURE;
  }

  // Parse optional 5th argument
  if (argc == 5) {
    if (strcmp(argv[4], FLAG_GEN_CTX) == 0) {
      generate_ctx = 1;
    } else {
      fprintf(stderr, "Unknown option '%s'. Expected '%s'.\n", argv[4], FLAG_GEN_CTX);
      PrintHelp(argv[0]);
      return EXIT_FAILURE;
    }
  }

  // Parse backend flag
  if (strcmp(argv[1], FLAG_CPU) == 0) {
    backend = "libQnnCpu.so";
    if (generate_ctx) {
      fprintf(stderr, "--gen_ctx is not supported with --cpu.\n");
      return EXIT_FAILURE;
    }
  } else if (strcmp(argv[1], FLAG_HTP) == 0) {
    backend = "libQnnHtp.so";
  } else if (strcmp(argv[1], FLAG_QNN) == 0) {
    backend = "libQnnHtp.so";
    if (generate_ctx) {
      fprintf(stderr, "--gen_ctx is not supported with --qnn.\n");
      return EXIT_FAILURE;
    }
  } else if (strcmp(argv[1], FLAG_FP32) == 0) {
    backend      = "libQnnHtp.so";
    float32_model = 1;
  } else if (strcmp(argv[1], FLAG_FP16) == 0) {
    backend = "libQnnHtp.so";
  } else {
    fprintf(stderr, "Unknown backend flag '%s'.\n", argv[1]);
    PrintHelp(argv[0]);
    return EXIT_FAILURE;
  }

  model_path = argv[2];
  input_path = argv[3];

  // Use the "C" locale for consistent numeric formatting
  setlocale(LC_ALL, "C");

  run_ort_qnn_ep(backend, model_path, input_path, generate_ctx, float32_model);

  printf("-------------------- done --------------------\n");
  return EXIT_SUCCESS;
}
