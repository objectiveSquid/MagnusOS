#pragma once
#include "irq.h"
#include "pic.h"
#include <stdbool.h>
#include <stdint.h>

const PICDriver *i8259_GetDriver();
