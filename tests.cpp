/* ******************************************************************
 * libzstd-seek-tests
 * Copyright (c) 2020, Martinelli Marco
 *
 * You can contact the author at :
 * - Source repository : https://github.com/martinellimarco/libzstd-seek-tests
 *
 * This source code is licensed under the GPLv3 (found in the LICENSE
 * file in the root directory of this source tree).
****************************************************************** */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc51-cpp"
#pragma ide diagnostic ignored "cert-msc50-cpp"
#pragma ide diagnostic ignored "cert-err58-cpp"
#include <cstdint>
#include <fcntl.h>
#include "libzstd-seek/zstd-seek.h"
#include "gtest/gtest.h"

TEST(ZSTDSeekInvalid, InvalidArguments) {
    ASSERT_EQ(ZSTDSeek_getJumpTableOfContext(nullptr), nullptr);

    ZSTDSeek_freeJumpTable(nullptr);

    ZSTDSeek_addJumpTableRecord(nullptr, 0, 0);

    ZSTDSeek_initializeJumpTable(nullptr);

    ASSERT_EQ(ZSTDSeek_uncompressedFileSize(nullptr), 0);

    ASSERT_EQ(ZSTDSeek_createFromFile(""), nullptr);

    ASSERT_EQ(ZSTDSeek_createFromFile("/dev/null"), nullptr);

    ASSERT_EQ(ZSTDSeek_createFromFileDescriptor(-1), nullptr);

    ASSERT_EQ(ZSTDSeek_read(nullptr, 0, nullptr), 0);

    ASSERT_EQ(ZSTDSeek_seek(nullptr, 0, 0), -1);

    ASSERT_EQ(ZSTDSeek_tell(nullptr), -1);

    ASSERT_EQ(ZSTDSeek_compressedTell(nullptr), -1);

    ASSERT_EQ(ZSTDSeek_isMultiframe(nullptr), 0);

    ASSERT_EQ(ZSTDSeek_getNumberOfFrames(nullptr), 0);

    ZSTDSeek_free(nullptr);
}

//try to read from a non zstd buffer
TEST(ZSTDSeekInvalid, UncompressInvalidData) {
    void *buff = malloc(100000);
    ZSTDSeek_Context *sctx = ZSTDSeek_create(buff, 1);
    ASSERT_EQ (sctx, nullptr);

    free(buff);
}

//try to load a truncated file
TEST(ZSTDSeekInvalid, TruncatedFile) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/truncated.zst");
    ASSERT_EQ (sctx, nullptr);
}

//test seek with an invalid origin
TEST(ZSTDSeekInvalid, SeekInvalidOrigin) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    ASSERT_EQ(ZSTDSeek_seek(sctx, 0, -1), -1);

    ZSTDSeek_free(sctx);
}

/*
 * seek_simple.zst is composed of 4 frames, containing the following bytes:
 * Frame1: ABCD
 * Frame2: EF
 * Frame3: GHIJ
 * Frame4: KLMNOPQRSTUVWXYZ
 *
 * it's an easy test to make sure we are reading correctly even when we hit the boundary of a frame
 * */

TEST(ZSTDSeekTestSimple, OpenFileDescriptor) {
    const char *file = "test_assets/seek_simple.zst";
    int fd = open(file, O_RDONLY, 0);
    ASSERT_GE(fd, 0);

    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFileDescriptor(fd);
    ASSERT_NE (sctx, nullptr);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(jt->length, 5);

    ASSERT_EQ(fd, ZSTDSeek_fileno(sctx));

    ASSERT_EQ(ZSTDSeek_isMultiframe(sctx), 1);

    ASSERT_EQ(ZSTDSeek_getNumberOfFrames(sctx), 4);

    ZSTDSeek_free(sctx);
}
//test seek before read
TEST(ZSTDSeekTestSimple, SeekFirst) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/mimi.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100000];
    int ret;

    ret = ZSTDSeek_seek(sctx, 1, SEEK_SET);
    ASSERT_EQ(ret, 0);

    ret = ZSTDSeek_read(buff, 100000, sctx);
    ASSERT_EQ(ret, 100000);

    ASSERT_EQ(ZSTDSeek_isMultiframe(sctx), 1);

    ASSERT_EQ(ZSTDSeek_getNumberOfFrames(sctx), 16);

    ZSTDSeek_free(sctx);
}

