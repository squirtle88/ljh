#ifndef UI_H
#define UI_H

#include "ds.h"

/* 登录界面，返回登录成功的用户角色，失败返回 -1 */
int ui_login(UserDatabase *db, UserRole *out_role, char out_username[32]);

/* 管理员-用户管理子菜单 */
void ui_user_manage(UserDatabase *db);

/* 角色对应的主菜单 */
void ui_admin_menu(ParkingSystem *ps);
void ui_operator_menu(ParkingSystem *ps);
void ui_customer_menu(ParkingSystem *ps, const char *my_plate);

/* 各功能界面 */
void ui_entry(ParkingSystem *ps);
void ui_exit(ParkingSystem *ps);
void ui_find(ParkingSystem *ps);
void ui_free_spots(ParkingSystem *ps);
void ui_wait_queue(ParkingSystem *ps);
void ui_parking_map(ParkingSystem *ps);
void ui_revenue(ParkingSystem *ps);
void ui_transactions(ParkingSystem *ps);
void ui_floor_usage(ParkingSystem *ps);
void ui_peak_hours(ParkingSystem *ps);
void ui_save(ParkingSystem *ps);
void ui_load(ParkingSystem *ps);

#endif
