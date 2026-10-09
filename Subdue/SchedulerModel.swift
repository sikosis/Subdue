import AppKit
import Combine
import Foundation

@MainActor
final class SchedulerModel: ObservableObject {
    @Published var runningApplications: [AppTarget] = []
    @Published var selectedTarget: AppTarget?
    @Published var schedule: ScheduledEvent?
    @Published var statusMessage = "Choose an action and a time."
    @Published var didComplete = false
    @Published var countdownText = ""

    private var timer: Timer?
    private var countdownTimer: Timer?

    init() {
        refreshApplications()
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didLaunchApplicationNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in Task { @MainActor in self?.refreshApplications() } }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didTerminateApplicationNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in Task { @MainActor in self?.refreshApplications() } }
    }

    func refreshApplications() {
        let ownBundleID = Bundle.main.bundleIdentifier
        runningApplications = NSWorkspace.shared.runningApplications
            .filter { $0.activationPolicy == .regular && $0.bundleIdentifier != ownBundleID && !$0.isTerminated }
            .map(AppTarget.init(application:))
            .reduce(into: [String: AppTarget]()) { $0[$1.id] = $1 }
            .values
            .sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }

        if let selectedTarget, !runningApplications.contains(selectedTarget) {
            self.selectedTarget = nil
        }
    }

    func target(for value: String) -> AppTarget {
        let expanded = NSString(string: value).expandingTildeInPath
        if expanded.hasSuffix(".app") || expanded.contains("/") {
            let url = URL(fileURLWithPath: expanded)
            let bundle = Bundle(url: url)
            return AppTarget(
                name: bundle?.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String ?? url.deletingPathExtension().lastPathComponent,
                bundleIdentifier: bundle?.bundleIdentifier,
                bundleURL: url
            )
        }
        if let match = runningApplications.first(where: {
            $0.name.localizedCaseInsensitiveCompare(value) == .orderedSame || $0.bundleIdentifier == value
        }) { return match }
        return AppTarget(name: value, bundleIdentifier: value.contains(".") ? value : nil)
    }

    func arm(target: AppTarget, choice: ScheduleChoice, forceIfNeeded: Bool) {
        arm(action: .subdue(target), choice: choice, forceIfNeeded: forceIfNeeded)
    }

    func armMute(choice: ScheduleChoice) {
        arm(action: .smother, choice: choice, forceIfNeeded: false)
    }

    private func arm(action: ScheduledAction, choice: ScheduleChoice, forceIfNeeded: Bool) {
        guard let date = choice.fireDate() else {
            statusMessage = "That time could not be scheduled."
            return
        }
        timer?.invalidate()
        let scheduled = ScheduledEvent(action: action, fireDate: date, forceIfNeeded: forceIfNeeded)
        schedule = scheduled
        didComplete = false
        updateCountdown(to: date)
        switch action {
        case .subdue(let target):
            statusMessage = "\(target.name) will quit \(date.formatted(date: .abbreviated, time: .shortened))."
        case .smother:
            statusMessage = "System audio will mute \(date.formatted(date: .abbreviated, time: .shortened))."
        }
        timer = Timer(fire: date, interval: 0, repeats: false) { [weak self] _ in
            Task { @MainActor in self?.performScheduledAction() }
        }
        if let timer { RunLoop.main.add(timer, forMode: .common) }
        countdownTimer?.invalidate()
        countdownTimer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { [weak self] _ in
            Task { @MainActor in
                guard let self, let fireDate = self.schedule?.fireDate else { return }
                self.updateCountdown(to: fireDate)
            }
        }
    }

    func cancel() {
        timer?.invalidate()
        countdownTimer?.invalidate()
        timer = nil
        countdownTimer = nil
        schedule = nil
        didComplete = false
        countdownText = ""
        statusMessage = "Schedule cancelled."
    }

    private func performScheduledAction() {
        guard let schedule else { return }
        switch schedule.action {
        case .smother:
            finish(AudioMuter.muteDefaultOutput() ? "System audio was muted." : "Could not mute the default audio output.")
        case .subdue(let target):
            performScheduledQuit(target: target, forceIfNeeded: schedule.forceIfNeeded)
        }
    }

    private func performScheduledQuit(target: AppTarget, forceIfNeeded: Bool) {
        let applications = target.matchingRunningApplications()
        guard !applications.isEmpty else {
            finish("\(target.name) was not running at the scheduled time.")
            return
        }
        applications.forEach { $0.terminate() }
        statusMessage = "Asked \(target.name) to quit."

        guard forceIfNeeded else {
            finish("Asked \(target.name) to quit.")
            return
        }
        let targets = applications
        DispatchQueue.main.asyncAfter(deadline: .now() + 10) { [weak self] in
            let remaining = targets.filter { !$0.isTerminated }
            remaining.forEach { $0.forceTerminate() }
            self?.finish(remaining.isEmpty ? "\(target.name) quit." : "Force-quit \(target.name).")
        }
    }

    private func finish(_ message: String) {
        timer = nil
        countdownTimer?.invalidate()
        countdownTimer = nil
        schedule = nil
        countdownText = ""
        statusMessage = message
        didComplete = true
    }

    private func updateCountdown(to date: Date, from now: Date = Date()) {
        let remaining = max(0, Int(date.timeIntervalSince(now)))
        let hours = remaining / 3600
        let minutes = (remaining % 3600) / 60
        let seconds = remaining % 60
        countdownText = hours > 0
            ? String(format: "%d:%02d:%02d", hours, minutes, seconds)
            : String(format: "%02d:%02d", minutes, seconds)
    }
}