//test if the jump table is automatically generated
TEST(ZSTDSeekTestSimple, JumpTableAutomatic) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(jt->length, 5);

    size_t expectedCompressedPos[] = {0, 17, 32, 49, 78};
    size_t expectedUncompressedPos[] = {0, 4, 6, 10, 26};

    for(uint32_t i = 0; i < jt->length; i++){
        ZSTDSeek_JumpTableRecord r = jt->records[i];
        ASSERT_EQ(r.compressedPos, expectedCompressedPos[i]);
        ASSERT_EQ(r.uncompressedPos, expectedUncompressedPos[i]);
    }

    ASSERT_EQ(ZSTDSeek_uncompressedFileSize(sctx), 26);

    ZSTDSeek_free(sctx);
}

//test if the jump table can be set correctly
TEST(ZSTDSeekTestSimple, JumpTableManual) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFileWithoutJumpTable("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(ZSTDSeek_lastKnownUncompressedFileSize(sctx), 0);

    ASSERT_EQ(jt->length, 0);

    ZSTDSeek_addJumpTableRecord(jt, 0, 0);
    ASSERT_EQ(jt->length, 1);

    ZSTDSeek_addJumpTableRecord(jt, 17, 4);
    ASSERT_EQ(jt->length, 2);

    ZSTDSeek_addJumpTableRecord(jt, 32, 6);
    ASSERT_EQ(jt->length, 3);

    ZSTDSeek_addJumpTableRecord(jt, 49, 10);
    ASSERT_EQ(jt->length, 4);

    ZSTDSeek_addJumpTableRecord(jt, 78, 26);
    ASSERT_EQ(jt->length, 5);

    size_t expectedCompressedPos[] = {0, 17, 32, 49, 78};
    size_t expectedUncompressedPos[] = {0, 4, 6, 10, 26};

    for(uint32_t i = 0; i < jt->length; i++){
        ZSTDSeek_JumpTableRecord r = jt->records[i];
        ASSERT_EQ(r.compressedPos, expectedCompressedPos[i]);
        ASSERT_EQ(r.uncompressedPos, expectedUncompressedPos[i]);
    }

    ASSERT_EQ(ZSTDSeek_uncompressedFileSize(sctx), 26);

    ZSTDSeek_free(sctx);
}

//test if the jump table is automatically generated while reading
TEST(ZSTDSeekTestSimple, JumpTableAutomaticProgressive) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFileWithoutJumpTable("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(ZSTDSeek_lastKnownUncompressedFileSize(sctx), 0);

    ASSERT_EQ(jt->length, 0);

    ret = ZSTDSeek_jumpTableIsInitialized(sctx);
    ASSERT_EQ(ret, 0);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'A');

    ASSERT_EQ(jt->length, 2);

    ret = ZSTDSeek_jumpTableIsInitialized(sctx);
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(ZSTDSeek_lastKnownUncompressedFileSize(sctx), 4);

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 1);

    ret = ZSTDSeek_read(buff, 3, sctx);
    ASSERT_EQ(ret, 3);
    ASSERT_EQ(buff[0], 'B');
    ASSERT_EQ(buff[1], 'C');
    ASSERT_EQ(buff[2], 'D');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 4);

    ASSERT_EQ(jt->length, 2);

    ret = ZSTDSeek_jumpTableIsInitialized(sctx);
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(ZSTDSeek_lastKnownUncompressedFileSize(sctx), 4);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'E');

    ASSERT_EQ(jt->length, 3);

    ret = ZSTDSeek_jumpTableIsInitialized(sctx);
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(ZSTDSeek_lastKnownUncompressedFileSize(sctx), 6);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_END);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(jt->length, 5);

    ret = ZSTDSeek_jumpTableIsInitialized(sctx);
    ASSERT_EQ(ret, 1);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_END);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(jt->length, 5);

    size_t expectedCompressedPos[] = {0, 17, 32, 49, 78};
    size_t expectedUncompressedPos[] = {0, 4, 6, 10, 26};

    for(uint32_t i = 0; i < jt->length; i++){
        ZSTDSeek_JumpTableRecord r = jt->records[i];
        ASSERT_EQ(r.compressedPos, expectedCompressedPos[i]);
        ASSERT_EQ(r.uncompressedPos, expectedUncompressedPos[i]);
    }

    ASSERT_EQ(ZSTDSeek_uncompressedFileSize(sctx), 26);

    ZSTDSeek_free(sctx);
}

