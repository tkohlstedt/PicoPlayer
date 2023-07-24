
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "ff.h" /* Obtains integer types */
//
#include "diskio.h" /* Declarations of disk functions */
//
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"
#include "fseq.h"
#include "PicoPlayer.h"
#include "outputManager.h"

inline uint32_t read4ByteUInt(const uint8_t* data) {
    uint32_t r = (data[0]) + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
    return r;
}
inline uint32_t read3ByteUInt(const uint8_t* data) {
    uint32_t r = (data[0]) + (data[1] << 8) + (data[2] << 16);
    return r;
}
inline uint32_t read2ByteUInt(const uint8_t* data) {
    uint32_t r = (data[0]) + (data[1] << 8);
    return r;
}
inline void write2ByteUInt(uint8_t* data, uint32_t v) {
    data[0] = (uint8_t)(v & 0xFF);
    data[1] = (uint8_t)((v >> 8) & 0xFF);
}
inline void write3ByteUInt(uint8_t* data, uint32_t v) {
    data[0] = (uint8_t)(v & 0xFF);
    data[1] = (uint8_t)((v >> 8) & 0xFF);
    data[2] = (uint8_t)((v >> 16) & 0xFF);
}
inline void write4ByteUInt(uint8_t* data, uint32_t v) {
    data[0] = (uint8_t)(v & 0xFF);
    data[1] = (uint8_t)((v >> 8) & 0xFF);
    data[2] = (uint8_t)((v >> 16) & 0xFF);
    data[3] = (uint8_t)((v >> 24) & 0xFF);
}

static struct FSEQFile fseqFile;
static FIL seqFile;
static struct repeating_timer outputTimer;
static thread_ctrl *localThreadCtlPtr;
static uint32_t framectr = 0;



inline void DumpHeader(const char* title, unsigned char data[], int len) {
    int x = 0;
    char tmpStr[128];

    printf(tmpStr, 128, "%s: (%d bytes)\n", title, len);

    for (int y = 0; y < len; y++) {
        if (x == 0) {
            printf(tmpStr, 128, "%06x: ", y);
        }
        printf(tmpStr + strlen(tmpStr), 128 - strlen(tmpStr), "%02x ", (int)(data[y] & 0xFF));
        x++;
        if (x == 16) {
            x = 0;
            printf(tmpStr + strlen(tmpStr), 128 - strlen(tmpStr), "\n");
        }
    }
    if (x != 0) {
        printf(tmpStr + strlen(tmpStr), 128 - strlen(tmpStr), "\n");
    }
}

