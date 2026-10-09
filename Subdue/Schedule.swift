import Foundation

enum ScheduleChoice: Equatable {
    case after(minutes: Int)
    case at(hour: Int, minute: Int)
    case topOfHour

    func fireDate(from now: Date = Date(), calendar: Calendar = .current) -> Date? {
        switch self {
        case .after(let minutes):
            return calendar.date(byAdding: .minute, value: minutes, to: now)
        case .topOfHour:
            guard let nextHour = calendar.date(byAdding: .hour, value: 1, to: now) else { return nil }
            let components = calendar.dateComponents([.year, .month, .day, .hour], from: nextHour)
            return calendar.date(from: components)
        case .at(let hour, let minute):
            var components = calendar.dateComponents([.year, .month, .day], from: now)
            components.hour = hour
            components.minute = minute
            components.second = 0
            guard let today = calendar.date(from: components) else { return nil }
            return today > now ? today : calendar.date(byAdding: .day, value: 1, to: today)
        }
    }
}

enum ScheduledAction: Equatable {
    case subdue(AppTarget)
    case smother

    var name: String {
        switch self {
        case .subdue(let target): target.name
        case .smother: "System audio"
        }
    }
}

struct ScheduledEvent: Equatable {
    let action: ScheduledAction
    let fireDate: Date
    let forceIfNeeded: Bool
}
