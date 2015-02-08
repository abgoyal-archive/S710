#ifndef ANDROID_LIB_WORDSEGSERVICE_H
#define ANDROID_LIB_WORDSEGSERVICE_H
//#include <binder/IPCThreadState.h>

namespace android {
       class WordSegServiceClient {
              public:
			// Segmentation for a word at touch point
			static bool WordSegment(const char *aStr, int aLen, int aPos, int &aBegin, int &aEnd);
			// Segmentation for a sentence
			static bool WordSegment(const char *aStr, int aLen, int *aOffsetArray, int &aOffsetLen);
		
              private:
                     static void getWordSegService();
			static bool mIsIBinderInit;
			static Mutex mTransacLock;
       };
} //namespace

#endif