int openFSEQFile(const char* fn) {

    FRESULT fr;
    printf("Opening file\n");
    fr = f_open(&seqFile,(const char*)fn, FA_READ);
    if (FR_OK != fr) {
        printf("Error pre-reading FSEQ file (%s), fopen returned NULL\n", fn);
        return -1;
    }
printf("Opened file\n");
    fseqFile.m_file_handle = &seqFile;
printf("Seek begining\n");
    f_lseek(&seqFile, 0L);

    // An initial read request of 8 bytes covers the file identifier, version fields and channel data offset
    // This is the minimum needed to validate the file and prepare the proper sized buffer for a larger read
    static const int initialReadLen = 8;

    unsigned char headerPeek[initialReadLen];
    int bytesRead; 
    fr = f_read(&seqFile,&headerPeek, initialReadLen, &bytesRead);

     // Validate bytesRead covers at least the initial read length
    if (bytesRead < initialReadLen) {
        printf( "Error pre-reading FSEQ file (%s) header, required %d bytes but read %d\n", fn, initialReadLen, bytesRead);
        DumpHeader("File header peek:", headerPeek, bytesRead);
        f_close(&seqFile);
        return -1;
    }

    // Validate the 4 byte file format identifier is supported
    if ((headerPeek[0] != 'P' && headerPeek[0] != 'F' && headerPeek[0] != V1ESEQ_HEADER_IDENTIFIER) || headerPeek[1] != 'S' || headerPeek[2] != 'E' || headerPeek[3] != 'Q') {
        printf( "Error pre-reading FSEQ file (%s) header, invalid identifier\n", fn);
        DumpHeader("File header peek:", headerPeek, bytesRead);
        f_close(&seqFile);
        return -1;
    }

    fseqFile.m_seqChanDataOffset = read2ByteUInt(&headerPeek[4]);
    fseqFile.m_seqVersionMinor = headerPeek[6];
    fseqFile.m_seqVersionMajor = headerPeek[7];

    // Test for a ESEQ file (identifier[0] == 'E')
    // ESEQ files are uncompressed V2 FSEQ files with a custom header
    if (headerPeek[0] == V1ESEQ_HEADER_IDENTIFIER) {
        fseqFile.m_seqChanDataOffset = V1ESEQ_CHANNEL_DATA_OFFSET;
        fseqFile.m_seqVersionMajor = V1ESEQ_MAJOR_VERSION;
        fseqFile.m_seqVersionMinor = V1ESEQ_MINOR_VERSION;
    }

    // Read the full header size (beginning at 0 and ending at seqChanDataOffset)
    uint8_t* header = malloc(fseqFile.m_seqChanDataOffset);
    f_lseek(&seqFile, 0L);
    fr = f_read(&seqFile,&header[0], fseqFile.m_seqChanDataOffset, &bytesRead);

    if (bytesRead != fseqFile.m_seqChanDataOffset) {
        printf( "Error reading FSEQ file (%s) header, length is %d bytes but read %d\n", fn, fseqFile.m_seqChanDataOffset, bytesRead);
        DumpHeader("File header:", &header[0], bytesRead);
        f_close(&seqFile);
        free(header);
        return -1;
    }
    fseqFile.m_variableHeaders = read2ByteUInt(&headerPeek[8]);
    fseqFile.m_seqChannelCount = read4ByteUInt(&header[10]);
    fseqFile.m_seqNumFrames = read4ByteUInt(&header[14]);
    fseqFile.m_seqStepTime = header[18];

    if(fseqFile.m_seqVersionMajor > 1){
        fseqFile.m_seqCompression = header[20]& 0x0f;
        fseqFile.m_seqCompressionBlocks = header[21];
        if(fseqFile.m_seqVersionMinor > 0){
            fseqFile.m_seqCompressionBlocks += (header[20] & 0xf0) * 0x0f; //V2.1+ has extra 4 bits in byte 20
        }
        fseqFile.m_seqSparseRanges = header[22];
    }

    // Return a file wrapper to handle version specific metadata
    if (fseqFile.m_seqCompression) {
        printf( "Error opening FSEQ file (%s), compressed data not supported\n", fn);
        DumpHeader("File header:", &header[0], bytesRead);
        f_close(&seqFile);
        return -1;
    }

    free(header);
    return 0;
}

int readFSEQFrame(uint8_t *buffer,uint32_t frame)
{
  FRESULT fr; 
  uint16_t bytesRead;
  uint64_t offset;
  offset = fseqFile.m_seqChanDataOffset + (fseqFile.m_seqChannelCount * frame );
  fr = f_lseek(fseqFile.m_file_handle,offset);  
  if(fr == FR_OK){
    fseqFile.m_memoryFramePos = 0;
    int st = readNextFSEQFrame(buffer);
    return st;
  } else {return -1;}
}

int readNextFSEQFrame(uint8_t *buffer)
{
    FRESULT fr;
    int bytesRead;
    fr = f_read(fseqFile.m_file_handle,&buffer[0],fseqFile.m_seqChannelCount, &bytesRead);
    if(fr == FR_OK){
        fseqFile.m_memoryFramePos++;
        return 0;
    }else{return -1;}
}

int playFSEQFile(thread_ctrl *hwconfig,const char *fname)
{
    printf("Playing %s\n",fname);
    int rval;
    localThreadCtlPtr = hwconfig;
    free(hwconfig->buffer); // just in case it didn't get de-allocated
    free(hwconfig->bufferb);
    rval = openFSEQFile(fname);
    dumpInfo(0);
    hwconfig->buffer = malloc(fseqFile.m_seqChannelCount);
    hwconfig->bufferb = malloc(fseqFile.m_seqChannelCount);
    hwconfig->run_mode = MODE_RUN;
    if (!rval){
        rval = readFSEQFrame(hwconfig->buffer,0);
        if(!rval){
            rval = readNextFSEQFrame(hwconfig->bufferb);
            if (!rval){
            printf("Starting %dms timer\n",fseqFile.m_seqStepTime);
            add_repeating_timer_ms(fseqFile.m_seqStepTime,(repeating_timer_callback_t)frameTimer,NULL,&outputTimer);
            }
        }
    }
    framectr = 0;
    return rval;
    
}

