#include "lora_helper.h"
#if defined(TARGET_XDOT_L151CC)
#include "xdot_low_power.h"
#endif

void update_network_link_check_config(uint8_t link_check_count, uint8_t link_check_threshold) {
    uint8_t current_link_check_count = dot->getLinkCheckCount();
    uint8_t current_link_check_threshold = dot->getLinkCheckThreshold();

    if (current_link_check_count != link_check_count) {
	    logInfo("changing link check count from %u to %u", current_link_check_count, link_check_count);
	    if (dot->setLinkCheckCount(link_check_count) != mDot::MDOT_OK) {
	        logError("failed to set link check count to %u", link_check_count);
	    }
    }

    if (current_link_check_threshold != link_check_threshold) {
	    logInfo("changing link check threshold from %u to %u", current_link_check_threshold, link_check_threshold);
	    if (dot->setLinkCheckThreshold(link_check_threshold) != mDot::MDOT_OK) {
	        logError("failed to set link check threshold to %u", link_check_threshold);
	    }
    }
}

void update_config_name_phrase(std::string network_name, std::string network_passphrase, uint8_t frequency_sub_band, lora::NetworkType network_type, uint8_t ack) {
    std::string current_network_name = dot->getNetworkName();
    std::string current_network_passphrase = dot->getNetworkPassphrase();
    uint8_t current_frequency_sub_band = dot->getFrequencySubBand();
    uint8_t current_network_type = dot->getPublicNetwork();
    uint8_t current_ack = dot->getAck();
    
    if (current_network_name != network_name) {
        logInfo("changing network name from \"%s\" to \"%s\"", current_network_name.c_str(), network_name.c_str());
        if (dot->setNetworkName(network_name) != mDot::MDOT_OK) {
            logError("failed to set network name to \"%s\"", network_name.c_str());
        }
    }
    
    if (current_network_passphrase != network_passphrase) {
        logInfo("changing network passphrase from \"%s\" to \"%s\"", current_network_passphrase.c_str(), network_passphrase.c_str());
        if (dot->setNetworkPassphrase(network_passphrase) != mDot::MDOT_OK) {
            logError("failed to set network passphrase to \"%s\"", network_passphrase.c_str());
        }
    }
    
    if (lora::ChannelPlan::IsPlanFixed(dot->getFrequencyBand())) {
	    if (current_frequency_sub_band != frequency_sub_band) {
	        logInfo("changing frequency sub band from %u to %u", current_frequency_sub_band, frequency_sub_band);
	        if (dot->setFrequencySubBand(frequency_sub_band) != mDot::MDOT_OK) {
		        logError("failed to set frequency sub band to %u", frequency_sub_band);
	        }
	    }
    }

    if (current_network_type != network_type) {
        if (dot->setPublicNetwork(network_type) != mDot::MDOT_OK) {
            logError("failed to set network type");
        }
    }

    if (current_ack != ack) {
        logInfo("changing acks from %u to %u", current_ack, ack);
        if (dot->setAck(ack) != mDot::MDOT_OK) {
            logError("failed to set acks to %u", ack);
        }
    }
}

