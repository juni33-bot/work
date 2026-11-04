/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tflite_det_app.h"

#define INPUT_WIDTH 640
#define INPUT_HEIGHT 384
#define INPUT_CHANNEL 3

int main(int argc, char* argv[])
{
  char* model_file_name = "model.tflite";
  char* input_file_name = "input.png";

  if (argc > 1) model_file_name = argv[1];
  if (argc > 2) input_file_name = argv[2];

  // Load model
  std::unique_ptr<tflite::FlatBufferModel> model =
      tflite::FlatBufferModel::BuildFromFile(model_file_name);

  // Build the interpreter
  tflite::ops::builtin::BuiltinOpResolver resolver;
  tflite::InterpreterBuilder builder(*model, resolver);
  std::unique_ptr<tflite::Interpreter> interpreter;
  builder(&interpreter);

  // Allocate tensor buffers.
  interpreter->AllocateTensors();

  // Get input tensor
  int input_index = interpreter->inputs()[0];
  TfLiteTensor* input_tensor = interpreter->tensor(input_index);

#if USE_GPU
  // Prepare a GPU delegate.
  TfLiteGpuDelegateOptionsV2 options = TfLiteGpuDelegateOptionsV2Default();
  TfLiteDelegate* delegate = TfLiteGpuDelegateV2Create(&options);
  interpreter->ModifyGraphWithDelegate(delegate);
#endif

  // Load the input file
  cv::Mat image = cv::imread(input_file_name);
  cv::Mat image_resized;
  cv::resize(image, image_resized, {INPUT_WIDTH, INPUT_HEIGHT});

  // Fill input tensor
  std::memcpy(input_tensor->data.f, image_resized.data, 
              INPUT_WIDTH * INPUT_HEIGHT * INPUT_CHANNEL);

  // Run inference
  interpreter->Invoke();

  // Read output tensor
  int output_tensor_index = interpreter->outputs()[0];
  TfLiteTensor* output_tensor = interpreter->tensor(output_tensor_index);

  // To Do: output data manipulation

#if USE_GPU
  // Clean up the GPU delegate
  TfLiteGpuDelegateV2Delete(delegate);
#endif

  return 0;
}

