#include <stdio.h>
#include <string.h>
#include <intrin.h>
#include <Windows.h>
#include <mmreg.h>
struct STDSTREAM
{
    FILE* file;
    int filepos;
    int filesize;
    int currentwritepos;
    char* pointer;
    char* pointer2;
    int iswritable;
    int needsflush;
    int filemode;
    char fname[1024];
    char fname2[1024];
    char buffer[8192];
};

enum codectype
{
    s16l_int = 0,
    s16b_int = 1,
    s8_int = 2,
    mt_blk = 4,
    vag_blk = 5,
    s16b_blk = 7,
    s16l_blk = 8,
    s8_blk = 9,
    eaxa_blk = 10,
    u8_int = 11,
    cdxa = 12,
    dvi_int = 13,
    layer3 = 16,
    gcadpcm = 18,
    s24l_int = 19,
    xboxadpcm = 20,
    s24b_int = 21,
    mt5_blk = 22,
    ealayer3 = 23,
    xas0_int = 24,
    ealayer30_int = 25,
    atrac3_int = 26,
    atrac3plus_int = 27,
    eaxma = 28,
    xas_int = 29,
    ealayer3_int = 30,
    ealayer3pcm_int = 31,
    ealayer3spike_int = 32,
};

enum playlocation
{
    PLAYLOC_MAINCPU = 0x4,
    PLAYLOC_SPU = 0x8,
    PLAYLOC_DS3DHW = 0x10,
    PLAYLOC_IOPCPU = 0x100,
    PLAYLOC_DSP = 0x200,
    PLAYLOC_DS2DHW = 0x400,
    PLAYLOC_RAM = 0x800,
    PLAYLOC_STREAM = 0x1000,
    PLAYLOC_GIGASAMPLE = 0x2000,
};

enum SndPlayerPlayType
{
    SNDPLAYER_PLAYTYPE_RAM = 0,
    SNDPLAYER_PLAYTYPE_STREAM = 1,
    SNDPLAYER_PLAYTYPE_GIGASAMPLE = 2,
    SNDPLAYER_PLAYTYPE_MAX = 3
};

