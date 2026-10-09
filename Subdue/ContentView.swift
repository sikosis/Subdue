import SwiftUI

struct ContentView: View {
    enum ActionMode: String, CaseIterable, Identifiable {
        case subdue = "Subdue App"
        case smother = "Smother Audio"
        var id: Self { self }
    }

    enum TimingMode: String, CaseIterable, Identifiable {
        case after = "After"
        case at = "At time"
        case top = "Top of hour"
        var id: Self { self }
    }

    @EnvironmentObject private var model: SchedulerModel
    @State private var actionMode: ActionMode = .subdue
    @State private var timingMode: TimingMode = .after
    @State private var minutes = 30
    @State private var chosenTime = Date()
    @State private var forceIfNeeded = true

    var body: some View {
        VStack(alignment: .leading, spacing: 20) {
            HStack(spacing: 12) {
                Image(nsImage: NSApplication.shared.applicationIconImage)
                    .resizable()
                    .scaledToFit()
                    .frame(width: 52, height: 52)
                VStack(alignment: .leading, spacing: 2) {
                    Text("Subdue").font(.largeTitle.bold())
                    Text("Quiet an application or your audio on your schedule.").foregroundStyle(.secondary)
                }
            }

            if model.schedule == nil {
                Form {
                    Picker("Action", selection: $actionMode) {
                        ForEach(ActionMode.allCases) { Text($0.rawValue).tag($0) }
                    }
                    .pickerStyle(.segmented)

                    if actionMode == .subdue {
                        Picker("Application", selection: $model.selectedTarget) {
                            Text("Choose an application").tag(AppTarget?.none)
                            ForEach(model.runningApplications) { app in
                                Text(app.name).tag(Optional(app))
                            }
                        }
                    }
                    HStack {
                        Picker("When", selection: $timingMode) {
                            ForEach(TimingMode.allCases) { Text($0.rawValue).tag($0) }
                        }
                        .frame(width: 230)
                        timingControl
                    }
                    if actionMode == .subdue {
                        Toggle("Force quit if the application does not close after 10 seconds", isOn: $forceIfNeeded)
                    } else {
                        Label("Mutes the current default audio output.", systemImage: "speaker.slash.fill")
                            .foregroundStyle(.secondary)
                    }
                }
                .formStyle(.grouped)

                HStack {
                    if actionMode == .subdue {
                        Button("Refresh Applications") { model.refreshApplications() }
                    }
                    Spacer()
                    Button(actionMode == .subdue ? "Schedule Subdue" : "Schedule Smother") { scheduleAction() }
                        .keyboardShortcut(.defaultAction)
                        .disabled(actionMode == .subdue && model.selectedTarget == nil)
                }
            } else {
                GroupBox {
                    VStack(alignment: .leading, spacing: 12) {
                        Label(model.statusMessage, systemImage: "clock.badge.checkmark")
                            .font(.title3)
                        if model.schedule != nil {
                            Text(model.countdownText)
                                .font(.system(.title, design: .rounded).monospacedDigit().bold())
                        }
                        Button("Cancel Schedule", role: .destructive) { model.cancel() }
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                }
            }

            Text(model.statusMessage).font(.footnote).foregroundStyle(.secondary)
        }
        .padding(24)
        .frame(width: 500, height: 390)
    }

    @ViewBuilder private var timingControl: some View {
        switch timingMode {
        case .after:
            Stepper("\(minutes) minutes", value: $minutes, in: 1...10_080)
        case .at:
            DatePicker("", selection: $chosenTime, displayedComponents: .hourAndMinute)
                .labelsHidden()
        case .top:
            Text("Next hour").foregroundStyle(.secondary)
        }
    }

    private func scheduleAction() {
        let choice: ScheduleChoice
        switch timingMode {
        case .after: choice = .after(minutes: minutes)
        case .top: choice = .topOfHour
        case .at:
            let components = Calendar.current.dateComponents([.hour, .minute], from: chosenTime)
            choice = .at(hour: components.hour ?? 0, minute: components.minute ?? 0)
        }
        switch actionMode {
        case .subdue:
            guard let target = model.selectedTarget else { return }
            model.arm(target: target, choice: choice, forceIfNeeded: forceIfNeeded)
        case .smother:
            model.armMute(choice: choice)
        }
        if let armedSchedule = model.schedule {
            let scheduledWindow = NSApplication.shared.keyWindow
            DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
                guard model.schedule == armedSchedule else { return }
                scheduledWindow?.orderOut(nil)
            }
        }
    }

}
