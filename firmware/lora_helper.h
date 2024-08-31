#ifndef __LORA_HELPER_H__
#define __LORA_HELPER_H__

#include "mbed.h"
#include "mDot.h"
#include "ChannelPlans.h"
#include "MTSLog.h"
#include "MTSText.h"
//#include "ISL29011.h"
//#include "example_config.h"

extern mDot* dot;

void join_network();

void update_config_name_phrase(std::string network_name, std::string network_passphrase, uint8_t frequency_sub_band, lora::NetworkType network_type, uint8_t ack);

void update_network_link_check_config(uint8_t link_check_count, uint8_t link_check_threshold);

int send_data(std::vector<uint8_t> data);

void sleep_wake_rtc_or_interrupt(bool deepsleep);

void sleep_save_io();

void sleep_configure_io();

void sleep_restore_io();

#endif