TEST(ZSTDSeekTestSimple, ReadSeqAll) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    pos = ZSTDSeek_compressedTell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_read(buff, 26, sctx);
    ASSERT_EQ(ret, 26);
    for(int i=0; i<26; i++){
        ASSERT_EQ(buff[i], 'A'+i);
    }

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 26);

    pos = ZSTDSeek_compressedTell(sctx);
    ASSERT_EQ(pos, 78);

    ZSTDSeek_free(sctx);
}

TEST(ZSTDSeekTestSimple, ReadSeqSmallBlocks) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'A');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 1);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'B');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 2);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'C');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 3);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'D');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 4);

    ret = ZSTDSeek_read(buff, 1, sctx);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(buff[0], 'E');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 5);

    ret = ZSTDSeek_read(buff, 2, sctx);
    ASSERT_EQ(ret, 2);
    ASSERT_EQ(buff[0], 'F');
    ASSERT_EQ(buff[1], 'G');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 7);

    ret = ZSTDSeek_read(buff, 3, sctx);
    ASSERT_EQ(ret, 3);
    ASSERT_EQ(buff[0], 'H');
    ASSERT_EQ(buff[1], 'I');
    ASSERT_EQ(buff[2], 'J');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 10);

    ret = ZSTDSeek_read(buff, 16, sctx);
    ASSERT_EQ(ret, 16);
    ASSERT_EQ(buff[0], 'K');
    ASSERT_EQ(buff[1], 'L');
    ASSERT_EQ(buff[2], 'M');
    ASSERT_EQ(buff[3], 'N');
    ASSERT_EQ(buff[4], 'O');
    ASSERT_EQ(buff[5], 'P');
    ASSERT_EQ(buff[6], 'Q');
    ASSERT_EQ(buff[7], 'R');
    ASSERT_EQ(buff[8], 'S');
    ASSERT_EQ(buff[9], 'T');
    ASSERT_EQ(buff[10], 'U');
    ASSERT_EQ(buff[11], 'V');
    ASSERT_EQ(buff[12], 'W');
    ASSERT_EQ(buff[13], 'X');
    ASSERT_EQ(buff[14], 'Y');
    ASSERT_EQ(buff[15], 'Z');

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 26);

    pos = ZSTDSeek_compressedTell(sctx);
    ASSERT_EQ(pos, 78);

    ZSTDSeek_free(sctx);
}

//test seek_set, moving forward sequentially on even positions (always read inside the same frame because they are all multiple of 2 in size)
TEST(ZSTDSeekTestSimple, SeekSetForwardSeqEven) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    for(int i=0; i<25; i+=2){
        ret = ZSTDSeek_seek(sctx, i, SEEK_SET);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);
    }

    ZSTDSeek_free(sctx);
}

//test seek_set, moving forward sequentially on odd positions (read between consecutive frames)
TEST(ZSTDSeekTestSimple, SeekSetForwardSeqOdd) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    for(int i=1; i<24; i+=2){
        ret = ZSTDSeek_seek(sctx, i, SEEK_SET);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);
    }


    ZSTDSeek_free(sctx);
}

//test seek_set, moving backward sequentially
TEST(ZSTDSeekTestSimple, SeekSetBackward) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    for(int i=25; i>=0; i--){
        ret = ZSTDSeek_seek(sctx, i, SEEK_SET);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);
    }


    ZSTDSeek_free(sctx);
}

