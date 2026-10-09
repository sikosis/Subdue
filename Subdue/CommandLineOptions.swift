import Foundation

struct CommandLineOptions {
    var application: String?
    var muteAudio = false
    var schedule: ScheduleChoice?
    var forceIfNeeded = true
    var foreground = false
    var showHelp = false
    var error: String?

    static let usage = """
    Usage: Subdue (--app NAME | --mute) (--after MINUTES | --at HH:mm | --top-of-hour) [--no-force] [--foreground]

      --app NAME              Application name, bundle identifier, or .app path
      --mute                  Smother: mute the default audio output
      --after MINUTES         Run the action after MINUTES (a positive integer)
      --at HH:mm              Run at the next occurrence of local 24-hour time
      --top-of-hour (or top)  Run at the start of the next hour
      --force                 Force-terminate after 10 seconds (the default)
      --no-force              Only request a graceful quit
      --foreground            Keep a direct terminal launch attached
      --help                  Show this help
    """

    static func parse(_ arguments: [String]) -> CommandLineOptions {
        var result = CommandLineOptions()
        var index = 0

        func fail(_ message: String) -> CommandLineOptions {
            var failed = result
            failed.error = message
            return failed
        }

        while index < arguments.count {
            let argument = arguments[index]
            switch argument {
            case "--help", "-h": result.showHelp = true
            case "--force": result.forceIfNeeded = true
            case "--no-force": result.forceIfNeeded = false
            case "--foreground": result.foreground = true
            case "--mute": result.muteAudio = true
            case "--app":
                index += 1
                guard index < arguments.count else { return fail("--app requires a value") }
                result.application = arguments[index]
            case "--after":
                guard result.schedule == nil else { return fail("Supply only one timing option") }
                index += 1
                guard index < arguments.count, let minutes = Int(arguments[index]), minutes > 0 else {
                    return fail("--after requires a positive number of minutes")
                }
                result.schedule = .after(minutes: minutes)
            case "--at":
                guard result.schedule == nil else { return fail("Supply only one timing option") }
                index += 1
                guard index < arguments.count else { return fail("--at requires a time in HH:mm format") }
                let parts = arguments[index].split(separator: ":", omittingEmptySubsequences: false)
                guard parts.count == 2, let hour = Int(parts[0]), let minute = Int(parts[1]),
                      (0...23).contains(hour), (0...59).contains(minute) else {
                    return fail("--at requires a valid 24-hour time in HH:mm format")
                }
                result.schedule = .at(hour: hour, minute: minute)
            case "--top":
                guard result.schedule == nil else { return fail("Supply only one timing option") }
                result.schedule = .topOfHour
            case "--top-of-hour":
                guard result.schedule == nil else { return fail("Supply only one timing option") }
                result.schedule = .topOfHour
            default: return fail("Unknown option: \(argument)")
            }
            index += 1
        }
        if result.application != nil && result.muteAudio {
            return fail("Supply either --app or --mute, not both")
        }
        if result.schedule != nil && result.application == nil && !result.muteAudio {
            return fail("A timing option requires either --app NAME or --mute")
        }
        if result.schedule == nil && (result.application != nil || result.muteAudio) {
            return fail("Supply one timing option")
        }
        return result
    }
}