int buffertobytes(unsigned char* p)
{
    return (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

short buffertobytesshort(unsigned char* p)
{
    return (p[0]) | (p[1] << 8);
}

void bytestobuffer(unsigned char* buf, int i)
{
    buf[0] = (unsigned char)((i >> 24) & 0xFF);
    buf[1] = (unsigned char)((i >> 16) & 0xFF);
    buf[2] = (unsigned char)((i >> 8) & 0xFF);
    buf[3] = (unsigned char)(i & 0xFF);
}
STDSTREAM* gopen(const char* filename)
{
    STDSTREAM* s = nullptr;
    FILE* f = fopen(filename, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        s = new STDSTREAM;
        s->pointer = s->buffer;
        s->pointer2 = s->buffer;
        s->filesize = ftell(f);
        fseek(f, 0, SEEK_SET);
        s->filemode = 1;
        s->file = f;
        s->filepos = 0;
        s->iswritable = 0;
        s->needsflush = 0;
        s->currentwritepos = 0;
        strcpy(s->fname, filename);
    }
    return s;
}

void strappend(char* s1, const char* s2, const char* s3)
{
    strcat(strcpy(s1, s2), s3);
}

void buildfilename(char* s1, const char* s2, const char* s3, const char* s4)
{
    char* s5 = strcpy(s1, s2);
    if (s3)
    {
        strcat(s5, s3);
    }
    strcat(s5, ".");
    strcat(s5, s4);
}

STDSTREAM* gwopen(char* filename)
{
    char fname[1024];
    fname[0] = 0;
    strappend(fname, filename, ".tm1");
    FILE* f = fopen(fname, "w+b");
    if (!f)
    {
        f = fopen(fname, "wb");
    }
    if (!f)
    {
        return nullptr;
    }
    STDSTREAM* s = new STDSTREAM;
    s->filemode = 3;
    s->pointer = s->buffer;
    s->pointer2 = s->buffer;
    s->file = f;
    s->filepos = 0;
    s->filesize = 0;
    s->iswritable = 1;
    s->needsflush = 0;
    s->currentwritepos = 0;
    strcpy(s->fname, filename);
    strcpy(s->fname2, fname);
    return s;
}

void WriteFlush(STDSTREAM* g)
{
    if (g->needsflush && g->iswritable)
    {
        fseek(g->file, g->currentwritepos, SEEK_SET);
        int ptr = g->pointer2 - (char *)g;
        if (ptr - 2084 > 0)
        {
            fwrite(g->buffer, 1, ptr - 2084, g->file);
        }
        g->needsflush = 0;
    }
    g->pointer2 = g->buffer;
    g->pointer = g->buffer;
    fflush(g->file);
}

int removefile(const char* fname)
{
    return remove(fname) == 0;
}

int renamefile(const char* fname1, const char* fname2)
{
    int ret2 = rename(fname1, fname2);
    int ret = ret2 == 0;
    if (!ret2 || errno != EEXIST || remove(fname2))
    {
        return ret;
    }
    else
    {
        return rename(fname1, fname2) == 0;
    }
}

int gclose(STDSTREAM* g)
{
    int ret = 1;
    if (g)
    {
        WriteFlush(g);
        if (g->filemode)
        {
            int ret2 = fclose(g->file);
            ret = ret2 == 0;
            if (!ret2 && (g->filemode == 2 || g->filemode == 3))
            {
                char fname1[1024];
                char fname2[1024];
                strappend(fname1, g->fname, ".tm2");
                removefile(fname1);
                renamefile(g->fname, fname1);
                ret = renamefile(g->fname2, g->fname);
                if (ret)
                {
                    if (g->filemode == 2)
                    {
                        strappend(fname2, g->fname, ".bak");
                        removefile(fname2);
                        renamefile(fname1, fname2);
                    }
                    else
                    {
                        removefile(fname1);
                    }
                }
            }
        }
    }
    return ret;
}

int gread(STDSTREAM* g, void* buf, int size)
{
    int i1 = 0;
    int i2 = 0;
    if (!g)
    {
        return 0;
    }
    int i3 = size;
    if (size < 0)
    {
        i3 = 0;
    }
    char* p1 = g->pointer;
    int i4 = g->pointer2 - p1;
    if (i3 < i4)
    {
        i4 = i3;
    }
    char* p2 = (char*)buf;
    if (i4 > 0)
    {
        memcpy(buf, p1, i4);
        g->pointer += i4;
        g->filepos += i4;
        p2 = (char*)buf + i4;
        i3 -= i4;
        i1 = i4;
    }
    int filepos = g->filepos;
    if (i3 >= g->filesize - filepos)
    {
        i3 = g->filesize - filepos;
    }
    if (i3 >= 0)
    {
        if (g->pointer2 == g->pointer && i3)
        {
            int i5 = (unsigned int)(filepos + i3) >> 13 << 13;
            WriteFlush(g);
            int i6 = g->filepos;
            if (i6 < i5)
            {
                int i7 = fread(p2, 1, i5 - i6, g->file);
                p2 += i7;
                i2 = i7;
                i3 -= i7;
            }
            unsigned int i8 = g->filesize - i5;
            g->currentwritepos = i5;
            int i9 = 8192;
            if (i8 <= 8192)
            {
                i9 = i8;
            }
            fread(g->buffer, 1, i9, g->file);
            memcpy(p2, g->pointer, i3);
            g->pointer += i3;
            g->pointer2 += i9;
            g->filepos += i3 + i2;
            i2 += i3;
        }
        return i1 + i2;
    }
    else
    {
        return i1;
    }
}

int gwrite(STDSTREAM* g, void* buf, int size)
{
    int i1 = 0;
    if (!g)
    {
        return 0;
    }
    if (!g->iswritable)
    {
        return 0;
    }
    char* p1 = g->pointer;
    int i2 = size;
    int i3 = (char*)g - p1 + 10276;
    if (size < i3)
    {
        i3 = size;
    }
    if (i3 > 0)
    {
        memcpy(p1, buf, i3);
        g->pointer += i3;
        p1 = g->pointer;
        g->filepos += i3;
        int i4 = g->filepos;
        if (g->pointer2 < p1)
        {
            g->pointer2 = p1;
        }
        if (i4 > g->filesize)
        {
            g->filesize = i4;
        }
        buf = (char*)buf + i3;
        g->needsflush = 1;
        i2 = size - i3;
        i1 = i3;
    }
    if (g->pointer2 == p1 && i2)
    {
        int i5 = (unsigned int)(i2 + g->filepos) >> 13 << 13;
        WriteFlush(g);
        int i6 = g->filesize;
        if (i6 > i5)
        {
            int i7 = i6 - i5;
            if (i7 > 8192)
            {
                i7 = 8192;
            }
            fseek(g->file, i5, 0);
            fread(g->buffer, 1, i7, g->file);
            g->pointer2 += i7;
        }
        fseek(g->file, g->filepos, 0);
        int i8 = 0;
        if (i5 - g->filepos > 0)
        {
            i8 = fwrite(buf, 1, i5 - g->filepos, g->file);
        }
        int i9 = g->filepos;
        if (i8 < i5 - i9)
        {
            return i8 + i1;
        }
        i1 += i2;
        g->filepos = i8 + i9;
        int i10 = i2 - i8;
        memcpy(g->buffer, (char*)buf + i8, i10);
        g->filepos += i10;
        int i11 = g->filepos;
        g->pointer += i10;
        char* p2 = g->pointer;
        g->currentwritepos = i5;
        if (i11 > g->filesize)
        {
            g->filesize = i11;
        }
        if (p2 > g->pointer2)
        {
            g->pointer2 = p2;
        }
        g->needsflush = 1;
    }
    return i1;
}

int gseek(STDSTREAM* g, __int64 pos)
{
    int i1 = 1;
    int result;
    if (!g)
    {
        return 0;
    }
    __int64 i2 = pos;
    if (i2 < 0)
    {
        i2 = 0;
        i1 = 0;
    }
    else
    {
        if (pos > g->filesize && !g->iswritable)
        {
            i2 = g->filesize;
            i1 = 0;
        }
    }
    __int64 i4 = i2 - g->filepos;
    unsigned int i5 = (unsigned int)(i4 + g->filepos);
    g->filepos = i5;
    if (i2 < (char*)g - g->pointer + 2084 || i2 > g->pointer2 - g->pointer)
    {
        int i6 = i5 >> 13 << 13;
        WriteFlush(g);
        int i1a;
        bool b;
        if (fseek(g->file, i6, SEEK_SET) || (b = 1, i1a = 1, b))
        {
            i1a = 0;
        }
        int size = g->filesize;
        g->currentwritepos = i6;
        if (size <= i6)
        {
            int i7 = g->filepos - i6;
            memset(g->buffer, 0, i7);
            result = i1a;
            g->pointer2 = &g->buffer[i7];
            g->pointer = &g->buffer[i7];
        }
        else
        {
            unsigned int i8 = size - i6;
            int i9 = 8192;
            if (i8 <= 8192)
            {
                i9 = i8;
            }
            fread(g->buffer, 1, i9, g->file);
            g->pointer2 += i9;
            g->pointer += g->filepos - i6;
            result = i1a;
        }
    }
    else
    {
        result = i1;
        g->pointer = &g->pointer[i4];
    }
    return result;
}

class Encoder
{
public:
    enum BitRateManagement
    {
        BITRATEMANAGEMENT_USINGVBR = 0,
        BITRATEMANAGEMENT_USINGCBR = 1
    };
    virtual int Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, int numSamplesOut) = 0;
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, int numSamplesOut) = 0;
    virtual int GetDataRateOverhead()
    {
        return 10240 * mNumChannels;
    }
    virtual int GetSeekMemoryRequired(int __formal)
    {
        return 0;
    }
    virtual void Release()
    {
        delete this;
    }
    virtual ~Encoder()
    {
    }
    int GetEncodeMemoryRequired(int numSamples)
    {
        return GetDataRateOverhead() + (int)(mAverageDataRate * (float)numSamples / (float)mSampleRate);
    }
    float mAverageDataRate;
    float mVbrQuality;
    int mCbrRate;
    int mLoopStart;
    int mLoopEnd;
    int mSampleRate;
    unsigned char mNumChannels;
    unsigned char mBitRateManagement;
    bool mIsChunked;
};

