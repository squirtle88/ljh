#include "ds.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void test_stack(void) {
    FreeSpotStack stack;
    SpotLocation a = make_loc(0, 0, 0);
    SpotLocation b = make_loc(1, 2, 3);

    stack_init(&stack, 1);
    assert(stack_is_empty(&stack));

    stack_push(&stack, a);
    stack_push(&stack, b);
    assert(stack_size(&stack) == 2);
    assert(loc_equal(stack_pop(&stack), b));
    assert(loc_equal(stack_pop(&stack), a));
    assert(stack_is_empty(&stack));

    stack_destroy(&stack);
}

static void test_queue(void) {
    WaitingQueue queue;
    char plate[16];
    time_t t;

    queue_init(&queue);
    assert(queue_is_empty(&queue));

    queue_enqueue(&queue, "A10001", 10);
    queue_enqueue(&queue, "A10002", 20);
    assert(queue_count(&queue) == 2);

    assert(queue_dequeue(&queue, plate, &t) == 1);
    assert(strcmp(plate, "A10001") == 0);
    assert(t == 10);

    assert(queue_dequeue(&queue, plate, &t) == 1);
    assert(strcmp(plate, "A10002") == 0);
    assert(t == 20);

    assert(queue_dequeue(&queue, plate, &t) == 0);
    assert(queue_is_empty(&queue));
}

static void test_hash_map(void) {
    HashMap map;
    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
    assert(rec != NULL);

    strcpy(rec->plate, "B20001");
    rec->loc = make_loc(2, 3, 4);
    rec->entry_time = 123;

    ht_init(&map);
    ht_insert(&map, rec);
    assert(ht_find(&map, "B20001") == rec);
    assert(ht_find(&map, "NO_SUCH") == NULL);

    ht_remove(&map, "B20001");
    assert(ht_find(&map, "B20001") == NULL);

    free(rec);
    ht_clear(&map);
}

static void test_system_init(void) {
    ParkingSystem ps;

    ds_init(&ps);
    assert(stack_size(&ps.free_spots) == TOTAL_SPOTS);
    assert(queue_count(&ps.wait_queue) == 0);
    assert(ps.plate_index.count == 0);
    assert(ps.history_count == 0);
    assert(ps.history_capacity == 100);
    assert(ps.total_revenue == 0.0);
    assert(ps.fee_rule.base_fee == 5.0);
    assert(ps.fee_rule.free_minutes == 15);
    assert(ps.fee_rule.rate_per_hour == 4.0);
    assert(ps.fee_rule.max_daily == 40.0);

    for (int f = 0; f < FLOORS; f++) {
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                assert(ps.spots[f][r][c].status == SPOT_FREE);
                assert(ps.spots[f][r][c].plate[0] == '\0');
                assert(loc_equal(ps.spots[f][r][c].loc, make_loc(f, r, c)));
            }
        }
    }

    ds_destroy(&ps);
}

int main(void) {
    test_stack();
    test_queue();
    test_hash_map();
    test_system_init();

    printf("ds tests passed\n");
    return 0;
}
