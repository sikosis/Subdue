#include "AudioMuter.h"

#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <String.h>

#include <string.h>


status_t
AudioMuter::Mute(BString& error)
{
	status_t status = B_OK;
	BMediaRoster* roster = BMediaRoster::Roster(&status);
	if (roster == NULL || status != B_OK || !BMediaRoster::IsRunning()) {
		error = "Media services are not running.";
		return status != B_OK ? status : B_NO_INIT;
	}

	media_node mixer = media_node::null;
	status = roster->GetAudioMixer(&mixer);
	if (status != B_OK) {
		error = "Could not find Haiku's system audio mixer.";
		return status;
	}

	BParameterWeb* web = NULL;
	status = roster->GetParameterWebFor(mixer, &web);
	if (status == B_OK && web != NULL) {
		BParameter* mute = NULL;
		for (int32 index = 0; index < web->CountParameters(); index++) {
			BParameter* parameter = web->ParameterAt(index);
			if (parameter != NULL && parameter->Kind() != NULL
				&& strcmp(parameter->Kind(), B_MUTE) == 0) {
				mute = parameter;
				break;
			}
		}

		if (mute != NULL) {
			int32 muted = 1;
			status = mute->SetValue(&muted, sizeof(muted), system_time());
		} else {
			status = B_NAME_NOT_FOUND;
			error = "The system mixer does not expose a master mute control.";
		}
	} else {
		error = "Could not read the system mixer's controls.";
	}

	delete web;
	roster->ReleaseNode(mixer);
	if (status != B_OK && error.IsEmpty())
		error.SetToFormat("Haiku could not mute audio (%s).", strerror(status));
	return status;
}
