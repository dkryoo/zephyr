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
#define HAMMING_BOUND 243
#define PDU_TIME 856U
#define SLOT_TIME PDU_TIME+EVENT_SYNC_B2B_MAFS_US

uint8_t count_test=0;
extern uint8_t count_evt;

//DK PATTERN START
#ifdef SYSTEM
int power(int a, int b){
	int tmp=1;
	for(int i=0;i<b;i++)
	tmp*=a;
	return tmp;
}
info_dk pattern_rep[num_evt][num_rep];
seq_DK *arr_mem;
int HAM[BIT_NUM][num_evt];
info_dk metadata_gen(uint8_t evt, uint8_t rep){//num means number of channelmap 
	info_dk info_dk_tmp;
	info_dk_tmp.offs=1200U-PDU_TIME;
	info_dk_tmp.chan_idx=3;//sys_rand32_get()%37; // aux_ptr + chan_idx
    return info_dk_tmp;
}
void HAM_init(){
	 for(int i=0; i<BIT_NUM;i++){
            for(int j=0;j<num_evt;j++){
                HAM[i][j]=0;
                if(i==j)
                    HAM[i][j]=1;
                if(j==5)
                    HAM[i][j]=1;
                if(j==6){
                    if(i<2)
                    HAM[i][j]=1;
                    else
                    HAM[i][j]=2;
                }
                if(j==7){
                    if(i==0 || i==3)
                        HAM[i][j]=1;
                    else if(i==1 || i==4)
                        HAM[i][j]=2;
                }
            }
        }
}
int hamming_code(int node, int evt){
	int tmp[BIT_NUM];
	 tmp[0]=node/(power(num_rep,4))%num_rep;
     tmp[1]=(node/(power(num_rep,3)))%num_rep;
     tmp[2]=node/(power(num_rep,2))%num_rep;
     tmp[3]=(node/num_rep)%num_rep;
     tmp[4]=node%num_rep;	
	 int aaa=0;
	for(int i=0;i<BIT_NUM;i++){
		aaa+=tmp[i]*HAM[i][evt];
		aaa=aaa%num_rep;
	}
	return aaa;
}
void arr_init(){
	for(int i=0;i<HAMMING_BOUND;i++){
		for(int j=0;j<num_evt;j++)
		arr_mem[i].seq[j]=hamming_code(i,j);
	//	arr_mem[i].bd_addr=;
		arr_mem[i].conn=FALSE;
	}
}
#endif
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
// PATTERN!
	#ifdef SYSTEM
		HAM_init();
		arr_mem=k_malloc(sizeof(seq_DK)*HAMMING_BOUND);
		arr_init();

	for(int i=0; i<num_evt;i++){
		for(int j=0;j<num_rep;j++){
			pattern_rep[i][j]=metadata_gen(i,j);
		}
	}

	#endif

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

		}
//NO COMPILE
	bt_le_per_adv_stop(adv);
}
