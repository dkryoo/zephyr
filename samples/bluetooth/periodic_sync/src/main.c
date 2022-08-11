/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <bluetooth/bluetooth.h>
#include <kernel.h> //DK: timer!

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN            30

static bool         per_adv_found;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
//DK timer start
int count=0;
uint16_t T_packet;
uint16_t packet_num=6000;
uint16_t count_test=0;
#define TRUE 1
#define FALSE 0

#ifdef SYSTEM
info_dk pattern_scan[num_evt];
#define PDU_LEN_DK 856U
#define SLOT_TIME PDU_LEN_DK+300U
#endif

void my_work_handler(struct k_work *work)
{
  printk("1 minute count(TIMEOUT): %d, LAST PACKET NUMBER: %u\n", count, T_packet);
  count=0;
//	printk("COUNT TEST for 10 seconds: %u\n", count_test);
//	count_test=0;
}
K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
//DK TIMER END
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

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	/*
	if(info->adv_type == 5){
	printk("DK [DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
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
	}
	*/
	if (!per_adv_found && info->interval) {
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

	printk("DK PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("DK PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);
//	printk("THE REASON: %u\n", info->reason);
	k_sem_give(&sem_per_sync_lost);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
//	char data_str[256];
	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
/*
	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u, data: %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len, data_str);
		   */
	printk("DATA[0]: %u , DATA[1]: %u, length: %u\n", buf->data[2], buf->data[3],buf->len);
	T_packet=256U*buf->data[3]+buf->data[2];
	count=count+1;
	if(T_packet>packet_num-1){
		printk("PACKET OVER %u, Count: %d, TOTAL PACKET: %u", packet_num,count,T_packet);
	}
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb
};

void main(void)
{

	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	int err;

	#ifdef SYSTEM
	for(int j=0;j<num_evt;j++){
		pattern_scan[j].offs=1500U; 
		pattern_scan[j].chan_idx=3;
	}
	#endif

	//FOR RESET
	printk("Starting Periodic Advertising Synchronization Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	printk("Success.\n");

	printk("Start scanning...");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success.\n");
	k_timer_start(&my_timer, K_SECONDS(60), K_SECONDS(60));//DK Timer start
	printk("receive enabled.\n");

	do {
		printk("Waiting for periodic advertising...\n");
		per_adv_found = false;
		err = k_sem_take(&sem_per_adv, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			break;
		}
		printk("Creating Periodic Advertising Sync...");
		bt_addr_le_copy(&sync_create_param.addr, &per_addr);
		sync_create_param.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
		sync_create_param.sid = per_sid;
		sync_create_param.skip = 0;
		sync_create_param.timeout = 0xa;
		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err) {
			printk("failed (err %d)\n", err);
			break;
		}
		printk("Waiting for periodic sync...\n");
		err = k_sem_take(&sem_per_sync, TIMEOUT_SYNC_CREATE);
		if (err) {
			printk("failed (err %d)\n", err);

			printk("Deleting Periodic Advertising Sync...");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("failed (err %d)\n", err);
				break;
			}
			continue;
		}

		printk("Periodic sync established.\n");
		err = k_sem_take(&sem_per_sync_lost, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			break;
		}
		printk("Periodic sync lost.\n");
	} while (true);
	return;
}
