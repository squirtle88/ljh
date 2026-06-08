#ifndef FEE_H
#define FEE_H

#include "../pzx/ds.h"

double calculate_fee(const FeeRule *rule, int duration_minutes);
void set_fee_rule(ParkingSystem *ps, FeeRule rule);

#endif
