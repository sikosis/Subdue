#ifndef SUBDUE_DESKBAR_VIEW_H
#define SUBDUE_DESKBAR_VIEW_H

#include <String.h>
#include <SupportDefs.h>
#include <View.h>

class BMessageRunner;

class SubdueDeskbarView : public BView {
public:
	explicit SubdueDeskbarView(BRect frame);
	explicit SubdueDeskbarView(BMessage* archive);
	virtual ~SubdueDeskbarView();

	static BArchivable* Instantiate(BMessage* archive);
	virtual status_t Archive(BMessage* archive, bool deep = true) const;
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual void MessageReceived(BMessage* message);

	static status_t Install();
	static status_t Remove();

private:
	enum { kPulse = 'spul' };

	void _QueryState();
	void _ShowMenu(BPoint where);
	status_t _SendCommand(uint32 command, bool launchIfMissing = false);
	void _SetDisconnected();

	BMessageRunner* fRunner;
	bool fConnected;
	bool fScheduled;
	BString fCountdown;
	BString fTarget;
};

extern "C" _EXPORT BView* instantiate_deskbar_item();

#endif