int closeFSEQFile(){
    FRESULT fr;
    printf("Done playing\n");
    fr = f_close(fseqFile.m_file_handle);
    free(localThreadCtlPtr->buffer);
    free(localThreadCtlPtr->bufferb);
    localThreadCtlPtr->buffer = NULL;
    localThreadCtlPtr->bufferb = NULL;
    localThreadCtlPtr->run_mode = MODE_STOPPED;
}

void frameTimer(){
    localThreadCtlPtr->tick = 1; //Just mark the tick 
}

int tickFSEQFile(){
    // trigger the channels to output from primary buffer (buffer)
    // then swap the buffers (buffer <-> bufferb)
    // channels will continue to output from what is now secondary buffer (bufferb)
    // finally, read the next frame into primary buffer (buffer)
    localThreadCtlPtr->tick = 0;  //reset tick 
    outputWork(); // start outputting channel data
    if(fseqFile.m_memoryFramePos == fseqFile.m_seqNumFrames){
        cancel_repeating_timer(&outputTimer);  // may drop last frame, may need to move this
        closeFSEQFile();
    }else{
        uint8_t *btemp;
        btemp = localThreadCtlPtr->bufferb;          // swap the buffers
        localThreadCtlPtr->bufferb = localThreadCtlPtr->buffer;
        localThreadCtlPtr->buffer = btemp;
        readNextFSEQFrame(localThreadCtlPtr->buffer);
    }
    if(!(framectr++ %100)){
        printf("Frame %d\r",framectr);
    }
}

void dumpInfo(bool indent) {
    char ind[5] = "    ";
    if (!indent) {
        ind[0] = 0;
    }
    printf("%sSequence File Information\n", ind);
    printf("%sseqFilename           : %s\n", ind, fseqFile.m_filename);
    printf("%sseqVersion            : %d.%d\n", ind, fseqFile.m_seqVersionMajor, fseqFile.m_seqVersionMinor);
    printf("%sseqChanDataOffset     : %d\n", ind, fseqFile.m_seqChanDataOffset);
    printf("%sseqChannelCount       : %d\n", ind, fseqFile.m_seqChannelCount);
    printf("%sseqNumPeriods         : %d\n", ind, fseqFile.m_seqNumFrames);
    printf("%sseqStepTime           : %dms\n", ind, fseqFile.m_seqStepTime);
    if(fseqFile.m_seqVersionMajor > 1) {
        printf("%sseqCompression        : %d\n", ind, fseqFile.m_seqCompression);
        printf("%sseqCompressionBlocks  : %d Blocks\n", ind, fseqFile.m_seqCompressionBlocks);
        printf("%sseqSparseRanges       : %d\n", ind, fseqFile.m_seqSparseRanges);
    }
}


