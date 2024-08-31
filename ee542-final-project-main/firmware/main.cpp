#include "TinyGPSPlus.h"
#include "lora_helper.h"
#include "mbed.h"
#include "rtos.h"

/* Network Credentials of LoRa Gateway */
static std::string network_name = "MTCDT-20314492";
static std::string network_passphrase = "20314492";

/* Unique xdot identifier */
#define XDOT_DEVICE_ID "device-01"

/* Neov6 GPS sensor default baud rate */
#define GPS_UART_BAUD_RATE 9600

/* Min interval between consecutive messages to gateway */
#define SEND_TIMEOUT       10

static bool deep_sleep = false;
mDot *dot = NULL;

class Coordinates {
public:
  long datetime;
  double latitude;
  double longitude;
  double speed;

  std::string device_id;

  vector<uint8_t> to_buf() {
    char data[128] = {0};
    snprintf(data, sizeof(data), "%s,%lu,%lf,%lf,%lf", device_id.c_str(), datetime, latitude, longitude, speed);

    //vector<uint8_t> buffer(strlen(data));
    vector<uint8_t> buffer;
    

    for (int i = 0; i < strlen(data); i++) {
      buffer.push_back(data[i]);
    }

    return buffer;
  }
};

void get_coordinates(void) {
  TinyGPSPlus gps_parser;
  BufferedSerial gps(UART1_TX, UART1_RX);
  DigitalOut led1(LED1);
  char buf[128];

  gps.set_baud(GPS_UART_BAUD_RATE);
  gps.set_format(8, BufferedSerial::None, 1);

  //time_t last_sent = 0;

  while (1) {
    char c;
    if (uint32_t num = gps.read(&c, sizeof(char))) {
      // Toggle the LED.
      led1 = !led1;
      switch (c) {
      case '\n':
        if (gps_parser.satellites.isValid() &&
            gps_parser.satellites.value() > 3 && gps_parser.hdop.hdop() > 0) {
          snprintf(buf, 128, "{\"Latitude\":%lf,\"Longitude\":%lf}",
                   gps_parser.location.lat(), gps_parser.location.lng());

          // /* Push coordinates to the queue */
          Coordinates data_to_upload;
          data_to_upload.datetime = time(NULL);
          data_to_upload.device_id = XDOT_DEVICE_ID;
          data_to_upload.latitude = gps_parser.location.lat();
          data_to_upload.longitude = gps_parser.location.lng();
          data_to_upload.speed = gps_parser.speed.mph();

          if (!dot->getNetworkJoinStatus()) {
            join_network();
          }

          /* Send data via Lora to Gateway */
          send_data(data_to_upload.to_buf());

          if (deep_sleep) {
            logInfo("saving network session to NVM");
            dot->saveNetworkSession();
          }

          //sleep_wake_rtc_or_interrupt(deep_sleep);

        } else {
          snprintf(buf, 128,
                   "Satellites: %u, time: %04d-%02d-%02dT%02d:%02d:%02d.%02d",
                   gps_parser.satellites.value(), gps_parser.date.year(),
                   gps_parser.date.month(), gps_parser.date.day(),
                   gps_parser.time.hour(), gps_parser.time.minute(),
                   gps_parser.time.second(), gps_parser.time.centisecond());
        }

        printf("%s\r\n", buf);
        break;
      default:
        gps_parser.encode(c);
        break;
      }
    }
  }
}

void setup_lora_connection(void) {

  lora::ChannelPlan *plan = NULL;
  static uint8_t join_delay = 5;
  static uint8_t ack = 0;
  static bool adr = true;
  static uint8_t frequency_sub_band = 7;
  //mbed::UnbufferedSerial pc(USBTX, USBRX);
  static lora::NetworkType network_type = lora::PUBLIC_LORAWAN;

  //pc.baud(LORA_BAUD_RATE);
  mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

  // Create channel plan
  plan = new lora::ChannelPlan_US915();
  assert(plan);

  dot = mDot::getInstance(plan);
  assert(dot);

  // Enable FOTA for multicast support
//   Fota::getInstance(dot);

  if (!dot->getStandbyFlag() && !dot->getPreserveSession()) {
    logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION,
            MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    // start from a well-known state
    logInfo("defaulting Dot configuration");
    dot->resetConfig();
    dot->resetNetworkSession();

    // make sure library logging is turned on
    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

    // update configuration if necessary
    if (dot->getJoinMode() != mDot::OTA) {
      logInfo("changing network join mode to OTA");
      if (dot->setJoinMode(mDot::OTA) != mDot::MDOT_OK) {
        logError("failed to set network join mode to OTA");
      }
    }

    // To preserve session over power-off or reset enable this flag
    // dot->setPreserveSession(true);

    update_config_name_phrase(network_name, network_passphrase,
                              frequency_sub_band, network_type, ack);

    // configure network link checks
    // network link checks are a good alternative to requiring the gateway to
    // ACK every packet and should allow a single gateway to handle more Dots
    // check the link every count packets
    // declare the Dot disconnected after threshold failed link checks
    // for count = 3 and threshold = 5, the Dot will ask for a link check
    // response every 5 packets and will consider the connection lost if it
    // fails to receive 3 responses in a row
    update_network_link_check_config(3, 5);

    // enable or disable Adaptive Data Rate
    dot->setAdr(adr);

    // Configure the join delay
    dot->setJoinDelay(join_delay);

    // save changes to configuration
    logInfo("saving configuration");
    if (!dot->saveConfig()) {
      logError("failed to save configuration");
    }

    // display configuration
    // display_config();
  } else {
    // restore the saved session if the dot woke from deepsleep mode
    // useful to use with deepsleep because session info is otherwise lost when
    // the dot enters deepsleep
    logInfo("restoring network session from NVM");
    dot->restoreNetworkSession();
  }
}

// void lora_handler(void) {

//     // Setup credentials and parameters to connect to lora gateway
//     //setup_lora_connection();

//     while (1) {
//         //Coordinates *data_to_upload = mail_box.try_get();

//         if (data_to_upload) {

//             // if lora is sleeping, wake it up
//             if (!dot->getNetworkJoinStatus()) {
//                 join_network();
//             }

//             /* Send data via Lora to Gateway */
//             send_data(data_to_upload->to_buf());

//            // mail_box.free(data_to_upload);

//             if (deep_sleep) {
//                 logInfo("saving network session to NVM");
//                 dot->saveNetworkSession();
//             }

//             sleep_wake_rtc_or_interrupt(deep_sleep);
//         }
//     }
// }

int main() {
  setup_lora_connection();

  get_coordinates();

  return 0;
}