class EncoderExtended
{
public:
    float GetAverageDataRate()
    {
        return mpEncoder->mAverageDataRate;
    }
    int GetEncodeMemoryRequired(int numSamples)
    {
        return mpEncoder->GetDataRateOverhead() + (int)((float)(numSamples + 256) * mpEncoder->mAverageDataRate / (float)mpEncoder->mSampleRate);
    }
    void SetVbrQuality(float quality)
    {
        mpEncoder->mVbrQuality = quality;
        mpEncoder->mBitRateManagement = Encoder::BITRATEMANAGEMENT_USINGVBR;
    }
    void SetIsChunked(bool isChunked)
    {
        mpEncoder->mIsChunked = isChunked;
    }
    void SetCbrRate(int bitsPerSecond)
    {
        mpEncoder->mCbrRate = bitsPerSecond;
        mpEncoder->mBitRateManagement = Encoder::BITRATEMANAGEMENT_USINGCBR;
    }
    static void TranslateS16ToF32(short** pSrc, float* pDst, int channels, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            for (int j = 0; j < channels; ++j)
            {
                *pDst = (float)((double)*pSrc[j] * 0.000030517578);
                ++pDst;
                ++pSrc[j];
            }
        }
        for (int k = numSamples; k < 256; ++k)
        {
            for (int l = 0; l < channels; ++l)
            {
                *pDst = (float)((double)*(pSrc[l] - 1) * 0.000030517578);
                ++pDst;
            }
        }
    }
    int Flush(unsigned char* pEncodedData, int* pBytesEncoded,  int numSamplesOut)
    {
        return mpEncoder->Flush(pEncodedData, pBytesEncoded, numSamplesOut);
    }
    int Encode(short** pSampleData, unsigned char* pEncodedData, int numSamplesIn, int* pBytesEncoded, int numSamplesOut)
    {
        short* pSrc[64];
        float pDst[16384];
        int encodedbytes;
        int channels = this->mpEncoder->mNumChannels;
        int result = 0;
        if (channels > 0)
        {
            memcpy(pSrc, pSampleData, 4 * channels);
        }
        for (*pBytesEncoded = 0; numSamplesIn > 0; numSamplesIn -= 256)
        {
            int samples = numSamplesIn;
            if (numSamplesIn >= 256)
            {
                samples = 256;
            }
            TranslateS16ToF32(pSrc, pDst, mpEncoder->mNumChannels, samples);
            result += mpEncoder->Encode(pDst, pEncodedData, samples, &encodedbytes, numSamplesOut);
            *pBytesEncoded += encodedbytes;
            pEncodedData += encodedbytes;
        }
        return result;
    }
    Encoder* mpEncoder;
};

