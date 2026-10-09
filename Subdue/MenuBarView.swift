import AppKit
import SwiftUI

struct MenuBarView: View {
    @Environment(\.openWindow) private var openWindow
    @EnvironmentObject private var model: SchedulerModel

    var body: some View {
        if let schedule = model.schedule {
            Text(schedule.action.name)
                .font(.headline)
            Text("Scheduled for \(schedule.fireDate.formatted(date: .abbreviated, time: .shortened))")
            Divider()
            Button("Show Subdue") { showMainWindow() }
            Button("Cancel Schedule", role: .destructive) { model.cancel() }
        } else {
            Text(model.statusMessage)
                .lineLimit(2)
            Divider()
            Button("Set Up a Schedule…") { showMainWindow() }
        }

        Divider()
        Button("Quit Subdue") { NSApplication.shared.terminate(nil) }
            .keyboardShortcut("q")
    }

    private func showMainWindow() {
        openWindow(id: "main")
        NSApplication.shared.activate(ignoringOtherApps: true)
    }
}

struct MenuBarLabel: View {
    @EnvironmentObject private var model: SchedulerModel

    var body: some View {
        if model.schedule != nil {
            Label(model.countdownText, systemImage: "clock.fill")
                .monospacedDigit()
        } else if model.didComplete {
            Image(systemName: "checkmark.circle.fill")
                .accessibilityLabel("Subdue completed")
        } else {
            Image(systemName: "moon.zzz")
                .accessibilityLabel("Subdue")
        }
    }

}