//fuzzy test, randomly jump around 100000 times reading a random buffer size
TEST(ZSTDSeekTestSimple, SeekSetFuzzy) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret, j, len;

    srand(0);

    for(int i=0; i<100000; i++){

        j = rand()%26;
        len = 1+(rand()%(26-j));

        ret = ZSTDSeek_seek(sctx, j, SEEK_SET);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j);

        ret = ZSTDSeek_read(buff, len, sctx);
        ASSERT_EQ(ret, len);
        for(int k=j, w=0; w < len; k++, w++){
            ASSERT_EQ(buff[w], 'A' + k);
        }

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j+len);
    }

    ZSTDSeek_free(sctx);
}

//test seek_end and tell, it should match the file size
TEST(ZSTDSeekTestSimple, SeekEndTellFileSize) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_END);
    ASSERT_EQ(ret, 0);

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 26);

    ASSERT_EQ(ZSTDSeek_compressedTell(sctx), 78);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(pos, ZSTDSeek_uncompressedFileSize(sctx));

    ZSTDSeek_free(sctx);
}

//test seek_cur, moving forward sequentially on even positions
TEST(ZSTDSeekTestSimple, SeekCurForwardSeqEven) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    for(int i=0; i<25; i+=2){
        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);

        ret = ZSTDSeek_seek(sctx, 1, SEEK_CUR);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+2);
    }

    ZSTDSeek_free(sctx);
}

//test seek_cur, moving forward sequentially on odd positions
TEST(ZSTDSeekTestSimple, SeekCurForwardSeqOdd) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    for(int i=1; i<24; i+=2){
        ret = ZSTDSeek_seek(sctx, 1, SEEK_CUR);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);
    }

    ZSTDSeek_free(sctx);
}

//test seek_cur, moving backward sequentially on even positions
TEST(ZSTDSeekTestSimple, SeekCurBackwardSeqEven) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100];
    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_seek(sctx, 24, SEEK_SET);
    ASSERT_EQ(ret, 0);

    for(int i=24; i>0; i-=2){
        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i);

        ret = ZSTDSeek_read(buff, 1, sctx);
        ASSERT_EQ(ret, 1);
        ASSERT_EQ(buff[0], 'A'+i);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i+1);

        ret = ZSTDSeek_seek(sctx, -2-1, SEEK_CUR);//-2, -1 that we read above
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, i-2);
    }

    ZSTDSeek_free(sctx);
}

//test seek_set, seek_cur, seek_end with out of file positions
TEST(ZSTDSeekTestSimple, SeekOutOfFile) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/seek_simple.zst");
    ASSERT_NE (sctx, nullptr);

    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_seek(sctx, -1, SEEK_SET);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_NEGATIVE_SEEK);

    ret = ZSTDSeek_seek(sctx, 26, SEEK_SET);
    ASSERT_EQ(ret, 0);

    ret = ZSTDSeek_seek(sctx, 27, SEEK_SET);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_BEYOND_END_SEEK);


    ret = ZSTDSeek_seek(sctx, 1, SEEK_END);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_BEYOND_END_SEEK);

    ret = ZSTDSeek_seek(sctx, -26, SEEK_END);
    ASSERT_EQ(ret, 0);

    ret = ZSTDSeek_seek(sctx, -27, SEEK_END);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_NEGATIVE_SEEK);


    ret = ZSTDSeek_seek(sctx, 0, SEEK_SET);
    ASSERT_EQ(ret, 0);
    ret = ZSTDSeek_seek(sctx, -1, SEEK_CUR);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_NEGATIVE_SEEK);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_SET);
    ASSERT_EQ(ret, 0);
    ret = ZSTDSeek_seek(sctx, 26, SEEK_CUR);
    ASSERT_EQ(ret, 0);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_SET);
    ASSERT_EQ(ret, 0);
    ret = ZSTDSeek_seek(sctx, 27, SEEK_CUR);
    ASSERT_EQ(ret, ZSTDSEEK_ERR_BEYOND_END_SEEK);

    ZSTDSeek_free(sctx);
}


/*
 * 100K.zst is composed of 100 frames, each containing digits from 0 to 9 again and again, 1000 times (1K)
 * it's easy to test because a digit in a certain position pos is expected to be pos%10
 * the name is after the uncompressed size, which is 100KB
 * the file was encoded without uncompressed frame size
 * */