void sleep_wake_rtc_or_interrupt(bool deepsleep) {
    // in some frequency bands we need to wait until another channel is available before transmitting again
    // wait at least 10s between transmissions
    uint32_t delay_s = dot->getNextTxMs() / 1000;
    if (delay_s < 10) {
        delay_s = 10;
    }

#if defined (TARGET_XDOT_L151CC)
    if (deepsleep) {
        // for xDot, WAKE pin (connected to S2 on xDot-DK) is the only pin that can wake the processor from deepsleep
        // it is automatically configured when INTERRUPT or RTC_ALARM_OR_INTERRUPT is the wakeup source and deepsleep is true in the mDot::sleep call
    } else {
        // configure WAKE pin (connected to S2 on xDot-DK) as the pin that will wake the xDot from low power modes
        //      other pins can be confgured instead: GPIO0-3 or UART_RX
        dot->setWakePin(WAKE);    
    }

    logInfo("%ssleeping %lus or until interrupt on %s pin", deepsleep ? "deep" : "", delay_s, deepsleep ? "WAKE" : mDot::pinName2Str(dot->getWakePin()).c_str());
#else
    if (deepsleep) {
        // for mDot, XBEE_DIO7 pin is the only pin that can wake the processor from deepsleep
        // it is automatically configured when INTERRUPT or RTC_ALARM_OR_INTERRUPT is the wakeup source and deepsleep is true in the mDot::sleep call
    } else {
        // configure XBEE_DIO7 pin as the pin that will wake the mDot from low power modes
        //      other pins can be confgured instead: XBEE_DIO2-6, XBEE_DI8, XBEE_DIN
        dot->setWakePin(XBEE_DIO7);    
    }

    logInfo("%ssleeping %lus or until interrupt on %s pin", deepsleep ? "deep" : "", delay_s, deepsleep ? "DIO7" : mDot::pinName2Str(dot->getWakePin()).c_str());
#endif

    logInfo("application will %s after waking up", deepsleep ? "execute from beginning" : "resume");

    // lowest current consumption in sleep mode can only be achieved by configuring IOs as analog inputs with no pull resistors
    // the library handles all internal IOs automatically, but the external IOs are the application's responsibility
    // certain IOs may require internal pullup or pulldown resistors because leaving them floating would cause extra current consumption
    // for xDot: UART_*, I2C_*, SPI_*, GPIO*, WAKE
    // for mDot: XBEE_*, USBTX, USBRX, PB_0, PB_1
    // steps are:
    //   * save IO configuration
    //   * configure IOs to reduce current consumption
    //   * sleep
    //   * restore IO configuration
    if (! deepsleep) {
	// save the GPIO state.
	    sleep_save_io();

	    // configure GPIOs for lowest current
	    sleep_configure_io();
    }
    
    // go to sleep/deepsleep and wake using the RTC alarm after delay_s seconds or rising edge of configured wake pin (only the WAKE pin in deepsleep)
    // whichever comes first will wake the xDot
    dot->sleep(delay_s, mDot::RTC_ALARM_OR_INTERRUPT, deepsleep);

    if (! deepsleep) {
	    // restore the GPIO state.
	    sleep_restore_io();
    }
}

void sleep_save_io() {
#if defined(TARGET_XDOT_L151CC)
	xdot_save_gpio_state();
#else
	portA[0] = GPIOA->MODER;
	portA[1] = GPIOA->OTYPER;
	portA[2] = GPIOA->OSPEEDR;
	portA[3] = GPIOA->PUPDR;
	portA[4] = GPIOA->AFR[0];
	portA[5] = GPIOA->AFR[1];

	portB[0] = GPIOB->MODER;
	portB[1] = GPIOB->OTYPER;
	portB[2] = GPIOB->OSPEEDR;
	portB[3] = GPIOB->PUPDR;
	portB[4] = GPIOB->AFR[0];
	portB[5] = GPIOB->AFR[1];

	portC[0] = GPIOC->MODER;
	portC[1] = GPIOC->OTYPER;
	portC[2] = GPIOC->OSPEEDR;
	portC[3] = GPIOC->PUPDR;
	portC[4] = GPIOC->AFR[0];
	portC[5] = GPIOC->AFR[1];

	portD[0] = GPIOD->MODER;
	portD[1] = GPIOD->OTYPER;
	portD[2] = GPIOD->OSPEEDR;
	portD[3] = GPIOD->PUPDR;
	portD[4] = GPIOD->AFR[0];
	portD[5] = GPIOD->AFR[1];

	portH[0] = GPIOH->MODER;
	portH[1] = GPIOH->OTYPER;
	portH[2] = GPIOH->OSPEEDR;
	portH[3] = GPIOH->PUPDR;
	portH[4] = GPIOH->AFR[0];
	portH[5] = GPIOH->AFR[1];
#endif
}

