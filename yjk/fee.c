#include "fee.h"

#include <math.h>

double calculate_fee(const FeeRule *rule, int duration_minutes) {
    if (rule == NULL || duration_minutes <= rule->free_minutes) {
        return 0.0;
    }

    int charged_minutes = duration_minutes - rule->free_minutes;
    double charged_hours = ceil(charged_minutes / 60.0);
    double fee = rule->base_fee + charged_hours * rule->rate_per_hour;

    if (rule->max_daily > 0.0 && fee > rule->max_daily) {
        fee = rule->max_daily;
    }

    return fee;
}

void set_fee_rule(ParkingSystem *ps, FeeRule rule) {
    if (ps == NULL) {
        return;
    }

    ps->fee_rule = rule;
}
