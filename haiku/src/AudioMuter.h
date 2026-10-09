#ifndef SUBDUE_AUDIO_MUTER_H
#define SUBDUE_AUDIO_MUTER_H

#include <SupportDefs.h>

class BString;

class AudioMuter {
public:
	static status_t Mute(BString& error);
};

#endif
