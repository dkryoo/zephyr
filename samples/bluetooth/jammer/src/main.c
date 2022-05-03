/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <bluetooth/bluetooth.h>

//SYNC
#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN            30

static bool         per_adv_found;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;
static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
static struct device const   *dev;
//SYNC end

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};
extern int jam=1;
void main(void)
{
	printk("START JAMMER");
		struct bt_le_per_adv_sync_param sync_create_param;
        struct bt_le_per_adv_sync *sync;
        /* Initialize the Bluetooth Subsystem */
        err = bt_enable(NULL);
        if (err) {
                printk("Bluetooth init failed (err %d)\n", err);
                return;
        }

	printk("START_SCANNING\n");
        bt_le_scan_cb_register(&scan_callbacks);
        bt_le_per_adv_sync_cb_register(&sync_callbacks);
        err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	do{
	printk("Waiting for victim\n");
        per_adv_found = false;
        err = k_sem_take(&sem_per_adv, K_FOREVER);
        if (err) {
               printk("failed (err %d)\n", err);
               return;
        }
        printk("Found periodic advertising.\n");
       
        printk("Creating Periodic Advertising Sync...");
        bt_addr_le_copy(&sync_create_param.addr, &per_addr);
        sync_create_param.options = 0;
        sync_create_param.sid = per_sid;
        sync_create_param.skip = 0;
        sync_create_param.timeout = 0xa;
        err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
        if (err) {
                printk("failed (err %d)\n", err);
                return;
        }
        }while(true);

	struct bt_le_ext_adv *adv;
	int err;

	printk("START_JAMMING\n");
	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}
	/* Set periodic advertising parameters */
/*	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}
*/
	/* Enable Periodic Advertising */
/*	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
*/
	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}
/*
	while (true) {
		k_sleep(K_SECONDS(1));
		//k_sleep(K_MSEC(1000));
		mfg_data[2]++;	
		printk("Set Periodic Advertising Data: %d \n",mfg_data[2]);
		err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
		if (err) {
			printk("Failed (err %d)\n", err);
			return;
		}
		//printk("done.\n");

	}*/
}
