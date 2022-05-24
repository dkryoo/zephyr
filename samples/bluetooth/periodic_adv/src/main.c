/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <bluetooth/bluetooth.h>
uint8_t mfg_data[200]; //= { 0xff, 0xff, 0x00 };
extern uint16_t COUNT_DK;
#define TRUE 1
#define FALSE 0
bool jam_f=TRUE;
uint8_t count_test=0;
extern uint8_t count_evt;

//DK PATTERN START

info_dk pattern_rep[num_rep][num_evt];

info_dk pattern_gen(uint8_t rep, uint8_t evt){//num means number of channelmap 
	info_dk info_dk_tmp;
//    info_dk_tmp.offs=sys_rand32_get()&0x000003FF;//EVENT_SYNC_B2B_MAFS_US + offs (us)
		info_dk_tmp.offs=300+100*evt;
//    info_dk_tmp.chan_idx=sys_rand32_get()%37; // aux_ptr + chan_idx
		info_dk_tmp.chan_idx=3*rep+evt;
    return info_dk_tmp;
}

void pattern_init(uint8_t rep, uint8_t evt){
    for(int i=0; i<rep; i++){
        for(int j=0; j<evt; j++){
            pattern_rep[i][j]=pattern_gen(rep, evt);
        }
    }
}

void pattern_reset(uint8_t rep, uint8_t evt){
    for(int i=0; i<rep; i++){
        for(int j=0; j<evt; j++){
            pattern_rep[i][j].offs=0U;
            pattern_rep[i][j].chan_idx=0U;          
        }
    }
}
//DK PATTERN END
/*
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};
*/
/*
static void adv_sent_cb(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_sent_info *info);

static struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = adv_sent_cb,
};
static void adv_sent_cb(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_sent_info *info)
{
	printk("Advertiser[%d] %p sent %d\n", bt_le_ext_adv_get_index(adv),
	       adv, info->num_sent);
}
*/
void main(void)
{
	struct bt_le_ext_adv *adv;
	int err;
	
	for(int i=0; i<sizeof(mfg_data);i++){
    if(i<256)
	mfg_data[i]=i;
	else
	mfg_data[i]=400-i;
	}
	mfg_data[1]=0;
	struct bt_data ad[] = {
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
	};
	printk("Starting Periodic Advertising Demo\n");
	pattern_init(num_rep, num_evt);
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}
	/*
	err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
		if (err) {
			printk("Failed (err %d)\n", err);
			return;
		}
		*/
	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising "
			       "(err %d)\n", err);
			return;
		}

	while (true) {

			k_sleep(K_MSEC(1000));

			mfg_data[0]=(uint8_t)(COUNT_DK & 0x00FF);
			mfg_data[1]=(uint8_t)((COUNT_DK & 0xFF00) >>8);
//			printk("COUNT: %u, DATA: %u, %u \n", COUNT_DK, mfg_data[0], mfg_data[1]);
//			printk("Set Periodic Advertising Data... ");
//			printk("DATA[0] %d, DATA[1] %d, [DATA2] %d\n", mfg_data[0], mfg_data[1],mfg_data[2]);
/* //CHECK DATA!!!
			for(int i=0; i<ad[0].data_len;i++){
				printk("DATA[%d] %u  ", i,ad[0].data[i]);
			}
			printk("\n");
*/
			err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
			if (err) {
				printk("Failed (err %d)\n", err);
				return;
			}
			printk("DATA[0]: %u , DATA[1]: %u \n", mfg_data[0], mfg_data[1]);
			printk("COUNT_evt: %u\n", count_evt);
//			count_test=0;
//			printk("done.\n");
		}
	bt_le_per_adv_stop(adv);
}