class Pcm16BigEnc : public Encoder
{
public:
    Pcm16BigEnc(int channels, int bitrate)
    {
        mCbrRate = 4000;
        mAverageDataRate = (float)(bitrate * channels) + (float)(bitrate * channels);
        mVbrQuality = 0.0f;
    }
    virtual int Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, int numSamplesOut)
    {
        int result = numSamplesIn;
        int i1 = numSamplesIn * mNumChannels;
        unsigned char* p1 = pDst;
        if (i1 > 0)
        {int i2 = numSamplesIn * mNumChannels;
            do
            {
                float f1 = *pSrc * 32767.0f;
                *p1 = (unsigned short)(int)f1 >> 8;
                p1[1] = (int)f1;
                ++pSrc;
                p1 += 2;
                --i2;
            } while (i2);
            result = numSamplesIn;
        }
        *pBytesEncoded = 2 * i1;
        return result;
    }
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, int numSamplesOut)
    {
        *pBytesEncoded = 0;
        return 0;
    }
};

enum SndPlayerCodec
{
    SNDPLAYER_CODEC_XAS_INT = 0,
    SNDPLAYER_CODEC_EALAYER3_INT = 1,
    SNDPLAYER_CODEC_SIGN16BIG_INT = 2,
    SNDPLAYER_CODEC_EAXMA = 3,
    SNDPLAYER_CODEC_XAS1_INT = 4,
    SNDPLAYER_CODEC_EALAYER31_INT = 5,
    SNDPLAYER_CODEC_EALAYER32PCM_INT = 6,
    SNDPLAYER_CODEC_EALAYER32SPIKE_INT = 7,
    SNDPLAYER_CODEC_GCADPCM = 8,
    SNDPLAYER_CODEC_EASPEEX = 9,
    SNDPLAYER_CODEC_MAX = 10
};

struct SSOUND
{
    int filepos = 0xFFFFFFFF;
    short* inputsamples[6] = {};
    unsigned char codec = 0xFF;
    char numchannels = 0;
    short playloc = 0;
    int bitrate = 50;
    int samplerate = 0;
    int numsamples = 0;
    STDSTREAM* filestruct = 0;
};

