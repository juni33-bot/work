#include <map>
#include <cstdio>
#include <signal.h>
#include <sys/time.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>
#include <new>

#include "tensorflow/lite/core/interpreter_builder.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model_builder.h"
#include "tensorflow/lite/optional_debug_tools.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"

#include "opencv.hpp"