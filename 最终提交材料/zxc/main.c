#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "ds.h"
#include "parking.h"
#include "fee.h"
#include "stats.h"
#include "ui.h"
#include "file_io.h"

static void configure_console_encoding(void) {
#ifdef _WIN32
    SetConsoleOutputCP(936);
    SetConsoleCP(936);
#endif
}

int main() {
    ParkingSystem ps;
    UserRole role;
    char username[32];
    char my_plate[16] = "";

    configure_console_encoding();

    /* 显示工作目录 */
#ifdef _WIN32
    {
        char cwd[512];
        if (GetCurrentDirectoryA(sizeof(cwd), cwd))
            printf("工作目录: %s\n", cwd);
    }
#endif

    /* 1. Initialize system */
    printf("正在初始化停车场系统...\n");
    ds_init(&ps);

    /* 2. Load users; if no file, create default admin */
    if (!load_users(&ps.user_db, "data/users.dat")) {
        user_db_add(&ps.user_db, "admin", "admin123", USER_ADMIN, "");
        printf("已创建默认管理员账号 (admin / admin123)\n");
    }

    /* 3. Login */
    if (!ui_login(&ps.user_db, &role, username)) {
        printf("登录失败，程序退出。\n");
        save_users(&ps.user_db, "data/users.dat");
        ds_destroy(&ps);
        return 1;
    }

    /* 4. Get plate for customer role */
    if (role == USER_CUSTOMER) {
        for (int i = 0; i < ps.user_db.count; i++) {
            if (strcmp(ps.user_db.users[i].username, username) == 0) {
                strcpy(my_plate, ps.user_db.users[i].plate);
                break;
            }
        }
    }

    /* 5. Load parking data */
    if (load_data(&ps, "data/parking_data.dat")) {
        rebuild_hash_from_spots(&ps);
    }

    printf("停车场初始化完成！共 %d 个车位\n", TOTAL_SPOTS);
    printf("当前用户: %s  ", username);

    /* 6. Route to role-based menu */
    switch (role) {
        case USER_ADMIN:
            printf("[管理员 — 全部功能]\n");
            ui_admin_menu(&ps);
            break;
        case USER_OPERATOR:
            printf("[操作员 — 出入场管理]\n");
            ui_operator_menu(&ps);
            break;
        case USER_CUSTOMER:
            printf("[车主 — %s]\n", my_plate);
            ui_customer_menu(&ps, my_plate);
            break;
        default:
            printf("[未知角色]\n");
            break;
    }

    /* 7. Save before exit */
    printf("正在保存数据...\n");
    save_data(&ps, "data/parking_data.dat");
    save_users(&ps.user_db, "data/users.dat");

    /* 8. Release all memory */
    ds_destroy(&ps);
    printf("系统已安全退出\n");

    return 0;
}