struct EncoderStruct
{
    EncoderExtended* encoder;
    unsigned char* encodebuffer;
    unsigned char* encodeptr;
    int encodesize;
    bool hasencodeddata;
};

EncoderStruct* CreateEncoder(SSOUND* sound, int encoder)
{
    EncoderStruct* e = new EncoderStruct;
    switch (encoder)
    {
    case xas_int:
        //todo
        e->encoder = nullptr;
        break;
    case s16b_int:
        {
            e->encoder = new EncoderExtended;
            e->encoder->mpEncoder = new Pcm16BigEnc(sound->numchannels, sound->samplerate);
            e->encoder->mpEncoder->mNumChannels = sound->numchannels;
            e->encoder->mpEncoder->mSampleRate = sound->samplerate;
        }
        break;
    case ealayer3_int:
        //todo
        e->encoder = nullptr;
        break;
    case ealayer3pcm_int:
        //todo
        e->encoder = nullptr;
        break;
    }
    e->encodebuffer = nullptr;
    e->encodeptr = nullptr;
    e->encodesize = 0;
    e->hasencodeddata = false;
    return e;
}

class PutBits {
public:
    unsigned int mBitPosition = 0;
    unsigned char bits[1028];
    void addbits(unsigned int data, int numBits)
    {
        unsigned int i1 = data << (32 - numBits) >> (32 - numBits);
        unsigned int i2 = data << (32 - numBits) >> (32 - numBits);
        if (numBits > 0)
        {
            for (;;)
            {
                unsigned int i3 = mBitPosition & 7;
                int i4 = 8 - i3;
                unsigned int i5 = this->mBitPosition >> 3;
                if (i3 == 0)
                {
                    bits[i5] = 0;
                }
                unsigned int i6 = 8 - i3;
                if (numBits <= i4)
                {
                    i6 = numBits;
                }
                numBits -= i6;
                bits[i5] |= (unsigned char)(i1 >> numBits) << (i4 - i6);
                mBitPosition += i6;
                if (numBits <= 0)
                {
                    break;
                }
                i1 = i2;
            }
        }
    }
};

int WriteSnrHeader(SSOUND* sound, STDSTREAM* file)
{
    PutBits pb;
    pb.addbits(0, 4);
    switch (sound->codec)
    {
    case xas_int:
        pb.addbits(SNDPLAYER_CODEC_XAS1_INT, 4);
        break;
    case ealayer3pcm_int:
        pb.addbits(SNDPLAYER_CODEC_EALAYER32PCM_INT, 4);
        break;
    case ealayer3_int:
        pb.addbits(SNDPLAYER_CODEC_EALAYER31_INT, 4);
        break;
    case s16b_int:
        pb.addbits(SNDPLAYER_CODEC_SIGN16BIG_INT, 4);
        break;
    }
    pb.addbits(sound->numchannels - 1, 6);
    pb.addbits(sound->samplerate, 18);
    if (sound->playloc & PLAYLOC_RAM || !sound->playloc)
    {
        pb.addbits(SNDPLAYER_PLAYTYPE_RAM, 2);
    }
    else if (sound->playloc & PLAYLOC_STREAM)
    {
        pb.addbits(SNDPLAYER_PLAYTYPE_STREAM, 2);
    }
    pb.addbits(0, 1);
    pb.addbits(sound->numsamples, 29);
    unsigned int i2 = pb.mBitPosition >> 3;
    if ((int)(pb.mBitPosition >> 3) >= 64)
    {
        i2 = 64;
    }
    gwrite(file, pb.bits, i2);
    return 0;
}

int EncodeBlock(EncoderStruct* encoder, __int16** sampledata, unsigned char** encodebuffer, int numsamples, int* encodedbytes, SSOUND* sound)
{
    int newencodesize = encoder->encoder->GetEncodeMemoryRequired(numsamples);
    int encodesize = encoder->encodesize;
    if (encodesize < newencodesize)
    {
        if (encodesize)
        {
            delete[] encoder->encodebuffer;
        }
        encoder->encodebuffer = new unsigned char[newencodesize];
        encoder->encodesize = newencodesize;
    }
    int bitrate = sound->bitrate;
    float quality;
    if (bitrate < 0)
    {
        if (bitrate <= 100)
        {
            quality = 0.89999998f;
            goto l1;
        }
    l2:
        encoder->encoder->SetCbrRate(bitrate);
        goto l3;
    }
    if (bitrate > 100)
    {
        goto l2;
    }
    quality = (float)sound->bitrate * 0.009999999776482582f;
l1:
    encoder->encoder->SetVbrQuality(quality);
l3:
    int result = encoder->encoder->Encode(sampledata, encoder->encodebuffer, numsamples, encodedbytes, 0);
    *encodebuffer = encoder->encodebuffer;
    encoder->encodeptr = &encoder->encodebuffer[*encodedbytes];
    encoder->hasencodeddata = 1;
    return result;
}

