#ifndef SUBDUE_PROTOCOL_H
#define SUBDUE_PROTOCOL_H

#include <SupportDefs.h>

inline constexpr const char* kSubdueSignature
	= "application/x-vnd.sikosis-subdue";
inline constexpr const char* kSubdueDeskbarName = "SubdueDeskbarView";

inline constexpr uint32 kMsgQueryState = 'sqry';
inline constexpr uint32 kMsgState = 'ssta';
inline constexpr uint32 kMsgShowWindow = 'sshw';
inline constexpr uint32 kMsgCancelSchedule = 'scnl';
inline constexpr uint32 kMsgQuitApp = 'squi';

#endif
