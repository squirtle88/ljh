#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"

int save_data(ParkingSystem *ps, const char *filename) {
    if (ps == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法打开文件 %s 写入\n", filename);
        return 0;
    }

    /* 1. Write file header */
    FileHeader header;
    header.magic         = FILE_MAGIC;
    header.version       = FILE_VERSION;
    header.history_count = ps->history_count;
    header.free_count    = ps->free_spots.top + 1;
    header.queue_count   = ps->wait_queue.count;
    header.total_revenue = ps->total_revenue;
    header.fee_rule      = ps->fee_rule;
    fwrite(&header, sizeof(FileHeader), 1, fp);

    /* 2. Write all parking spots (3D array) */
    fwrite(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);

    /* 3. Write free spot stack coordinates */
    fwrite(ps->free_spots.spots, sizeof(SpotLocation),
           ps->free_spots.top + 1, fp);

    /* 4. Write waiting queue (linked list → serialized nodes) */
    QueueNode *p = ps->wait_queue.front;
    while (p != NULL) {
        fwrite(p, sizeof(QueueNode), 1, fp);
        p = p->next;
    }

    /* 5. Write transaction history */
    fwrite(ps->history, sizeof(TransactionRecord),
           ps->history_count, fp);

    fclose(fp);
    return 1;
}

int load_data(ParkingSystem *ps, const char *filename) {
    if (ps == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return 0;  /* File not found is OK — use fresh state */

    /* 1. Read file header */
    FileHeader header;
    if (fread(&header, sizeof(FileHeader), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* Validate magic number */
    if (header.magic != FILE_MAGIC) {
        printf("文件格式错误！\n");
        fclose(fp);
        return 0;
    }

    /* 2. Read parking spot states */
    fread(ps->spots, sizeof(ParkingSpot), FLOORS * ROWS * COLS, fp);

    /* 3. Restore free spot stack */
    stack_init(&ps->free_spots, TOTAL_SPOTS);
    ps->free_spots.top = header.free_count - 1;
    if (header.free_count > 0)
        fread(ps->free_spots.spots, sizeof(SpotLocation),
              header.free_count, fp);

    /* 4. Restore waiting queue (rebuild linked list from serialized nodes) */
    queue_init(&ps->wait_queue);
    for (int i = 0; i < header.queue_count; i++) {
        QueueNode node;
        if (fread(&node, sizeof(QueueNode), 1, fp) != 1) break;
        queue_enqueue(&ps->wait_queue, node.plate, node.arrival_time);
    }

    /* 5. Restore transaction history */
    ps->history_count = header.history_count;
    ps->history_capacity = header.history_count + 100;
    ps->history = (TransactionRecord *)malloc(
        sizeof(TransactionRecord) * ps->history_capacity);
    if (ps->history == NULL) {
        printf("内存分配失败！\n");
        fclose(fp);
        return 0;
    }
    if (header.history_count > 0)
        fread(ps->history, sizeof(TransactionRecord),
              header.history_count, fp);

    /* 6. Restore revenue and fee rule */
    ps->total_revenue = header.total_revenue;
    ps->fee_rule = header.fee_rule;

    fclose(fp);
    printf("数据已从 %s 加载\n", filename);
    return 1;
}

void rebuild_hash_from_spots(ParkingSystem *ps) {
    if (ps == NULL) return;

    ht_clear(&ps->plate_index);
    ht_init(&ps->plate_index);

    for (int f = 0; f < FLOORS; f++)
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (ps->spots[f][r][c].status == SPOT_OCCUPIED) {
                    VehicleRecord *rec = (VehicleRecord *)malloc(sizeof(VehicleRecord));
                    if (rec == NULL) {
                        printf("内存分配失败！\n");
                        return;
                    }
                    strcpy(rec->plate, ps->spots[f][r][c].plate);
                    rec->loc = make_loc(f, r, c);
                    rec->entry_time = ps->spots[f][r][c].entry_time;
                    ht_insert(&ps->plate_index, rec);
                }
}

/* ═══════════════════════════════════════════
   用户数据持久化
   ═══════════════════════════════════════════ */

int save_users(UserDatabase *db, const char *filename) {
    if (db == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("无法保存用户数据到 %s\n", filename);
        return 0;
    }

    if (fwrite(&db->count, sizeof(int), 1, fp) != 1 ||
        (db->count > 0 && fwrite(db->users, sizeof(User), db->count, fp) != (size_t)db->count)) {
        printf("写入用户数据失败！\n");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

int load_users(UserDatabase *db, const char *filename) {
    if (db == NULL || filename == NULL) return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return 0;  /* 首次运行，文件不存在 */

    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        printf("读取用户文件头失败！\n");
        fclose(fp); return 0;
    }

    if (count < 0 || count > 10000) {
        printf("用户数据损坏（异常数量: %d）！\n", count);
        fclose(fp); return 0;
    }

    while (db->capacity < count) {
        int new_cap = db->capacity > 0 ? db->capacity * 2 : 8;
        User *new_users = (User *)realloc(db->users, sizeof(User) * new_cap);
        if (new_users == NULL) { fclose(fp); return 0; }
        db->users = new_users;
        db->capacity = new_cap;
    }

    size_t read_count = fread(db->users, sizeof(User), count, fp);
    if (read_count != (size_t)count) {
        printf("读取用户数据不完整 (期望 %d, 实际 %zu)！\n", count, read_count);
        fclose(fp); return 0;
    }
    db->count = count;

    fclose(fp);
    printf("已加载 %d 个用户账号\n", count);
    return 1;
}