//test seek_end and tell, it should match the file size
TEST(ZSTDSeekTest100K, JumpTable) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(jt->length, 101);

    ASSERT_EQ(ZSTDSeek_uncompressedFileSize(sctx), 100000);

    ZSTDSeek_free(sctx);
}

//test seek_end and tell, it should match the file size
TEST(ZSTDSeekTest100K, SeekEndTellFileSize) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    size_t pos;
    int ret;

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 0);

    ret = ZSTDSeek_seek(sctx, 0, SEEK_END);
    ASSERT_EQ(ret, 0);

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 100000);

    ZSTDSeek_JumpTable *jt = ZSTDSeek_getJumpTableOfContext(sctx);
    ASSERT_NE (jt, nullptr);

    ASSERT_EQ(pos, ZSTDSeek_uncompressedFileSize(sctx));

    ZSTDSeek_free(sctx);
}

//fuzzy test for seek_set, randomly jump around 1000 times reading a random buffer size
TEST(ZSTDSeekTest100K, SeekSetFuzzy) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100000];
    size_t pos;
    int ret, j, len;

    srand(0);

    for(int i=0; i<1000; i++){

        j = rand()%100000;
        len = 1+(rand()%(100000-j));

        ret = ZSTDSeek_seek(sctx, j, SEEK_SET);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j);

        ret = ZSTDSeek_read(buff, len, sctx);
        ASSERT_EQ(ret, len);
        for(int k=j, w=0; w < len; k++, w++){
            ASSERT_EQ(buff[w], '0'+((j+w)%10));
        }

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j+len);
    }

    ZSTDSeek_free(sctx);
}

//fuzzy test for seek_cur, randomly jump around 1000 times reading a random buffer size
TEST(ZSTDSeekTest100K, SeekCurFuzzy) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100000];
    size_t pos;
    int ret, j, len;

    srand(0);

    for(int i=0; i<1000; i++){

        j = rand()%100000;
        len = 1+(rand()%(100000-j));

        pos = ZSTDSeek_tell(sctx);

        ret = ZSTDSeek_seek(sctx, j-(long)pos, SEEK_CUR);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j);

        ret = ZSTDSeek_read(buff, len, sctx);
        ASSERT_EQ(ret, len);
        for(int k=j, w=0; w < len; k++, w++){
            ASSERT_EQ(buff[w], '0'+((j+w)%10));
        }

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j+len);
    }

    ZSTDSeek_free(sctx);
}

//fuzzy test for seek_end, randomly jump around 1000 times reading a random buffer size
TEST(ZSTDSeekTest100K, SeekEndFuzzy) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[100000];
    size_t pos;
    int ret, j, len;

    srand(0);

    for(int i=0; i<1000; i++){

        j = rand()%100000;
        len = 1+(rand()%(100000-j));

        ret = ZSTDSeek_seek(sctx, j-100000, SEEK_END);
        ASSERT_EQ(ret, 0);

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j);

        ret = ZSTDSeek_read(buff, len, sctx);
        ASSERT_EQ(ret, len);
        for(int k=j, w=0; w < len; k++, w++){
            ASSERT_EQ(buff[w], '0'+((j+w)%10));
        }

        pos = ZSTDSeek_tell(sctx);
        ASSERT_EQ(pos, j+len);
    }

    ZSTDSeek_free(sctx);
}

//try to read more than 100KB
TEST(ZSTDSeekTest100K, ReadTooMuch) {
    ZSTDSeek_Context* sctx = ZSTDSeek_createFromFile("test_assets/100K.zst");
    ASSERT_NE (sctx, nullptr);

    char buff[200000];
    size_t pos;
    int ret;

    ret = ZSTDSeek_read(buff, 200000, sctx);
    ASSERT_EQ(ret, 100000);

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 100000);

    ret = ZSTDSeek_read(buff, 200000, sctx);
    ASSERT_EQ(ret, 0);

    pos = ZSTDSeek_tell(sctx);
    ASSERT_EQ(pos, 100000);

    ZSTDSeek_free(sctx);
}

#pragma clang diagnostic pop
