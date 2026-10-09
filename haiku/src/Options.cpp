#include "Options.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static bool
ParsePositive(const char* value, int32& result)
{
	char* end = NULL;
	long parsed = strtol(value, &end, 10);
	if (value == end || *end != '\0' || parsed <= 0 || parsed > INT32_MAX)
		return false;
	result = (int32)parsed;
	return true;
}


static bool
ParseClock(const char* value, int32& hour, int32& minute)
{
	char extra = '\0';
	int parsedHour;
	int parsedMinute;
	if (sscanf(value, "%d:%d%c", &parsedHour, &parsedMinute, &extra) != 2
		|| parsedHour < 0 || parsedHour > 23 || parsedMinute < 0
		|| parsedMinute > 59) {
		return false;
	}
	hour = parsedHour;
	minute = parsedMinute;
	return true;
}


Options
Options::Parse(int argc, const char* const* argv)
{
	Options result;
	for (int index = 1; index < argc; index++) {
		const char* argument = argv[index];
		if (strcmp(argument, "--help") == 0 || strcmp(argument, "-h") == 0)
			result.help = true;
		else if (strcmp(argument, "--mute") == 0)
			result.action = ActionMode::Smother;
		else if (strcmp(argument, "--force") == 0)
			result.force = true;
		else if (strcmp(argument, "--no-force") == 0)
			result.force = false;
		else if (strcmp(argument, "--foreground") == 0)
			result.foreground = true;
		else if (strcmp(argument, "--app") == 0) {
			if (++index >= argc) {
				result.error = "--app requires a value";
				break;
			}
			result.application = argv[index];
		} else if (strcmp(argument, "--after") == 0) {
			if (result.timing != TimingMode::None || ++index >= argc
				|| !ParsePositive(argv[index], result.minutes)) {
				result.error = "--after requires one positive number of minutes";
				break;
			}
			result.timing = TimingMode::After;
		} else if (strcmp(argument, "--at") == 0) {
			if (result.timing != TimingMode::None || ++index >= argc
				|| !ParseClock(argv[index], result.hour, result.minute)) {
				result.error = "--at requires one valid 24-hour time in HH:mm format";
				break;
			}
			result.timing = TimingMode::At;
		} else if (strcmp(argument, "--top") == 0
			|| strcmp(argument, "--top-of-hour") == 0) {
			if (result.timing != TimingMode::None) {
				result.error = "Supply only one timing option";
				break;
			}
			result.timing = TimingMode::TopOfHour;
		} else {
			result.error.SetToFormat("Unknown option: %s", argument);
			break;
		}
	}

	if (result.error.IsEmpty() && !result.help) {
		if (result.action == ActionMode::Smother && !result.application.IsEmpty())
			result.error = "Supply either --app or --mute, not both";
		else if (result.HasSchedule() && result.action == ActionMode::Subdue
			&& result.application.IsEmpty())
			result.error = "A schedule requires --app NAME or --mute";
		else if (!result.HasSchedule()
			&& (!result.application.IsEmpty() || result.action == ActionMode::Smother))
			result.error = "Supply one timing option";
	}
	return result;
}


const char*
Options::Usage()
{
	return "Usage: Subdue (--app NAME | --mute) "
		"(--after MINUTES | --at HH:mm | --top-of-hour) "
		"[--no-force] [--foreground]\n\n"
		"  --app NAME       Running application name or signature\n"
		"  --mute           Smother the Haiku system audio mixer\n"
		"  --after MINUTES  Run after a positive number of minutes\n"
		"  --at HH:mm       Run at the next local occurrence of the time\n"
		"  --top-of-hour    Run at the start of the next hour\n"
		"  --force          Force-close after 10 seconds (default)\n"
		"  --no-force       Only request a graceful quit\n"
		"  --foreground     Keep the window visible\n";
}