void sleep_configure_io() {
#if defined(TARGET_XDOT_L151CC)
    // GPIO Ports Clock Enable
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOH_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    // UART1_TX, UART1_RTS & UART1_CTS to analog nopull - RX could be a wakeup source
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // I2C_SDA & I2C_SCL to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SPI_MOSI, SPI_MISO, SPI_SCK, & SPI_NSS to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // iterate through potential wake pins - leave the configured wake pin alone if one is needed
    if (dot->getWakePin() != WAKE || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO0 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO1 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO2 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO3 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != UART1_RX || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
#else
    /* GPIO Ports Clock Enable */
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    // XBEE_DOUT, XBEE_DIN, XBEE_DO8, XBEE_RSSI, USBTX, USBRX, PA_12, PA_13, PA_14 & PA_15 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 
                | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);    

    // PB_0, PB_1, PB_3 & PB_4 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); 

    // PC_9 & PC_13 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct); 

    // iterate through potential wake pins - leave the configured wake pin alone if one is needed
    // XBEE_DIN - PA3
    // XBEE_DIO2 - PA5
    // XBEE_DIO3 - PA4
    // XBEE_DIO4 - PA7
    // XBEE_DIO5 - PC1
    // XBEE_DIO6 - PA1
    // XBEE_DIO7 - PA0
    // XBEE_SLEEPRQ - PA11
                
    if (dot->getWakePin() != XBEE_DIN || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (dot->getWakePin() != XBEE_DIO2 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (dot->getWakePin() != XBEE_DIO3 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

         if (dot->getWakePin() != XBEE_DIO4 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO5 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO6 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO7 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_SLEEPRQ|| dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
#endif
}

void sleep_restore_io() {
#if defined(TARGET_XDOT_L151CC)
    xdot_restore_gpio_state();
#else
    GPIOA->MODER = portA[0];
    GPIOA->OTYPER = portA[1];
    GPIOA->OSPEEDR = portA[2];
    GPIOA->PUPDR = portA[3];
    GPIOA->AFR[0] = portA[4];
    GPIOA->AFR[1] = portA[5];

    GPIOB->MODER = portB[0];
    GPIOB->OTYPER = portB[1];
    GPIOB->OSPEEDR = portB[2];
    GPIOB->PUPDR = portB[3];
    GPIOB->AFR[0] = portB[4];
    GPIOB->AFR[1] = portB[5];

    GPIOC->MODER = portC[0];
    GPIOC->OTYPER = portC[1];
    GPIOC->OSPEEDR = portC[2];
    GPIOC->PUPDR = portC[3];
    GPIOC->AFR[0] = portC[4];
    GPIOC->AFR[1] = portC[5];

    GPIOD->MODER = portD[0];
    GPIOD->OTYPER = portD[1];
    GPIOD->OSPEEDR = portD[2];
    GPIOD->PUPDR = portD[3];
    GPIOD->AFR[0] = portD[4];
    GPIOD->AFR[1] = portD[5];

    GPIOH->MODER = portH[0];
    GPIOH->OTYPER = portH[1];
    GPIOH->OSPEEDR = portH[2];
    GPIOH->PUPDR = portH[3];
    GPIOH->AFR[0] = portH[4];
    GPIOH->AFR[1] = portH[5];
#endif
}

void join_network() {
    int32_t j_attempts = 0;
    int32_t ret = mDot::MDOT_ERROR;
    
    // attempt to join the network
    while (ret != mDot::MDOT_OK) {
        logInfo("attempt %d to join network", ++j_attempts);
        ret = dot->joinNetwork();
        if (ret != mDot::MDOT_OK) {
            logError("failed to join network %d:%s", ret, mDot::getReturnCodeString(ret).c_str());
            // in some frequency bands we need to wait until another channel is available before transmitting again
            uint32_t delay_s = (dot->getNextTxMs() / 1000) + 1;
            if (delay_s < 5) {
                logInfo("waiting %lu s until next free channel", delay_s);
                ThisThread::sleep_for(std::chrono::seconds(delay_s));
            } else {
                logInfo("sleeping %lu s until next free channel", delay_s);
                dot->sleep(delay_s, mDot::RTC_ALARM, false);
            }
        }
    }
}

int send_data(std::vector<uint8_t> data) {
    int32_t ret;

    ret = dot->send(data);
    if (ret != mDot::MDOT_OK) {
        logError("failed to send data to %s [%d][%s]", 
            dot->getJoinMode() == mDot::PEER_TO_PEER ? "peer" : "gateway", 
            ret, mDot::getReturnCodeString(ret).c_str());
    } else {
        logInfo("successfully sent data to %s", 
            dot->getJoinMode() == mDot::PEER_TO_PEER ? "peer" : "gateway");
    }

    return ret;
}