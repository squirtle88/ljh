#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include "ds.h"
#include "parking.h"
#include "fee.h"
#include "stats.h"
#include "ui.h"
#include "file_io.h"

int main() {
    ParkingSystem ps;

    /* 1. Initialize system */
    printf("正在初始化停车场系统...\n");
    ds_init(&ps);

    /* 2. Try loading saved data */
    if (load_data(&ps, "parking_data.dat")) {
        /* Rebuild hash table (hash table is not persisted) */
        rebuild_hash_from_spots(&ps);
    }

    printf("停车场初始化完成！共 %d 个车位\n", TOTAL_SPOTS);

    /* 3. Enter main menu loop */
    ui_main_menu(&ps);

    /* 4. Save data before exit */
    printf("正在保存数据...\n");
    save_data(&ps, "parking_data.dat");

    /* 5. Release all memory */
    ds_destroy(&ps);
    printf("系统已安全退出\n");

    return 0;
}
