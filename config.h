#ifndef __CONFIG_H
#define __CONFIG_H

struct __network_info_common {
	uint8_t mac[6];
	uint8_t local_ip[4];
	uint8_t gateway[4];
	uint8_t subnet[4];
} __attribute__((packed));

struct __network_info {
	uint8_t working_mode;
	uint8_t state;	// 소켓의 상태 TCP의 경우 Not Connected, Connected, UDP의 경우 UDP
	uint8_t remote_ip[4];			// Must Be 4byte Alignment
	uint16_t local_port;
	uint16_t remote_port;

	uint16_t inactivity;
	uint16_t reconnection;

	uint16_t packing_time;	// 0~65535
	uint8_t packing_size;		// 0~255
	uint8_t packing_delimiter[4];
	uint8_t packing_delimiter_length;	// 0~4
	uint8_t packing_data_appendix;	// 0~2(구분자까지 전송, 구분자 +1바이트 까지 전송, 구분자 +2바이트 까지 전송)
} __attribute__((packed));

struct __serial_info {
	uint32_t baud_rate;	// 각 Baud Rate별로 코드 부여?
	uint8_t data_bits;	// 7, 8, 9 and more?
	uint8_t parity;			// None, odd, even
	uint8_t stop_bits;	// 1, 1.5, 2
	uint8_t flow_control;	// None, RTS/CTS, XON/XOFF, DST/DTR, RTS Only for RS422/485
} __attribute__((packed));

struct __options {
	char pw_setting[10];
	//char pw_connect[10];

	uint8_t dhcp_use;

	uint8_t dns_use;
	uint8_t dns_server_ip[4];
	char dns_domain_name[50];

	//uint8_t serial_command;			// Serial Command Mode 사용 여부
	//uint8_t serial_trigger[3];		// Serial Command Mode 진입을 위한 Trigger 코드
} __attribute__((packed));

struct __io_info {
    uint8_t pin[8];
    uint8_t protocol[8];
    uint32_t start_channel[8];
    uint32_t channel_count[8];
} __attribute__((packed));

struct __proto_info {
    uint8_t protocol;                   // 0 = ZPP, 1=E1.31
    uint16_t universe;                  // starting DMX Universe 0-65535
    uint16_t usize;                     // Universe size
} __attribute__((packed));

typedef struct __CFG_Packet {
    char magic[4];      // magic string "PiPx"
	uint16_t packet_size;
	uint8_t module_type[3]; 
	uint8_t module_name[25];
	uint8_t fw_ver[3];			//  Major Version . Minor Version . Maintenance Version 
	struct __network_info_common network_info_common;
	//struct __network_info network_info[0];
	struct __serial_info serial_info[2];
	struct __options options;
    struct __io_info io;
    struct __proto_info proto;
} __attribute__((packed)) CFG_Packet;

CFG_Packet* get_CFG_Packet_pointer();
void set_CFG_Packet_to_factory_value();
int configInit();
int openPlaylist();
int getNextPlayitem(char *seqFileName,char *seqMediaName,int *seqDelay);
int closePlaylist();

#endif