inline bool isRecognizedStringVariableHeader(uint8_t a, uint8_t b) {
    // mf - media filename
    // sp - sequence producer
    // see https://github.com/FalconChristmas/fpp/blob/master/docs/FSEQ_Sequence_File_Format.txt#L48 for more information
    return (a == 'm' && b == 'f') || (a == 's' && b == 'p');
}
inline bool isRecognizedBinaryVariableHeader(uint8_t a, uint8_t b) {
    // FC - FPP Commands
    // FE - FPP Effects
    // ED - Extended data
    return (a == 'F' && b == 'C') || (a == 'F' && b == 'E') || (a == 'E' && b == 'D');
}
/*
void parseVariableHeaders(const uint8_t *header, int readIndex,uint16_t headerSize) {
    const int VariableCodeSize = 2;
    const int VariableLengthSize = 2;

    // when encoding, the header size is rounded to the nearest multiple of 4
    // this comparison ensures that there is enough bytes left to at least constitute a 2 byte length + 2 byte code
    while (readIndex + FSEQ_VARIABLE_HEADER_SIZE < headerSize) {
        int dataLength = read2ByteUInt(&header[readIndex]);
        readIndex += VariableLengthSize;

        uint8_t code0 = header[readIndex];
        uint8_t code1 = header[readIndex + 1];
        readIndex += VariableCodeSize;

        VariableHeader vheader;
        vheader.code[0] = code0;
        vheader.code[1] = code1;
        if (dataLength <= FSEQ_VARIABLE_HEADER_SIZE) {
            // empty data, advance only the length of the 2 byte code
            printf("VariableHeader has 0 length data: %c%c", code0, code1);
        } else if (code0 == 'E' && code1 == 'D') {
            // The actual data is elsewhere in the file
            code0 = header[readIndex];
            code1 = header[readIndex + 1];
            readIndex += VariableCodeSize;
            vheader.code[0] = code0;
            vheader.code[1] = code1;
            vheader.extendedData = true;

            uint64_t offset;
            memcpy(&offset, &header[readIndex], 8);
            uint32_t len;
            memcpy(&len, &header[readIndex + 8], 4);
            vheader.data.resize(len);

            uint64_t t = tell();
            seek(offset, SEEK_SET);
            read(&vheader.data[0], len);
            seek(t, SEEK_SET);
            readIndex += 12;
        } else if (readIndex + (dataLength - FSEQ_VARIABLE_HEADER_SIZE) > header.size()) {
            // ensure the data length is contained within the header
            // this is primarily protection against hand modified, or corrupted, sequence files
            printf("VariableHeader '%c%c' has out of bounds data length: %d bytes, max length: %d bytes\n", header[readIndex], header[readIndex + 1], readIndex + VariableCodeSize + dataLength, header.size());

            // there is no reasonable way to recover from this error - the reported dataLength is longer than possible
            // return from parsing variable headers and let the program attempt to read the rest of the file
            return;
        } else {
            // log when reading unrecongized variable headers
            if (!isRecognizedStringVariableHeader(header[readIndex], header[readIndex + 1])) {
                if (!isRecognizedBinaryVariableHeader(header[readIndex], header[readIndex + 1])) {
                    printf("Unrecognized VariableHeader code: %c%c, length: %d bytes\n", header[readIndex], header[readIndex + 1], dataLength);
                }
            } else {
                // print a warning if the data is not null terminated
                // this is to assist debugging potential string related issues
                // the data is not forcibly null terminated to avoid mutating unknown data
                if (header.size() < readIndex + VariableCodeSize + dataLength) {
                    printf( "VariableHeader %c%c data exceeds header buffer size!  %d > %d\n",
                           header[readIndex], header[readIndex + 1], (readIndex + VariableCodeSize + dataLength), header.size());
                } else if (header[readIndex + VariableCodeSize + dataLength - 1] != '\0') {
                    printf("VariableHeader %c%c data is not NULL terminated!\n", header[readIndex], header[readIndex + 1]);
                }
            }
            dataLength -= FSEQ_VARIABLE_HEADER_SIZE;

            vheader.data.resize(dataLength);
            memcpy(&vheader.data[0], &header[readIndex], dataLength);


            printf("Read VariableHeader: %c%c, length: %d bytes\n", vheader.code[0], vheader.code[1], dataLength);

            // advance the length of the data
            // readIndex now points at the next VariableHeader's length (if any)
            readIndex += dataLength;
        }
        m_variableHeaders.push_back(vheader);
    }
}

class UncompressedFrameData : public FSEQFile::FrameData {
public:
    UncompressedFrameData(uint32_t frame,
                          uint32_t sz,
                          const std::vector<std::pair<uint32_t, uint32_t>>& ranges) :
        FrameData(frame),
        m_ranges(ranges) {
        m_size = sz;
        m_data = (uint8_t*)malloc(sz);
    }
    virtual ~UncompressedFrameData() {
        if (m_data != nullptr) {
            free(m_data);
        }
    }

    virtual bool readFrame(uint8_t* data, uint32_t maxChannels) override {
        if (m_data == nullptr)
            return false;
        uint32_t offset = 0;
        for (auto& rng : m_ranges) {
            uint32_t toRead = rng.second;
            if (offset + toRead <= m_size) {
                uint32_t toCopy = std::min(toRead, maxChannels - rng.first);
                memcpy(&data[rng.first], &m_data[offset], toCopy);
                offset += toRead;
            } else {
                return false;
            }
        }
        return true;
    }

    uint32_t m_size;
    uint8_t* m_data;
    std::vector<std::pair<uint32_t, uint32_t>> m_ranges;
};

static const int V2FSEQ_HEADER_SIZE = 32;
static const int V2FSEQ_SPARSE_RANGE_SIZE = 6;
static const int V2FSEQ_COMPRESSION_BLOCK_SIZE = 8;
#if !defined(NO_ZLIB) || !defined(NO_ZSTD)
static const int V2FSEQ_OUT_BUFFER_SIZE = 1024 * 1024;          // 1MB output buffer
static const int V2FSEQ_OUT_BUFFER_FLUSH_SIZE = 900 * 1024;     // 90% full, flush it
static const int V2FSEQ_OUT_COMPRESSION_BLOCK_SIZE = 64 * 1024; // 64KB blocks
#endif
*/