int WriteSnrBlock(STDSTREAM** file, SSOUND* sound, EncoderStruct* encoder, int sampleoffset, int numsamplesin, int isstream, int shouldflush, int* numsamplesout)
{
    int result = numsamplesin;
    *numsamplesout = 0;
    STDSTREAM* file2 = *file;
    int totalencodedsamples = 0;
    int currentsampleoffset = sampleoffset;
    int numsamples = numsamplesin;
    if (numsamplesin > 0)
    {
        short** sampledata = new short* [sound->numchannels];
        do
        {
            if (isstream)
            {
                float averagedatarate = encoder->encoder->GetAverageDataRate();
                float rate = (float)sound->samplerate * (2040.0f / averagedatarate);
                numsamples = (int)rate;
                if ((int)rate > numsamplesin)
                {
                    numsamples = numsamplesin;
                }
            }
            short **inputsamples = sound->inputsamples;
            for (int count = 0; count < sound->numchannels; count++)
            {
                sampledata[count] = &(*inputsamples)[currentsampleoffset];
                inputsamples++;
            }
            unsigned char* encodebuffer = nullptr;
            unsigned char* extradataptr = nullptr;
            int encodedbytes = 0;
            int encodedbytes3 = 0;
            int encodedsamples = EncodeBlock(encoder, sampledata, &encodebuffer, numsamples, &encodedbytes, sound);
            int blocksamples = encodedsamples;
            if (encodedsamples <= 0 && !shouldflush)
            {
                break;
            }
            int encodedbytes2 = encodedbytes;
            numsamplesin -= numsamples;
            currentsampleoffset += numsamples;
            int paddingsize = 0;
            if (encodedbytes > 0 || shouldflush)
            {
                unsigned int blocksize;
                unsigned int blocksize2;
                if (numsamplesin > 0)
                {
                    blocksize = encodedbytes + 8;
                    blocksize2 = encodedbytes + 8;
                }
                else
                {
                    if (shouldflush)
                    {
                        int flushbytes;
                        if (encoder->hasencodeddata)
                        {
                            flushbytes = encoder->encoder->Flush(encoder->encodeptr, &encodedbytes3, 0);
                            extradataptr = encoder->encodeptr;
                            encoder->hasencodeddata = 0;
                        }
                        encodedbytes2 = encodedbytes;
                        blocksamples = flushbytes + encodedsamples;
                    }
                    blocksize = encodedbytes3 + encodedbytes2 + 8;
                    blocksize2 = encodedbytes3 + encodedbytes2 + 8;
                    if (isstream)
                    {
                        blocksize |= 0x80000000;
                    }
                }
                totalencodedsamples += encodedbytes2 + encodedbytes3;
                bool blockhasdata = blocksize2 > 8;
                bool blockhasdata2 = blocksize2 > 8;
                if (blockhasdata)
                {
                    totalencodedsamples += 8;
                    if (numsamplesin <= 0 && isstream)
                    {
                        blocksize2 += 64 - totalencodedsamples % 64;
                        blocksize += 64 - totalencodedsamples % 64;
                        paddingsize = 64 - totalencodedsamples % 64;
                        totalencodedsamples += paddingsize;
                    }
                    unsigned char blockheaderbuffer[8];
                    bytestobuffer(blockheaderbuffer, blocksize);
                    bytestobuffer(&blockheaderbuffer[4], blocksamples);
                    gwrite(file2, blockheaderbuffer, 8);
                    blockhasdata = blockhasdata2;
                }
            }
            *numsamplesout += blocksamples;
            gwrite(file2, encodebuffer, encodedbytes);
            if (numsamplesin <= 0 && shouldflush)
            {
                gwrite(file2, extradataptr, encodedbytes3);
            }
            if (paddingsize)
            {
                char paddingbuffer[64];
                memset(paddingbuffer, 0, 64);
                gwrite(file2, paddingbuffer, paddingsize);
            }
        } while (numsamplesin);
        delete[] sampledata;
        result = totalencodedsamples;
    }
    return result;
}

