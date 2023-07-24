#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "ws2812.pio.h"
#include "PicoPlayer.h"
#include "config.h"

#include "outputManager.h"

#define DEF_TRANSFER_COUNT 0

static thread_ctrl *config;


PIO pio_map[8] = {pio0,pio1,pio0,pio1,pio0,pio1,pio0,pio1};
int pio_num_map[8] = {0,1,0,1,0,1,0,1};
int sm_map[8] = {0,0,1,1,2,2,3,3};
uint16_t dreq_map[8] = {DREQ_PIO0_TX0,DREQ_PIO1_TX0,DREQ_PIO0_TX1,DREQ_PIO1_TX1,
    DREQ_PIO0_TX2,DREQ_PIO1_TX2,DREQ_PIO0_TX3,DREQ_PIO1_TX3};

uint offset_map[2][16];  // offset_map[pio][protocol]
int dma_chan[8];
dma_channel_config chan;
int wrk_chan;
PIO pio;
int sm;
int port_id;
int pio_num;
dma_channel_config c;


void outputInit(thread_ctrl *worker)
{
    for(int a = 0;a<PIXEL_PORTS;a++)
        {dma_chan[a] = 0;}
    // update pointer to config
    config = worker;

    CFG_Packet *dev_config = get_CFG_Packet_pointer();

    // First load the programs into memory.  Right now only WS2812 is available
    offset_map[0][1] = pio_add_program(pio0, &ws2812_program); 
    offset_map[1][1] = pio_add_program(pio1, &ws2812_program);

    // Configure first state machine
    // This is first sm on Pio0
    for(port_id = 0;port_id < 8;port_id++)
    {
        if(dev_config->io.protocol[port_id])
        {
    //        pio = pio0;
    //        sm = 0;
            pio = pio_map[port_id];
            pio_num = pio_num_map[port_id];
            sm = sm_map[port_id];
        //    uint offset = pio_add_program(pio, &ws2812_program);
            switch(dev_config->io.protocol[port_id]){
                case 1 :  //ws2812
                    ws2812_program_init(pio, sm, offset_map[pio_num][1], dev_config->io.pin[port_id], 800000);
                    break;
                case 8 : //dmx
                    break;
            }
    //        ws2812_program_init(pio, sm, offset_map[0][dev_config.io.protocol[0]], dev_config->io.pin[0], 800000);

            dma_chan[port_id] = dma_claim_unused_channel(true);
            c = dma_channel_get_default_config(dma_chan[port_id]);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_read_increment(&c, true);
            channel_config_set_write_increment(&c,false);
            channel_config_set_dreq(&c, dreq_map[port_id]);

            dma_channel_configure(
                dma_chan[port_id],
                &c,
                &((pio_map[port_id])->txf[sm]), // Write address (only need to set this once)
                NULL,             // Don't provide a read address yet
                DEF_TRANSFER_COUNT, // Write the same value many times, then halt and interrupt
                false             // Don't start yet
            );
    printf("configured port %i offset %i pin %i dma %i \n\r",port_id,offset_map[pio_num][1],dev_config->io.pin[port_id],dma_chan[port_id]);
            config->led_string[port_id].active[0] = 1;
        }else
        {
            config->led_string[port_id].active[0] = 0;
        }

    }
}

void outputTrigger(struct repeating_timer *timer_data)
{
    *(uint32_t *)timer_data->user_data = 1;
}

void outputWork()
{
    int port_id;
    for(port_id=0;port_id < PIXEL_PORTS;port_id++)
    {
        // start port 0
        if ((dma_chan[port_id]) && (!dma_channel_is_busy(dma_chan[port_id])))
        {
    #ifdef _OUTPUT_DEBUG_
            printf("Output %i Start %d Channels %d\n\r",port_id,config->led_string[port_id].start_channel[0],config->led_string[port_id].channel_count[0]);
    #endif
            dma_channel_set_trans_count(dma_chan[port_id],config->led_string[port_id].channel_count[0],false);
            dma_channel_set_read_addr(dma_chan[port_id], &config->buffer[config->led_string[port_id].start_channel[0]], true);
        }
    }

}
