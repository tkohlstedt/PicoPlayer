#include <stdint.h>
#include "ff.h"
#include "PicoPlayer.h"

static const int V1FSEQ_MINOR_VERSION = 0;
static const int V1FSEQ_MAJOR_VERSION = 1;

static const int V2FSEQ_MINOR_VERSION = 0;
static const int V2FSEQ_MAJOR_VERSION = 2;

static const int V1ESEQ_MINOR_VERSION = 0;
static const int V1ESEQ_MAJOR_VERSION = 2;
static const int V1ESEQ_HEADER_IDENTIFIER = 'E';
static const int V1ESEQ_CHANNEL_DATA_OFFSET = 20;
static const int V1ESEQ_STEP_TIME = 50;

#define FSEQ_VARIABLE_HEADER_SIZE 2

struct FSEQFile{
    char m_filename[128];
    uint8_t m_seqVersionMajor;
    uint8_t m_seqVersionMinor;
    uint32_t m_seqNumFrames;
    uint32_t m_seqChannelCount;
    uint8_t m_seqStepTime;
    uint8_t m_seqCompression;   //bits 0-3 = compression flags, bit 4-7 =high bits of compression blocks
    uint16_t m_seqCompressionBlocks;
    uint8_t m_seqSparseRanges;
    uint16_t m_variableHeaders; //offset to start of variable headers
    uint64_t m_uniqueId;
 //   m_seqFileSize(0),
 //   m_memoryBuffer(),
    uint16_t m_seqChanDataOffset;
    uint32_t m_memoryFramePos;
    FIL* m_file_handle;
} ;
int openFSEQFile(const char* fn);
int playFSEQFile(thread_ctrl *hwconfig,const char *fname);
void dumpInfo(bool indent);
int readFSEQFrame(uint8_t *buffer,uint32_t frame);
int readNextFSEQFrame(uint8_t *buffer);
int tickFSEQFile();
void frameTimer();