#include <string.h>
#include <stdio.h>

#include "PicoPlayer.h"
#include "config.h"
#include "ff.h" /* Obtains integer types */
#include "diskio.h" /* Declarations of disk functions */
#include "f_util.h"

static CFG_Packet cfg_packet;
static FIL playlistFile;


CFG_Packet* get_CFG_Packet_pointer()
{
    if(strncmp((const char *)&cfg_packet.magic,"PiPl",4))
    {
        set_CFG_Packet_to_factory_value();
    }
	return &cfg_packet;
}

#define MAJOR_VER 1
#define MINOR_VER 0
#define MAINTENANCE_VER 0

void set_CFG_Packet_to_factory_value()
{
	memcpy(cfg_packet.magic, "PiPl",4);  // set the magic string
	cfg_packet.packet_size = sizeof(CFG_Packet);	// 133
	cfg_packet.module_type[0] = 0x01;
	cfg_packet.module_type[1] = 0x02;
	cfg_packet.module_type[2] = 0x00;
	memcpy(cfg_packet.module_name, "PicoPixel\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 25);
	cfg_packet.fw_ver[0] = MAJOR_VER;
	cfg_packet.fw_ver[1] = MINOR_VER;
	cfg_packet.fw_ver[2] = MAINTENANCE_VER;

	cfg_packet.network_info_common.local_ip[0] = 192;
	cfg_packet.network_info_common.local_ip[1] = 168;
	cfg_packet.network_info_common.local_ip[2] = 11;
	cfg_packet.network_info_common.local_ip[3] = 100;
	cfg_packet.network_info_common.gateway[0] = 192;
	cfg_packet.network_info_common.gateway[1] = 168;
	cfg_packet.network_info_common.gateway[2] = 11;
	cfg_packet.network_info_common.gateway[3] = 1;
	cfg_packet.network_info_common.subnet[0] = 255;
	cfg_packet.network_info_common.subnet[1] = 255;
	cfg_packet.network_info_common.subnet[2] = 255;
	cfg_packet.network_info_common.subnet[3] = 0;

	cfg_packet.io.protocol[0] = 0; // set all ports to unused
	cfg_packet.io.protocol[1] = 0;
	cfg_packet.io.protocol[2] = 0;
	cfg_packet.io.protocol[3] = 0;
	cfg_packet.io.protocol[4] = 0;
	cfg_packet.io.protocol[5] = 0;
	cfg_packet.io.protocol[6] = 0;
	cfg_packet.io.protocol[7] = 0;

	cfg_packet.proto.protocol = 0; //default to zcpp

	memcpy(cfg_packet.options.pw_setting, "PicoPlayer\0", 10);
	cfg_packet.options.dhcp_use = 1;
	cfg_packet.options.dns_use = 1;
	cfg_packet.options.dns_server_ip[0] = 8;
	cfg_packet.options.dns_server_ip[1] = 8;
	cfg_packet.options.dns_server_ip[2] = 8;
	cfg_packet.options.dns_server_ip[3] = 8;
	memset(cfg_packet.options.dns_domain_name, '\0', 50);
}

int configInit()
{
    //
    // Need to read config file and set io mode
    // 

    CFG_Packet *dev_config = get_CFG_Packet_pointer();
    thread_ctrl *run_cfg = get_running_config();

	run_cfg->run_mode = MODE_STOPPED;
	char cfgfilename[] = "hwconfig.txt\0";
	FIL configFile;
 	FRESULT fr;
    fr = f_open(&configFile, cfgfilename, FA_READ);
    if (FR_OK != fr) {
        printf("Config file %s open error: %s (%d)\n", cfgfilename,FRESULT_str(fr), fr);
        return -1;
    }
    char buf[256];
	char delim[] = ",";
	uint8_t gpio;
	char protocol[16];
	uint8_t protocolNum;
	uint16_t startaddress;
	uint16_t channelcount;
	uint8_t pnum = 0;


    while((f_gets(buf,sizeof buf, &configFile)) && (pnum < 8)){
        if(buf[0] != '#'){
			char *ptr = strtok(buf,delim);
			if(ptr!=NULL){
				gpio = atoi(ptr);
				ptr = strtok(NULL,delim);
				if(ptr!=NULL){
					strncpy(protocol,ptr,15);
					ptr = strtok(NULL,delim);
					if(ptr!=NULL){
						startaddress = atoi(ptr);
						ptr = strtok(NULL,delim);
						if(ptr!=NULL){
							channelcount = atoi(ptr);
							if(strcasecmp(protocol,"WS2811")==0){protocolNum=1;}
							else if(strcasecmp(protocol,"DMX")==0){protocolNum=8;}
							else {
								printf("Invalid protocol %s\n",protocol);
								protocolNum=0;
							}
							if (protocolNum){
								printf("Configuring GPIO: %d Protocol: %s  Start Address: %d  Channels: %d\n",gpio,protocol,startaddress,channelcount);
							    dev_config->io.protocol[pnum] = protocolNum; 
    							dev_config->io.pin[pnum] = gpio; 
    							run_cfg->led_string[pnum].active[0] = 1;
    							run_cfg->led_string[pnum].channel_count[0] = channelcount;
    							run_cfg->led_string[pnum].start_channel[0] = startaddress;
								pnum++;
							}
						}
					}
				}
			}
		}
	}
    fr = f_close(&configFile);
    if (FR_OK != fr) printf("Config file %s close error: %s (%d)\n", cfgfilename,FRESULT_str(fr), fr);

/*
    dev_config->io.protocol[0] = 1; //WS2812
    dev_config->io.pin[0] = 0; // GPIO 0
    run_cfg->led_string[0].active[0] = 1;
    run_cfg->led_string[0].channel_count[0] = 150;
    run_cfg->led_string[0].start_channel[0] = 3151;

    dev_config->io.protocol[1] = 1; //WS2812
    dev_config->io.pin[1] = 1; // GPIO 1
    run_cfg->led_string[1].active[0] = 1;
    run_cfg->led_string[1].channel_count[0] = 144;
    run_cfg->led_string[1].start_channel[0] = 3301;

    dev_config->io.protocol[2] = 1; //WS2812
    dev_config->io.pin[2] = 2; // GPIO 2
    run_cfg->led_string[2].active[0] = 1;
    run_cfg->led_string[2].channel_count[0] = 150;
    run_cfg->led_string[2].start_channel[0] = 3445;
*/
}

int getNextPlayitem(char *seqFileName,char *seqMediaName,int *seqDelay){
    char buf[256];
	char delim[] = ",";
    while(f_gets(buf,sizeof buf, &playlistFile)){
        if(buf[0] != '#'){
			char *ptr = strtok(buf,delim);
			if(ptr!=NULL){
				strcpy(seqFileName,ptr);
				ptr = strtok(NULL,delim);
				if(ptr!=NULL){
					strcpy(seqMediaName,ptr);
					ptr = strtok(NULL,delim);
					if(ptr!=NULL){
						*seqDelay = atoi(ptr);
					}
				}
			}
		printf("Sequence: %s  Audio: %s  Delay: %d\n", seqFileName,seqMediaName,*seqDelay);
		return 0;
		}
    }
	f_rewind(&playlistFile);
	return getNextPlayitem(seqFileName,seqMediaName,seqDelay); //re-entrant call will probably hang if there 
																// is a problem with the playlist file
}

int openPlaylist(){
    // Read the playlist

	FRESULT fr;
    fr = f_open(&playlistFile, "playlist.txt", FA_READ);
    if (FR_OK != fr) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return -1;
    }
}

int closePlaylist(){
    FRESULT fr = f_close(&playlistFile);
    if (FR_OK != fr) printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);

}