int EncodeSnr(const char* filename, SSOUND* sound)
{
    int retval = 0;
    EncoderStruct* encoderstruct = CreateEncoder(sound, sound->codec);
    char snrpath[1024];
    buildfilename(snrpath, filename, nullptr, "snr");
    remove(snrpath);
    char snspath[1024];
    buildfilename(snspath, filename, nullptr, "sns");
    remove(snspath);
    char snrhpath[1024];
    buildfilename(snrhpath, filename, "H", "snr");
    remove(snrhpath);
    STDSTREAM* snrhfile = gwopen(snrhpath);
    if (sound->playloc == PLAYLOC_STREAM)
    {
        encoderstruct->encoder->SetIsChunked(true);
    }
    else
    {
        encoderstruct->encoder->SetIsChunked(false);
    }
    int snsopen = 0;
    int numsamples = sound->numsamples;
    for (int sampleoffset = 0;sampleoffset < numsamples;)
    {
        if (!snsopen && sound->playloc == PLAYLOC_STREAM)
        {
            gclose(snrhfile);
            renamefile(snrhpath, snrpath);
            buildfilename(snrhpath, filename, "S", "sns");
            remove(snrhpath);
            snrhfile = gwopen(snrhpath);
            snsopen = 1;
        }
        int isstream = sound->playloc == PLAYLOC_STREAM;
        int numsamplesin = sound->numsamples;
        int shouldflush = 1;
        int numsamplesout;
        int encodedsamples = WriteSnrBlock(&snrhfile, sound, encoderstruct, sampleoffset, numsamplesin, isstream, shouldflush, &numsamplesout);
        retval += encodedsamples;
        sampleoffset += numsamplesin;
    }
    int shouldremovesnrh = 0;
    gseek(snrhfile, 0);
    if (!snrhfile->filesize)
    {
        shouldremovesnrh = 1;
    }
    gclose(snrhfile);
    if (shouldremovesnrh)
    {
        removefile(snrhpath);
    }
    else if (snsopen)
    {
        renamefile(snrhpath, snspath);
    }
    else
    {
        renamefile(snrhpath, snrpath);
    }
    encoderstruct->encoder->mpEncoder->Release();
    delete encoderstruct->encoder;
    if (encoderstruct->encodebuffer)
    {
        delete[] encoderstruct->encodebuffer;
    }
    STDSTREAM* snrfile = gopen(snrpath);
    char* snrfiledata = nullptr;
    int snrfilesize = snrfile->filesize;
    if (snrfilesize)
    {
        snrfiledata = new char[snrfilesize];
        gread(snrfile, snrfiledata, snrfilesize);
    }
    gclose(snrfile);
    removefile(snrpath);
    buildfilename(snrhpath, filename, "H", "snr");
    removefile(snrhpath);
    STDSTREAM* file = gwopen(snrhpath);
    if (WriteSnrHeader(sound, file) >= 0)
    {
        if (snrfilesize)
        {
            gwrite(file, snrfiledata, snrfilesize);
            delete[] snrfiledata;
        }
        gclose(file);
        renamefile(snrhpath, snrpath);
    }
    return retval;
}

int WaveInfo(const char* filename, SSOUND **newsound)
{
    STDSTREAM* file = gopen(filename);
    unsigned char buf1[12];
    gread(file, buf1, 12);
    unsigned char buf2[8];
    if (gread(file, buf2, 8))
    {
        while (strncmp((char*)buf2, "fmt ", 4))
        {
            int size = buffertobytes(&buf2[4]);
            gseek(file, size + file->filepos);
            gread(file, buf2, 8);
        }
    }
    int filepos = file->filepos;
    PCMWAVEFORMAT w;
    gread(file, &w, sizeof(w));
    if (w.wf.wFormatTag == WAVE_FORMAT_PCM || w.wf.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        SSOUND* sound = new SSOUND;
        *newsound = sound;
        sound->numchannels = (char)w.wf.nChannels;
        int size = buffertobytes(&buf2[4]);
        gseek(file, size + filepos);
        if (gread(file, buf2, 8))
        {
            while (strncmp((char*)buf2, "data", 4))
            {
                size = buffertobytes(&buf2[4]);
                gseek(file, size + file->filepos);
                gread(file, buf2, 8);
            }
        }
        sound->samplerate = w.wf.nSamplesPerSec;
        size = buffertobytes(&buf2[4]);
        if (w.wBitsPerSample == 8)
        {
            sound->codec = u8_int;
            sound->numsamples = size / sound->numchannels;
        }
        else if (w.wBitsPerSample == 16)
        {
            sound->codec = s16l_int;
            sound->numsamples = size / sound->numchannels / 2;
        }
        else if (w.wBitsPerSample == 24)
        {
            sound->codec = s24l_int;
            sound->numsamples = size / sound->numchannels / 3;
        }
        sound->filepos = file->filepos;
        sound->filestruct = file;
        return 1;
    }
    return 0;
}

void AllocateWaveMemory(SSOUND* sound)
{
    for (int i = 0; i < sound->numchannels; i++)
    {
        sound->inputsamples[i] = new short[sound->numsamples];
    }
}

void ReorderChannels(SSOUND* sound)
{
    unsigned char numchannels = sound->numchannels;
    if (numchannels == 4)
    {
        short *p1 = sound->inputsamples[2];
        sound->inputsamples[2] = sound->inputsamples[3];
        sound->inputsamples[3] = p1;
    }
    if (numchannels == 6)
    {
        short* p2 = sound->inputsamples[1];
        sound->inputsamples[1] = sound->inputsamples[2];
        short* p3 = sound->inputsamples[5];
        sound->inputsamples[2] = p2;
        short* p4 = sound->inputsamples[3];
        sound->inputsamples[3] = p3;
        sound->inputsamples[5] = sound->inputsamples[4];
        sound->inputsamples[4] = p4;
    }
}

void wavdecode_s16l_int(STDSTREAM* file, unsigned int numchannels, int numsamples, short** inputsamples)
{
    for (int i = 0; i <= numsamples; i++)
    {
        for (unsigned int j = 0; j < numchannels; j++)
        {
            unsigned char buf[2];
            gread(file, buf, 2);
            inputsamples[j][i] = *(unsigned short *)(buf);
        }
    }
}

void wavdecode_s24l_int(STDSTREAM* file, unsigned int numchannels, int numsamples, short** inputsamples)
{
    for (int i = 0; i <= numsamples; i++)
    {
        for (unsigned int j = 0; j < numchannels; j++)
        {
            unsigned char buf[3];
            gread(file, buf, 3);
            inputsamples[j][i] = buf[1] | (unsigned short)(buf[2] << 8);
        }
    }
}

void wavdecode_u8_int(STDSTREAM* file, unsigned int numchannels, int numsamples, short** inputsamples)
{
    for (int i = 0; i <= numsamples; i++)
    {
        for (unsigned int j = 0; j < numchannels; j++)
        {
            unsigned char buf;
            gread(file, &buf, 1);
            inputsamples[j][i] = (buf - 128) << 8;
        }
    }
}

void DecodeData(STDSTREAM* file, int numsamples, unsigned int numchannels, unsigned int codec, short **inputsamples)
{
    switch (codec)
    {
    case s16l_int:
        wavdecode_s16l_int(file, numchannels, numsamples - 1, inputsamples);
        break;
    case u8_int:
        wavdecode_u8_int(file, numchannels, numsamples - 1, inputsamples);
        break;
    case s24l_int:
        wavdecode_s24l_int(file, numchannels, numsamples - 1, inputsamples);
        break;
    }
}

void WaveRead(SSOUND* sound)
{
    AllocateWaveMemory(sound);
    SSOUND snd;
    memcpy(&snd, sound, sizeof(snd));
    ReorderChannels(sound);
    DecodeData(sound->filestruct, sound->numsamples, sound->numchannels, sound->codec, sound->inputsamples);
    memcpy(sound, &snd, sizeof(SSOUND));
}

int main()
{
    SSOUND* s = nullptr;
    WaveInfo("c:\\temp\\test.wav", &s);
    WaveRead(s);
    s->codec = s16b_int;
    s->playloc = PLAYLOC_STREAM;
    EncodeSnr("c:\\temp\\test.snr", s);
}
